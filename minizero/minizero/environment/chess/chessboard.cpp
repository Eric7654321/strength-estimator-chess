#include "chessboard.h"
#include "bitboard.h"
#include "color_message.h"
#include "move_generator.h"
#include "random.h"

#include <algorithm>
#include <string>

namespace minizero::env::chess {

using namespace minizero::utils;

uint64_t PieceKeys[2][6][64] = {0};
uint64_t CastleKeys[4] = {0};
uint64_t EnPassantKeys[8] = {0};
uint64_t whiteToMoveKey = 0;

// generate key for position hash
void initKeys()
{
    for (int i = 0; i < 8; i++) {
        EnPassantKeys[i] = utils::Random::randInt64();
    }
    for (int i = 0; i < 4; i++) {
        CastleKeys[i] = utils::Random::randInt64();
    }
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 6; j++) {
            for (int k = 0; k < 64; k++) {
                PieceKeys[i][j][k] = utils::Random::randInt64();
            }
        }
    }
    whiteToMoveKey = utils::Random::randInt64();
}

// generate board string, white pieces are uppercase, black pieces are lowercase and blue.
std::string ChessBoard::toString(Square prev_move_from, Square prev_move_to) const
{
    std::string board;

    for (int i = 0; i < 64; i++) {
        if (pawns_.get(i)) {
            board += 'p';
        } else if (knights_.get(i)) {
            board += 'n';
        } else if (bishops_.get(i)) {
            board += 'b';
        } else if (rooks_.get(i)) {
            board += 'r';
        } else if (queens_.get(i)) {
            board += 'q';
        } else if (kings_.get(i)) {
            board += 'k';
        } else {
            board += '.';
        }
        if (white_pieces_.get(i)) {
            // convert last char to uppercase
            board.back() -= 32;
        }
        if (i % 8 == 7) {
            board += '\n';
        } else {
            board += ' ';
        }
    }

    std::ostringstream result;
    for (int i = 7; i >= 0; i--) {
        result << i + 1 << "  ";
        for (int j = 0; j < 16; j++) {
            if ((prev_move_from == i * 8 + j / 2 || prev_move_to == i * 8 + j / 2) && j % 2 == 0) {
                if (white_pieces_.get(i * 8 + j / 2)) {
                    result << getColorText(std::string() + board[i * 16 + j], TextType::kBold, TextColor::kWhite, TextColor::kGreen);
                } else if (black_pieces_.get(i * 8 + j / 2)) {
                    result << getColorText(std::string() + board[i * 16 + j], TextType::kBold, TextColor::kBlue, TextColor::kGreen);
                } else {
                    result << getColorText(std::string() + board[i * 16 + j], TextType::kBold, TextColor::kWhite, TextColor::kGreen);
                }
            } else if (white_pieces_.get(i * 8 + j / 2)) {
                result << getColorText(std::string() + board[i * 16 + j], TextType::kBold, TextColor::kSize, TextColor::kSize);
            } else if (black_pieces_.get(i * 8 + j / 2)) {
                result << getColorText(std::string() + board[i * 16 + j], TextType::kBold, TextColor::kBlue, TextColor::kSize);
            } else {
                result << board[i * 16 + j];
            }
        }
    }

    result << "   a b c d e f g h\n\n";

    return result.str();
}

bool ChessBoard::act(Square from, Square to, char promotion, bool update)
{
    updateDrawCondition(from, to);
    checkEnPassant(from, to);
    checkPromotion(promotion, from);
    castling(from, to);
    pawns_.update(from, to);
    knights_.update(from, to);
    bishops_.update(from, to);
    rooks_.update(from, to);
    queens_.update(from, to);
    kings_.update(from, to);
    white_pieces_.update(from, to);
    black_pieces_.update(from, to);
    all_pieces_.update(from, to);

    position_hash_history_.push_back(generateHash());
    player_ = getNextPlayer(player_, 2);

    // this is false when move generation is filtering pins/checks
    if (update) {
        updateGameState();
    }
    fullmove_number_++;

    return true;
}

void ChessBoard::checkEnPassant(Square from, Square to)
{
    // check if en passant is played
    if (pawns_.get(from) && en_passant_.get(to)) {
        // clear captured pawn
        if (white_pieces_.get(from)) {
            pawns_.clear(to.square_ - 8);
            black_pieces_.clear(to.square_ - 8);
            all_pieces_.clear(to.square_ - 8);
        } else if (black_pieces_.get(from)) {
            pawns_.clear(to.square_ + 8);
            white_pieces_.clear(to.square_ + 8);
            all_pieces_.clear(to.square_ + 8);
        }
    }
    en_passant_.reset();
    // check for double pawn moves
    if (pawns_.get(from) && (abs(from.rank_ - to.rank_) == 2)) {
        en_passant_.set((from.square_ + to.square_) / 2);
    }
}

void ChessBoard::checkPromotion(char promotion, Square from)
{
    if (promotion) {
        switch (promotion) {
            case 'q': queens_.set(from); break;
            case 'r': rooks_.set(from); break;
            case 'b': bishops_.set(from); break;
            case 'n': knights_.set(from); break;
            default: std::cerr << "invalid promotion piece" << std::endl;
        }
        pawns_.clear(from);
    }
}

Bitboard ChessBoard::generateMoves(Square from) const
{
    Bitboard our_pieces = white_pieces_.get(from) ? white_pieces_ : black_pieces_;
    Bitboard moves(0);

    if (pawns_.get(from)) {
        if (white_pieces_.get(from)) {
            moves = generateWhitePawnMoves(from, all_pieces_, black_pieces_ | en_passant_);
        } else if (black_pieces_.get(from)) {
            moves = generateBlackPawnMoves(from, all_pieces_, white_pieces_ | en_passant_);
        }
    } else if (knights_.get(from)) {
        moves = generateKnightMoves(from);
    } else if (bishops_.get(from)) {
        moves = generateBishopMoves(from, all_pieces_);
    } else if (rooks_.get(from)) {
        moves = generateRookMoves(from, all_pieces_);
    } else if (queens_.get(from)) {
        moves = generateQueenMoves(from, all_pieces_);
    } else if (kings_.get(from)) {
        moves = generateKingMoves(from);
    } else {
        moves = Bitboard(0);
    }

    // Remove moves that would capture our own pieces
    moves &= ~our_pieces;

    return moves;
}

Bitboard ChessBoard::generateLegalMoves(Square from) const
{
    Bitboard moves = generateMoves(from);
    // add castling moves
    if (kings_.get(from)) {
        if (from == 4) {
            Bitboard all_black_moves = Bitboard(0);
            for (auto from : black_pieces_) {
                all_black_moves |= generateMoves(from);
                // pawn attacks on castling squares even if no pieces are there
                if (pawns_.get(from)) {
                    all_black_moves.set(from.square_ + 7);
                    all_black_moves.set(from.square_ + 9);
                }
            }

            // can castle if no squares are occupied or attacked between king and rook, and castling rights are true
            bool can_white_castle_kingside =
                (all_black_moves & kWhiteKingsideSquares).empty() &&
                (all_pieces_ & (kWhiteKingsideSquares & ~kings_)).empty() &&
                (castling_rights_ & 1);
            bool can_white_castle_queenside =
                (all_black_moves & kWhiteQueensideSquares).empty() &&
                (all_pieces_ & (kWhiteQueensideSquares & ~kings_)).empty() &&
                (castling_rights_ & 2);

            if (can_white_castle_kingside) {
                moves.set(6);
            }
            if (can_white_castle_queenside) {
                moves.set(2);
            }
        } else if (from == 60) {
            Bitboard all_white_moves = Bitboard(0);
            for (auto from : white_pieces_) {
                all_white_moves |= generateMoves(from);
                // pawn attacks on castling squares even if no pieces are there
                if (pawns_.get(from)) {
                    all_white_moves.set(from.square_ + 7);
                    all_white_moves.set(from.square_ + 9);
                }
            }

            // can castle if no squares occupied or attacked between king and rook, and castling rights are set
            bool can_black_castle_kingside =
                (all_white_moves & kBlackKingsideSquares).empty() &&
                (all_pieces_ & (kBlackKingsideSquares & ~kings_)).empty() &&
                (castling_rights_ & 4);

            bool can_black_castle_queenside =
                (all_white_moves & kBlackQueensideSquares).empty() &&
                (all_pieces_ & (kBlackQueensideSquares & ~kings_)).empty() &&
                (castling_rights_ & 8);

            if (can_black_castle_kingside) {
                moves.set(62);
            }
            if (can_black_castle_queenside) {
                moves.set(58);
            }
        }
    }

    // remove moves that would put our king in check
    // e.g. pins, illegal king moves
    for (auto to : moves) {
        ChessBoard temp_board = *this;
        // promotion would not be relevant here
        temp_board.act(from, to, '\0', false);
        if (temp_board.isPlayerInCheck(player_)) {
            moves.clear(to);
        }
    }
    return moves;
}

bool ChessBoard::isPlayerInCheck(Player player) const
{
    Bitboard their_pieces = theirPieces(player);
    Bitboard our_king = ourPieces(player) & kings_;
    Square our_king_position = Square(our_king.getLSB());

    for (auto from : their_pieces) {
        Bitboard moves = generateMoves(from);
        if (moves.get(our_king_position)) {
            return true;
        }
    }
    return false;
}

void ChessBoard::updateGameState()
{
    Bitboard all_moves = Bitboard(0);
    for (auto from : ourPieces()) {
        all_moves |= generateLegalMoves(from);
    }

    if (all_moves.empty()) {
        if (isPlayerInCheck(player_)) {
            // Checkmate
            if (player_ == Player::kPlayer1) {
                game_state_ = GameState::BlackWin;
            } else if (player_ == Player::kPlayer2) {
                game_state_ = GameState::WhiteWin;
            }
        } else {
            // Stalemate
            game_state_ = GameState::Draw;
        }
    }
    // Insufficient material
    if (!hasMatingMaterial()) {
        game_state_ = GameState::Draw;
    }
    // 50-move rule
    if (fifty_move_rule_ == 100) {
        game_state_ = GameState::Draw;
    }
    // check for three-fold repetition
    if (getRepetitionCount() >= 2) {
        game_state_ = GameState::Draw;
    }
}

bool ChessBoard::hasMatingMaterial() const
{
    if (!pawns_.empty() || !rooks_.empty() || !queens_.empty()) {
        return true;
    }

    int num_white_bishops = (bishops_ & white_pieces_).count();
    int num_black_bishops = (bishops_ & black_pieces_).count();
    int num_white_knights = (knights_ & white_pieces_).count();
    int num_black_knights = (knights_ & black_pieces_).count();

    if (num_white_bishops + num_white_knights > 1 || num_black_bishops + num_black_knights > 1) {
        return true;
    }

    return false;
}

void ChessBoard::updateDrawCondition(Square from, Square to)
{
    fifty_move_rule_++;
    // reset if pawn is moved or piece is captured
    if (pawns_.get(from) || all_pieces_.get(to)) {
        fifty_move_rule_ = 0;
    }
}

void ChessBoard::castling(Square from, Square to)
{
    // remove castling rights if king or rook is moved or captured
    if (from == 0 || to == 0) {
        castling_rights_ &= ~2;
    } else if (from == 7 || to == 7) {
        castling_rights_ &= ~1;
    } else if (from == 4 || to == 4) {
        castling_rights_ &= ~3;
    } else if (from == 56 || to == 56) {
        castling_rights_ &= ~8;
    } else if (from == 60 || to == 60) {
        castling_rights_ &= ~12;
    } else if (from == 63 || to == 63) {
        castling_rights_ &= ~4;
    }

    // move rook if castling
    if (kings_.get(from) && abs(from.file_ - to.file_) == 2) {
        if (from == 4) {
            if (to == 2) {
                rooks_.update(0, 3);
                white_pieces_.update(0, 3);
                all_pieces_.update(0, 3);
            } else if (to == 6) {
                rooks_.update(7, 5);
                white_pieces_.update(7, 5);
                all_pieces_.update(7, 5);
            }
        } else if (from == 60) {
            if (to == 58) {
                rooks_.update(56, 59);
                black_pieces_.update(56, 59);
                all_pieces_.update(56, 59);
            } else if (to == 62) {
                rooks_.update(63, 61);
                black_pieces_.update(63, 61);
                all_pieces_.update(63, 61);
            }
        }
    }
}

void ChessBoard::setFen(std::string fen)
{
    std::string board;
    std::istringstream fen_str(fen);
    fen_str >> board;

    int rank = 7;
    int file = 0;
    for (char c : board) {
        int i = rank * 8 + file;
        if (c == '/') {
            rank--;
            file = 0;
            continue;
        }
        if (std::isdigit(c)) {
            file += c - '0';
            continue;
        }
        if (std::isupper(c)) {
            white_pieces_.set(i);
        } else {
            black_pieces_.set(i);
        }
        all_pieces_.set(i);
        if (c == 'k' || c == 'K') {
            kings_.set(i);
        } else if (c == 'q' || c == 'Q') {
            queens_.set(i);
        } else if (c == 'r' || c == 'R') {
            rooks_.set(i);
        } else if (c == 'b' || c == 'B') {
            bishops_.set(i);
        } else if (c == 'n' || c == 'N') {
            knights_.set(i);
        } else if (c == 'p' || c == 'P') {
            pawns_.set(i);
        }
        file++;
    }

    // set player
    std::string player_string = "w";
    if (!fen_str.eof()) fen_str >> player_string;
    player_ = player_string == "w" ? Player::kPlayer1 : Player::kPlayer2;

    // set castling rights
    std::string castling_rights_string = "-";
    if (!fen_str.eof()) fen_str >> castling_rights_string;
    castling_rights_ = 0;
    for (char c : castling_rights_string) {
        if (c == 'K') {
            castling_rights_ |= 1;
        } else if (c == 'Q') {
            castling_rights_ |= 2;
        } else if (c == 'k') {
            castling_rights_ |= 4;
        } else if (c == 'q') {
            castling_rights_ |= 8;
        }
    }

    // set en passant square
    std::string en_passant_string = "-";
    if (!fen_str.eof()) fen_str >> en_passant_string;
    if (en_passant_string != "-") {
        en_passant_.set(Square(en_passant_string));
    }

    if (!fen_str.eof()) fen_str >> fifty_move_rule_;
    if (!fen_str.eof()) fen_str >> fullmove_number_;
}

std::string ChessBoard::getFen() const
{
    std::string fen_string = "";
    for (int rank = 7; rank >= 0; rank--) {
        int empty_count = 0;
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            if (pawns_.get(square)) {
                if (empty_count > 0) {
                    fen_string += std::to_string(empty_count);
                    empty_count = 0;
                }
                fen_string += 'p';
            } else if (knights_.get(square)) {
                if (empty_count > 0) {
                    fen_string += std::to_string(empty_count);
                    empty_count = 0;
                }
                fen_string += 'n';
            } else if (bishops_.get(square)) {
                if (empty_count > 0) {
                    fen_string += std::to_string(empty_count);
                    empty_count = 0;
                }
                fen_string += 'b';
            } else if (rooks_.get(square)) {
                if (empty_count > 0) {
                    fen_string += std::to_string(empty_count);
                    empty_count = 0;
                }
                fen_string += 'r';
            } else if (queens_.get(square)) {
                if (empty_count > 0) {
                    fen_string += std::to_string(empty_count);
                    empty_count = 0;
                }
                fen_string += 'q';
            } else if (kings_.get(square)) {
                if (empty_count > 0) {
                    fen_string += std::to_string(empty_count);
                    empty_count = 0;
                }
                fen_string += 'k';
            } else {
                empty_count++;
            }
            if (white_pieces_.get(square)) {
                fen_string.back() -= 32;
            }
        }
        if (empty_count > 0) {
            fen_string += std::to_string(empty_count);
        }
        if (rank > 0) {
            fen_string += '/';
        }
    }

    fen_string += " " + std::string(player_ == Player::kPlayer1 ? "w" : "b") + " ";

    std::string castling_rights = "";
    if (castling_rights_ & 1) castling_rights += 'K';
    if (castling_rights_ & 2) castling_rights += 'Q';
    if (castling_rights_ & 4) castling_rights += 'k';
    if (castling_rights_ & 8) castling_rights += 'q';
    if (castling_rights.empty()) castling_rights = "-";

    fen_string += castling_rights + " ";
    std::string en_passant = en_passant_.empty() ? "-" : kChessPositions[en_passant_.getLSB()];
    fen_string += en_passant + " " + std::to_string(fifty_move_rule_) + " " + std::to_string((fullmove_number_ + 1) / 2);

    return fen_string;
}

uint64_t ChessBoard::generateHash() const
{
    uint64_t hash = 0;
    for (auto i : white_pieces_) {
        if (pawns_.get(i)) {
            hash ^= PieceKeys[0][0][i.square_];
        } else if (knights_.get(i)) {
            hash ^= PieceKeys[0][1][i.square_];
        } else if (bishops_.get(i)) {
            hash ^= PieceKeys[0][2][i.square_];
        } else if (rooks_.get(i)) {
            hash ^= PieceKeys[0][3][i.square_];
        } else if (queens_.get(i)) {
            hash ^= PieceKeys[0][4][i.square_];
        } else if (kings_.get(i)) {
            hash ^= PieceKeys[0][5][i.square_];
        }
    }
    for (auto i : black_pieces_) {
        if (pawns_.get(i)) {
            hash ^= PieceKeys[1][0][i.square_];
        } else if (knights_.get(i)) {
            hash ^= PieceKeys[1][1][i.square_];
        } else if (bishops_.get(i)) {
            hash ^= PieceKeys[1][2][i.square_];
        } else if (rooks_.get(i)) {
            hash ^= PieceKeys[1][3][i.square_];
        } else if (queens_.get(i)) {
            hash ^= PieceKeys[1][4][i.square_];
        } else if (kings_.get(i)) {
            hash ^= PieceKeys[1][5][i.square_];
        }
    }
    if (!en_passant_.empty()) {
        int en_passant_square = en_passant_.getLSB();
        hash ^= EnPassantKeys[en_passant_square % 8];
    }
    if (castling_rights_ & 1) {
        hash ^= CastleKeys[0];
    }
    if (castling_rights_ & 2) {
        hash ^= CastleKeys[1];
    }
    if (castling_rights_ & 4) {
        hash ^= CastleKeys[2];
    }
    if (castling_rights_ & 8) {
        hash ^= CastleKeys[3];
    }
    if (player_ == Player::kPlayer1) {
        hash ^= whiteToMoveKey;
    }

    return hash;
}

std::vector<uint64_t> ChessBoard::getPositionInfo() const
{
    std::vector<uint64_t> position;
    position.push_back((white_pieces_ & pawns_).bitboard_);
    position.push_back((black_pieces_ & pawns_).bitboard_);
    position.push_back((white_pieces_ & bishops_).bitboard_);
    position.push_back((black_pieces_ & bishops_).bitboard_);
    position.push_back((white_pieces_ & rooks_).bitboard_);
    position.push_back((black_pieces_ & rooks_).bitboard_);
    position.push_back((white_pieces_ & queens_).bitboard_);
    position.push_back((black_pieces_ & queens_).bitboard_);
    position.push_back((white_pieces_ & kings_).bitboard_);
    position.push_back((black_pieces_ & kings_).bitboard_);
    position.push_back((white_pieces_ & knights_).bitboard_);
    position.push_back((black_pieces_ & knights_).bitboard_);

    int repetitions = getRepetitionCount();
    if (repetitions == 0) {
        position.push_back(0);
        position.push_back(0);
    } else if (repetitions == 1) {
        position.push_back(1);
        position.push_back(0);
    } else {
        position.push_back(1);
        position.push_back(1);
    }

    return position;
}

} // namespace minizero::env::chess
