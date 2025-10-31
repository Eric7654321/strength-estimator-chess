#include "chess.h"
#include "move_generator.h"
#include "random.h"

#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace minizero::env::chess {

std::vector<std::string> kChessActionName;
std::unordered_map<std::string, int> kChessActionMap;
int kChessActionID[64][64];
int kPromotionActionID[64][64][4];

void initialize()
{
    // compute action string and id mapping
    for (int i = 0; i < kMovePerSquare * kChessBoardSize * kChessBoardSize; i++) {
        generateActionString(i);
    }

    // check if the mapping is correct
    for (std::size_t i = 0; i < kChessActionName.size(); i++) {
        ChessAction action({"W", kChessActionName[i]});
        assert(static_cast<int>(i) == action.getActionID());
    }
    initKeys();
    initSlidingMoves();
}

ChessAction::ChessAction(int action_id, Player player) : BaseAction(action_id, player)
{
    if (action_id == -1) {
        from_ = Square(-1);
        to_ = Square(-1);
        promotion_ = '\0';
        return;
    }
    std::string action_string = kChessActionName[action_id];
    from_ = Square(action_string.substr(0, 2));
    to_ = Square(action_string.substr(2, 2));

    // if action string contains promotion, set promotion piece
    if (action_string.size() == 5) {
        promotion_ = action_string[4];
    } else {
        promotion_ = '\0';
    }
}

ChessAction::ChessAction(const std::vector<std::string>& action_string_args)
{
    // construct action from action string
    assert(action_string_args.size() == 2);
    assert(action_string_args[0].size() == 1);

    const std::string action_string = action_string_args[1];
    assert(kChessActionMap.count(action_string));

    action_id_ = kChessActionMap[action_string];
    player_ = charToPlayer(action_string_args[0][0]);
    from_ = Square(action_string.substr(0, 2));
    to_ = Square(action_string.substr(2, 2));

    // if action string contains promotion, set promotion piece
    if (action_string.size() == 5) {
        promotion_ = action_string[4];
    } else {
        promotion_ = '\0';
    }
}

void generateActionString(int action_id)
{
    std::string action_string;
    char promotion = '\0';

    const int start_square = action_id / kMovePerSquare;
    const int start_rank = start_square / kChessBoardSize;
    const int start_file = start_square % kChessBoardSize;

    int end_file = start_file;
    int end_rank = start_rank;

    const int move_id = action_id % kMovePerSquare;

    if (move_id < 56) {
        // queen moves
        const int direction = move_id / 7;    // 0: N, 1: NE, 2: E, 3: SE, 4: S, 5: SW, 6: W, 7: NW
        const int distance = move_id % 7 + 1; // 1~7
        end_file += distance * ((direction % 4 == 0) ? 0 : ((direction > 4) ? -1 : 1));
        end_rank += distance * ((direction % 4 == 2) ? 0 : ((direction > 2 && direction < 6) ? -1 : 1));
    } else if (move_id < 64) {
        // knight moves
        int end_file_idx[8] = {1, 2, 2, 1, -1, -2, -2, -1};
        int end_rank_idx[8] = {2, 1, -1, -2, -2, -1, 1, 2};
        end_file += end_file_idx[move_id - 56];
        end_rank += end_rank_idx[move_id - 56];
    } else if (move_id < 76) {
        // promotion
        if (start_rank == 6) {
            const int direction = (move_id - 64) / 4; // 0: up-left, 1: up, 2: up-right

            const std::string promotion_pieces = "qrbn";
            promotion = promotion_pieces[(move_id - 64) % 4]; // 0: queen, 1: rook, 2: bishop, 3: knight

            end_file = end_file - 1 + direction;
            end_rank += 1;
        } else if (start_rank == 1) {
            const int direction = (move_id - 64) / 4; // 0: down-left, 1: down, 2: down-right

            const std::string promotion_pieces = "qrbn";
            promotion = promotion_pieces[(move_id - 64) % 4]; // 0: queen, 1: rook, 2: bishop, 3: knight

            end_file = end_file - 1 + direction;
            end_rank -= 1;
        } else {
            return;
        }
    } else {
        std::cerr << "Invalid Move";
    }

    // check bounds
    if (end_rank < 0 || end_rank > 7 || end_file < 0 || end_file > 7) return;

    const int end_square = end_rank * kChessBoardSize + end_file;

    // action_string: "a1b1" or "g7h7q"
    action_string = kChessPositions[start_square] + kChessPositions[end_square];

    if (promotion == '\0') {
        kChessActionID[start_square][end_square] = kChessActionName.size();
    } else {
        action_string += promotion;
        kPromotionActionID[start_square][end_square][(move_id - 64) % 4] = kChessActionName.size();
    }

    kChessActionMap[action_string] = kChessActionName.size();
    kChessActionName.push_back(action_string);
}

std::string ChessAction::toConsoleString() const
{
    return kChessActionName[action_id_];
}

// reconstruct the board
void ChessEnv::reset()
{
    if (config::env_chess_initial_fen.size()) {
        setFen(config::env_chess_initial_fen);
    } else {
        position_history_.clear();
        board_ = ChessBoard();
        position_history_.push_back(board_.getPositionInfo());
        actions_.clear();
        turn_ = Player::kPlayer1;
    }
}

void ChessEnv::setFen(const std::string& fen)
{
    position_history_.clear();
    board_ = ChessBoard(fen);
    position_history_.push_back(board_.getPositionInfo());
    actions_.clear();
    turn_ = board_.player_;
}

std::string ChessEnv::getFen() const
{
    return board_.getFen();
}

bool ChessEnv::act(const ChessAction& action)
{
    if (isLegalAction(action)) {
        actions_.push_back(action);
        turn_ = getNextPlayer(board_.player_, kChessNumPlayer);
        // default char with no promotion
        board_.act(action.from_, action.to_, action.promotion_);

        position_history_.push_back(board_.getPositionInfo());
        if (position_history_.size() > 8) {
            position_history_.erase(position_history_.begin());
        }

        return true;
    }

    return false;
}

// action_string_args: {player, action_string}, e.g. {"W", "a1b1"}
bool ChessEnv::act(const std::vector<std::string>& action_string_args)
{
    ChessAction action = ChessAction(action_string_args);
    return act(action);
}

std::vector<ChessAction> ChessEnv::getLegalActions() const
{
    std::vector<ChessAction> legal_actions;

    for (auto from : board_.ourPieces()) {
        Bitboard moves = board_.generateLegalMoves(from);

        for (auto to : moves) {
            // check if the move is a promotion
            if (board_.pawns_.get(from) && ((to.rank_ == 7) || (to.rank_ == 0))) {
                for (int i = 0; i < 4; i++) {
                    int promotion_action_id = kPromotionActionID[from.square_][to.square_][i];
                    legal_actions.push_back(ChessAction(promotion_action_id, board_.player_));
                }
            } else {
                int action_id = kChessActionID[from.square_][to.square_];
                legal_actions.push_back(ChessAction(action_id, board_.player_));
            }
        }
    }

    return legal_actions;
}

bool ChessEnv::isLegalAction(const ChessAction& action) const
{
    Square from = action.from_;
    Square to = action.to_;

    // promotion from non-pawn piece is illegal
    if (!board_.pawns_.get(from) && action.promotion_ != '\0') {
        return false;
    }

    // non-promotion move to promotion square is illegal
    if (board_.pawns_.get(from) && (to.rank_ == 7 || to.rank_ == 0) && action.promotion_ == '\0') {
        return false;
    }

    return board_.generateLegalMoves(from).get(to) && board_.ourPieces().get(from);
}

bool ChessEnv::isTerminal() const
{
    return board_.game_state_ != GameState::Playing;
}

float ChessEnv::getEvalScore(bool is_resign) const
{
    if (is_resign) {
        if (board_.player_ == Player::kPlayer1) {
            return -1.0f;
        } else if (board_.player_ == Player::kPlayer2) {
            return 1.0f;
        }
    }
    if (board_.game_state_ == GameState::Draw) {
        return 0.0f;
    } else if (board_.game_state_ == GameState::WhiteWin) {
        return 1.0f;
    } else if (board_.game_state_ == GameState::BlackWin) {
        return -1.0f;
    } else {
        return 0.0f;
    }
}

std::string ChessEnv::toString() const
{
    if (actions_.empty()) {
        return board_.toString(Square(-1), Square(-1));
    } else {
        return board_.toString(actions_.back().from_, actions_.back().to_);
    }
}

std::vector<float> ChessEnv::getFeatures(utils::Rotation rotation) const
{
    std::vector<float> features;

    // push last 8 positions
    for (size_t i = 0; i < 8 - position_history_.size(); i++) {
        for (int i = 0; i < 64 * 14; i++) {
            features.push_back(0.0f);
        }
    }

    for (auto position : position_history_) {
        // push 14 feature planes
        for (size_t i = 0; i < position.size() - 2; i++) {
            for (int j = 0; j < 64; j++) {
                if (position[i] & (1ULL << j)) {
                    features.push_back(1.0f);
                } else {
                    features.push_back(0.0f);
                }
            }
        }

        int repetition_once = position[position.size() - 2];
        for (int i = 0; i < 64; i++) {
            features.push_back(repetition_once);
        }
        int repetition_twice = position[position.size() - 1];
        for (int i = 0; i < 64; i++) {
            features.push_back(repetition_twice);
        }
    }

    for (int i = 0; i < 64; i++) {
        features.push_back((board_.castling_rights_ & 1) ? 1.0f : 0.0f);
    }
    for (int i = 0; i < 64; i++) {
        features.push_back((board_.castling_rights_ & 2) ? 1.0f : 0.0f);
    }
    for (int i = 0; i < 64; i++) {
        features.push_back((board_.castling_rights_ & 4) ? 1.0f : 0.0f);
    }
    for (int i = 0; i < 64; i++) {
        features.push_back((board_.castling_rights_ & 8) ? 1.0f : 0.0f);
    }
    for (int i = 0; i < 64; i++) {
        features.push_back(board_.fifty_move_rule_ / 2);
    }
    for (int i = 0; i < 64; i++) {
        features.push_back(board_.fullmove_number_ / 2);
    }
    for (int i = 0; i < 64; i++) {
        features.push_back(board_.player_ == Player::kPlayer1 ? 1.0f : 0.0f);
    }

    return features;
}

std::vector<float> ChessEnv::getActionFeatures(const ChessAction& action, utils::Rotation rotation) const
{
    std::vector<float> action_features;
    for (int i = 0; i < getBoardSize() * getBoardSize(); i++) {
        action_features.push_back((action.from_ == i) ? 1.0f : 0.0f);
    }
    for (int i = 0; i < getBoardSize() * getBoardSize(); i++) {
        action_features.push_back((action.to_ == i) ? 1.0f : 0.0f);
    }
    for (int i = 0; i < getBoardSize() * getBoardSize(); i++) {
        action_features.push_back((action.promotion_ == 'q') ? 1.0f : 0.0f);
    }
    for (int i = 0; i < getBoardSize() * getBoardSize(); i++) {
        action_features.push_back((action.promotion_ == 'n') ? 1.0f : 0.0f);
    }
    for (int i = 0; i < getBoardSize() * getBoardSize(); i++) {
        action_features.push_back((action.promotion_ == 'b') ? 1.0f : 0.0f);
    }
    for (int i = 0; i < getBoardSize() * getBoardSize(); i++) {
        action_features.push_back((action.promotion_ == 'r') ? 1.0f : 0.0f);
    }
    for (int i = 0; i < getBoardSize() * getBoardSize(); i++) {
        action_features.push_back((action.promotion_ == '\0') ? 1.0f : 0.0f);
    }

    return action_features;
}

std::vector<float> ChessEnvLoader::getActionFeatures(const int pos, utils::Rotation rotation) const
{
    ChessAction action = action_pairs_[pos].first;
    if (pos > static_cast<int>(action_pairs_.size())) {
        int action_id = utils::Random::randInt() % kChessActionSize;
        action = ChessAction(action_id, Player::kPlayer1);
    }
    ChessEnv env;

    return env.getActionFeatures(action, rotation);
}

} // namespace minizero::env::chess
