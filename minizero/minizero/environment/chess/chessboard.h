#pragma once

#include "base_env.h"
#include "bitboard.h"
#include "move_generator.h"
#include <iostream>
#include <string>
#include <vector>

namespace minizero::env::chess {

extern uint64_t PieceKeys[2][6][64];
extern uint64_t CastleKeys[4];
extern uint64_t EnPassantKeys[8];
extern uint64_t whiteToMoveKey;

const uint64_t kWhiteKingsideSquares = 0x0000000000000070;
const uint64_t kWhiteQueensideSquares = 0x000000000000001C;
const uint64_t kBlackKingsideSquares = 0x7000000000000000;
const uint64_t kBlackQueensideSquares = 0x1C00000000000000;
const std::string kStartingFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

const std::string kChessPositions[] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"};

enum class GameState {
    Playing,
    WhiteWin,
    BlackWin,
    Draw,
};

void initKeys();

class ChessBoard {
public:
    ChessBoard() : game_state_(GameState::Playing)
    {
        setFen(kStartingFen);
    }

    ChessBoard(std::string fen) : game_state_(GameState::Playing)
    {
        setFen(fen);
    }

    std::string toString(Square prev_move_from, Square prev_move_to) const;
    bool act(Square from, Square to, char promotion, bool update = true);
    void checkEnPassant(Square from, Square to);
    void checkPromotion(char promotion, Square from);
    bool hasMatingMaterial() const;
    bool isPlayerInCheck(Player player) const;
    // generate pseudolegal moves
    Bitboard generateMoves(Square square) const;
    // remove moves from generateMoves that would leave the king in check
    Bitboard generateLegalMoves(Square square) const;
    std::vector<uint64_t> getPositionInfo() const;
    void updateGameState();
    void updateDrawCondition(Square from, Square to);
    void castling(Square from, Square to);
    void setFen(std::string fen);
    std::string getFen() const;
    uint64_t generateHash() const;
    inline Bitboard ourPieces(Player player) const
    {
        return player == Player::kPlayer1 ? white_pieces_ : black_pieces_;
    }
    inline Bitboard ourPieces() const
    {
        return ourPieces(player_);
    }
    inline Bitboard theirPieces(Player player) const
    {
        return player == Player::kPlayer1 ? black_pieces_ : white_pieces_;
    }
    inline Bitboard theirPieces() const
    {
        return theirPieces(player_);
    }
    inline int getRepetitionCount() const
    {
        int repetitions = 0;
        for (int i = position_hash_history_.size() - 3; i >= 0; i -= 2) {
            if (position_hash_history_[i] == position_hash_history_.back()) {
                repetitions++;
            }
        }
        return repetitions;
    }

    GameState game_state_;
    Player player_;
    std::vector<uint64_t> position_hash_history_; // store position hashes for three-fold repetition
    int fifty_move_rule_;                         // 50-move draws
    int fullmove_number_;                         // number of full moves
    int castling_rights_;                         // 1 = white kingside, 2 = white queenside, 4 = black kingside, 8 = black queenside
    Bitboard en_passant_;                         // en passant square

    Bitboard all_pieces_;

    Bitboard white_pieces_;
    Bitboard black_pieces_;

    Bitboard pawns_; // store white and black pawns
    Bitboard knights_;
    Bitboard bishops_;
    Bitboard rooks_;
    Bitboard queens_;
    Bitboard kings_;
};

} // namespace minizero::env::chess
