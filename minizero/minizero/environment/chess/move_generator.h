#pragma once

#include "bitboard.h"
#include "square.h"
namespace minizero::env::chess {

extern const Bitboard kWhitePawnCaptures[64];
extern const Bitboard kBlackPawnCaptures[64];
extern const Bitboard kKnightAttacks[64];
extern const Bitboard kKingAttacks[64];
extern const Bitboard kBishopMasks[64];
extern const Bitboard kRookMasks[64];
extern Bitboard kBishopTable[64][1024];
extern Bitboard kRookTable[64][4096];
extern const int kRookShiftBits[64];
extern const int kBishopShiftBits[64];

uint64_t getBlocker(int index, Bitboard mask);

void initSlidingMoves();

Bitboard generateWhitePawnMoves(Square from, Bitboard all_pieces, Bitboard capture_pieces);

Bitboard generateBlackPawnMoves(Square from, Bitboard all_pieces, Bitboard capture_pieces);

Bitboard generateKnightMoves(Square from);

Bitboard generateKingMoves(Square from);

Bitboard generateRookMoves(Square from, Bitboard all_pieces);

Bitboard generateBishopMoves(Square from, Bitboard all_pieces);

Bitboard generateQueenMoves(Square from, Bitboard all_pieces);

Bitboard generateRookMovesSlow(Square from, Bitboard all_pieces);

Bitboard generateBishopMovesSlow(Square from, Bitboard all_pieces);

} // namespace minizero::env::chess
