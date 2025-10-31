#include "move_generator.h"
#include "bitboard.h"
namespace minizero::env::chess {

// references:
//  1. Shallow-Blue; https://github.com/GunshipPenguin/shallow-blue
//  2. LeelaChessZero: https://github.com/LeelaChessZero/lc0

const Bitboard kWhitePawnCaptures[64] = {
    0x200ULL,
    0x500ULL,
    0xa00ULL,
    0x1400ULL,
    0x2800ULL,
    0x5000ULL,
    0xa000ULL,
    0x4000ULL,
    0x20000ULL,
    0x50000ULL,
    0xa0000ULL,
    0x140000ULL,
    0x280000ULL,
    0x500000ULL,
    0xa00000ULL,
    0x400000ULL,
    0x2000000ULL,
    0x5000000ULL,
    0xa000000ULL,
    0x14000000ULL,
    0x28000000ULL,
    0x50000000ULL,
    0xa0000000ULL,
    0x40000000ULL,
    0x200000000ULL,
    0x500000000ULL,
    0xa00000000ULL,
    0x1400000000ULL,
    0x2800000000ULL,
    0x5000000000ULL,
    0xa000000000ULL,
    0x4000000000ULL,
    0x20000000000ULL,
    0x50000000000ULL,
    0xa0000000000ULL,
    0x140000000000ULL,
    0x280000000000ULL,
    0x500000000000ULL,
    0xa00000000000ULL,
    0x400000000000ULL,
    0x2000000000000ULL,
    0x5000000000000ULL,
    0xa000000000000ULL,
    0x14000000000000ULL,
    0x28000000000000ULL,
    0x50000000000000ULL,
    0xa0000000000000ULL,
    0x40000000000000ULL,
    0x200000000000000ULL,
    0x500000000000000ULL,
    0xa00000000000000ULL,
    0x1400000000000000ULL,
    0x2800000000000000ULL,
    0x5000000000000000ULL,
    0xa000000000000000ULL,
    0x4000000000000000ULL,
    0x0ULL,
    0x0ULL,
    0x0ULL,
    0x0ULL,
    0x0ULL,
    0x0ULL,
    0x0ULL,
    0x0ULL,
};

const Bitboard kBlackPawnCaptures[64] = {
    0x0ULL,
    0x0ULL,
    0x0ULL,
    0x0ULL,
    0x0ULL,
    0x0ULL,
    0x0ULL,
    0x0ULL,
    0x2ULL,
    0x5ULL,
    0xaULL,
    0x14ULL,
    0x28ULL,
    0x50ULL,
    0xa0ULL,
    0x40ULL,
    0x200ULL,
    0x500ULL,
    0xa00ULL,
    0x1400ULL,
    0x2800ULL,
    0x5000ULL,
    0xa000ULL,
    0x4000ULL,
    0x20000ULL,
    0x50000ULL,
    0xa0000ULL,
    0x140000ULL,
    0x280000ULL,
    0x500000ULL,
    0xa00000ULL,
    0x400000ULL,
    0x2000000ULL,
    0x5000000ULL,
    0xa000000ULL,
    0x14000000ULL,
    0x28000000ULL,
    0x50000000ULL,
    0xa0000000ULL,
    0x40000000ULL,
    0x200000000ULL,
    0x500000000ULL,
    0xa00000000ULL,
    0x1400000000ULL,
    0x2800000000ULL,
    0x5000000000ULL,
    0xa000000000ULL,
    0x4000000000ULL,
    0x20000000000ULL,
    0x50000000000ULL,
    0xa0000000000ULL,
    0x140000000000ULL,
    0x280000000000ULL,
    0x500000000000ULL,
    0xa00000000000ULL,
    0x400000000000ULL,
    0x2000000000000ULL,
    0x5000000000000ULL,
    0xa000000000000ULL,
    0x14000000000000ULL,
    0x28000000000000ULL,
    0x50000000000000ULL,
    0xa0000000000000ULL,
    0x40000000000000ULL,
};

const Bitboard kKingAttacks[64] = {
    0x302ULL,
    0x705ULL,
    0xe0aULL,
    0x1c14ULL,
    0x3828ULL,
    0x7050ULL,
    0xe0a0ULL,
    0xc040ULL,
    0x30203ULL,
    0x70507ULL,
    0xe0a0eULL,
    0x1c141cULL,
    0x382838ULL,
    0x705070ULL,
    0xe0a0e0ULL,
    0xc040c0ULL,
    0x3020300ULL,
    0x7050700ULL,
    0xe0a0e00ULL,
    0x1c141c00ULL,
    0x38283800ULL,
    0x70507000ULL,
    0xe0a0e000ULL,
    0xc040c000ULL,
    0x302030000ULL,
    0x705070000ULL,
    0xe0a0e0000ULL,
    0x1c141c0000ULL,
    0x3828380000ULL,
    0x7050700000ULL,
    0xe0a0e00000ULL,
    0xc040c00000ULL,
    0x30203000000ULL,
    0x70507000000ULL,
    0xe0a0e000000ULL,
    0x1c141c000000ULL,
    0x382838000000ULL,
    0x705070000000ULL,
    0xe0a0e0000000ULL,
    0xc040c0000000ULL,
    0x3020300000000ULL,
    0x7050700000000ULL,
    0xe0a0e00000000ULL,
    0x1c141c00000000ULL,
    0x38283800000000ULL,
    0x70507000000000ULL,
    0xe0a0e000000000ULL,
    0xc040c000000000ULL,
    0x302030000000000ULL,
    0x705070000000000ULL,
    0xe0a0e0000000000ULL,
    0x1c141c0000000000ULL,
    0x3828380000000000ULL,
    0x7050700000000000ULL,
    0xe0a0e00000000000ULL,
    0xc040c00000000000ULL,
    0x203000000000000ULL,
    0x507000000000000ULL,
    0xa0e000000000000ULL,
    0x141c000000000000ULL,
    0x2838000000000000ULL,
    0x5070000000000000ULL,
    0xa0e0000000000000ULL,
    0x40c0000000000000ULL,
};

const Bitboard kRookMasks[64] = {
    0x101010101017eULL,
    0x202020202027cULL,
    0x404040404047aULL,
    0x8080808080876ULL,
    0x1010101010106eULL,
    0x2020202020205eULL,
    0x4040404040403eULL,
    0x8080808080807eULL,
    0x1010101017e00ULL,
    0x2020202027c00ULL,
    0x4040404047a00ULL,
    0x8080808087600ULL,
    0x10101010106e00ULL,
    0x20202020205e00ULL,
    0x40404040403e00ULL,
    0x80808080807e00ULL,
    0x10101017e0100ULL,
    0x20202027c0200ULL,
    0x40404047a0400ULL,
    0x8080808760800ULL,
    0x101010106e1000ULL,
    0x202020205e2000ULL,
    0x404040403e4000ULL,
    0x808080807e8000ULL,
    0x101017e010100ULL,
    0x202027c020200ULL,
    0x404047a040400ULL,
    0x8080876080800ULL,
    0x1010106e101000ULL,
    0x2020205e202000ULL,
    0x4040403e404000ULL,
    0x8080807e808000ULL,
    0x1017e01010100ULL,
    0x2027c02020200ULL,
    0x4047a04040400ULL,
    0x8087608080800ULL,
    0x10106e10101000ULL,
    0x20205e20202000ULL,
    0x40403e40404000ULL,
    0x80807e80808000ULL,
    0x17e0101010100ULL,
    0x27c0202020200ULL,
    0x47a0404040400ULL,
    0x8760808080800ULL,
    0x106e1010101000ULL,
    0x205e2020202000ULL,
    0x403e4040404000ULL,
    0x807e8080808000ULL,
    0x7e010101010100ULL,
    0x7c020202020200ULL,
    0x7a040404040400ULL,
    0x76080808080800ULL,
    0x6e101010101000ULL,
    0x5e202020202000ULL,
    0x3e404040404000ULL,
    0x7e808080808000ULL,
    0x7e01010101010100ULL,
    0x7c02020202020200ULL,
    0x7a04040404040400ULL,
    0x7608080808080800ULL,
    0x6e10101010101000ULL,
    0x5e20202020202000ULL,
    0x3e40404040404000ULL,
    0x7e80808080808000ULL};

const Bitboard kBishopMasks[64] = {
    0x40201008040200ULL,
    0x402010080400ULL,
    0x4020100a00ULL,
    0x40221400ULL,
    0x2442800ULL,
    0x204085000ULL,
    0x20408102000ULL,
    0x2040810204000ULL,
    0x20100804020000ULL,
    0x40201008040000ULL,
    0x4020100a0000ULL,
    0x4022140000ULL,
    0x244280000ULL,
    0x20408500000ULL,
    0x2040810200000ULL,
    0x4081020400000ULL,
    0x10080402000200ULL,
    0x20100804000400ULL,
    0x4020100a000a00ULL,
    0x402214001400ULL,
    0x24428002800ULL,
    0x2040850005000ULL,
    0x4081020002000ULL,
    0x8102040004000ULL,
    0x8040200020400ULL,
    0x10080400040800ULL,
    0x20100a000a1000ULL,
    0x40221400142200ULL,
    0x2442800284400ULL,
    0x4085000500800ULL,
    0x8102000201000ULL,
    0x10204000402000ULL,
    0x4020002040800ULL,
    0x8040004081000ULL,
    0x100a000a102000ULL,
    0x22140014224000ULL,
    0x44280028440200ULL,
    0x8500050080400ULL,
    0x10200020100800ULL,
    0x20400040201000ULL,
    0x2000204081000ULL,
    0x4000408102000ULL,
    0xa000a10204000ULL,
    0x14001422400000ULL,
    0x28002844020000ULL,
    0x50005008040200ULL,
    0x20002010080400ULL,
    0x40004020100800ULL,
    0x20408102000ULL,
    0x40810204000ULL,
    0xa1020400000ULL,
    0x142240000000ULL,
    0x284402000000ULL,
    0x500804020000ULL,
    0x201008040200ULL,
    0x402010080400ULL,
    0x2040810204000ULL,
    0x4081020400000ULL,
    0xa102040000000ULL,
    0x14224000000000ULL,
    0x28440200000000ULL,
    0x50080402000000ULL,
    0x20100804020000ULL,
    0x40201008040200ULL};

const Bitboard kKnightAttacks[64] = {
    0x0000000000020400ULL,
    0x0000000000050800ULL,
    0x00000000000A1100ULL,
    0x0000000000142200ULL,
    0x0000000000284400ULL,
    0x0000000000508800ULL,
    0x0000000000A01000ULL,
    0x0000000000402000ULL,
    0x0000000002040004ULL,
    0x0000000005080008ULL,
    0x000000000A110011ULL,
    0x0000000014220022ULL,
    0x0000000028440044ULL,
    0x0000000050880088ULL,
    0x00000000A0100010ULL,
    0x0000000040200020ULL,
    0x0000000204000402ULL,
    0x0000000508000805ULL,
    0x0000000A1100110AULL,
    0x0000001422002214ULL,
    0x0000002844004428ULL,
    0x0000005088008850ULL,
    0x000000A0100010A0ULL,
    0x0000004020002040ULL,
    0x0000020400040200ULL,
    0x0000050800080500ULL,
    0x00000A1100110A00ULL,
    0x0000142200221400ULL,
    0x0000284400442800ULL,
    0x0000508800885000ULL,
    0x0000A0100010A000ULL,
    0x0000402000204000ULL,
    0x0002040004020000ULL,
    0x0005080008050000ULL,
    0x000A1100110A0000ULL,
    0x0014220022140000ULL,
    0x0028440044280000ULL,
    0x0050880088500000ULL,
    0x00A0100010A00000ULL,
    0x0040200020400000ULL,
    0x0204000402000000ULL,
    0x0508000805000000ULL,
    0x0A1100110A000000ULL,
    0x1422002214000000ULL,
    0x2844004428000000ULL,
    0x5088008850000000ULL,
    0xA0100010A0000000ULL,
    0x4020002040000000ULL,
    0x0400040200000000ULL,
    0x0800080500000000ULL,
    0x1100110A00000000ULL,
    0x2200221400000000ULL,
    0x4400442800000000ULL,
    0x8800885000000000ULL,
    0x100010A000000000ULL,
    0x2000204000000000ULL,
    0x0004020000000000ULL,
    0x0008050000000000ULL,
    0x00110A0000000000ULL,
    0x0022140000000000ULL,
    0x0044280000000000ULL,
    0x0088500000000000ULL,
    0x0010A00000000000ULL,
    0x0020400000000000ULL};

Bitboard kBishopTable[64][1024] = {0};
Bitboard kRookTable[64][4096] = {0};

const uint64_t kRookMagicNumbers[64] = {
    0xa8002c000108020ULL,
    0x6c00049b0002001ULL,
    0x100200010090040ULL,
    0x2480041000800801ULL,
    0x280028004000800ULL,
    0x900410008040022ULL,
    0x280020001001080ULL,
    0x2880002041000080ULL,
    0xa000800080400034ULL,
    0x4808020004000ULL,
    0x2290802004801000ULL,
    0x411000d00100020ULL,
    0x402800800040080ULL,
    0xb000401004208ULL,
    0x2409000100040200ULL,
    0x1002100004082ULL,
    0x22878001e24000ULL,
    0x1090810021004010ULL,
    0x801030040200012ULL,
    0x500808008001000ULL,
    0xa08018014000880ULL,
    0x8000808004000200ULL,
    0x201008080010200ULL,
    0x801020000441091ULL,
    0x800080204005ULL,
    0x1040200040100048ULL,
    0x120200402082ULL,
    0xd14880480100080ULL,
    0x12040280080080ULL,
    0x100040080020080ULL,
    0x9020010080800200ULL,
    0x813241200148449ULL,
    0x491604001800080ULL,
    0x100401000402001ULL,
    0x4820010021001040ULL,
    0x400402202000812ULL,
    0x209009005000802ULL,
    0x810800601800400ULL,
    0x4301083214000150ULL,
    0x204026458e001401ULL,
    0x40204000808000ULL,
    0x8001008040010020ULL,
    0x8410820820420010ULL,
    0x1003001000090020ULL,
    0x804040008008080ULL,
    0x12000810020004ULL,
    0x1000100200040208ULL,
    0x430000a044020001ULL,
    0x280009023410300ULL,
    0xe0100040002240ULL,
    0x200100401700ULL,
    0x2244100408008080ULL,
    0x8000400801980ULL,
    0x2000810040200ULL,
    0x8010100228810400ULL,
    0x2000009044210200ULL,
    0x4080008040102101ULL,
    0x40002080411d01ULL,
    0x2005524060000901ULL,
    0x502001008400422ULL,
    0x489a000810200402ULL,
    0x1004400080a13ULL,
    0x4000011008020084ULL,
    0x26002114058042ULL};

const uint64_t kBishopMagicNumbers[64] = {
    0x89a1121896040240ULL,
    0x2004844802002010ULL,
    0x2068080051921000ULL,
    0x62880a0220200808ULL,
    0x4042004000000ULL,
    0x100822020200011ULL,
    0xc00444222012000aULL,
    0x28808801216001ULL,
    0x400492088408100ULL,
    0x201c401040c0084ULL,
    0x840800910a0010ULL,
    0x82080240060ULL,
    0x2000840504006000ULL,
    0x30010c4108405004ULL,
    0x1008005410080802ULL,
    0x8144042209100900ULL,
    0x208081020014400ULL,
    0x4800201208ca00ULL,
    0xf18140408012008ULL,
    0x1004002802102001ULL,
    0x841000820080811ULL,
    0x40200200a42008ULL,
    0x800054042000ULL,
    0x88010400410c9000ULL,
    0x520040470104290ULL,
    0x1004040051500081ULL,
    0x2002081833080021ULL,
    0x400c00c010142ULL,
    0x941408200c002000ULL,
    0x658810000806011ULL,
    0x188071040440a00ULL,
    0x4800404002011c00ULL,
    0x104442040404200ULL,
    0x511080202091021ULL,
    0x4022401120400ULL,
    0x80c0040400080120ULL,
    0x8040010040820802ULL,
    0x480810700020090ULL,
    0x102008e00040242ULL,
    0x809005202050100ULL,
    0x8002024220104080ULL,
    0x431008804142000ULL,
    0x19001802081400ULL,
    0x200014208040080ULL,
    0x3308082008200100ULL,
    0x41010500040c020ULL,
    0x4012020c04210308ULL,
    0x208220a202004080ULL,
    0x111040120082000ULL,
    0x6803040141280a00ULL,
    0x2101004202410000ULL,
    0x8200000041108022ULL,
    0x21082088000ULL,
    0x2410204010040ULL,
    0x40100400809000ULL,
    0x822088220820214ULL,
    0x40808090012004ULL,
    0x910224040218c9ULL,
    0x402814422015008ULL,
    0x90014004842410ULL,
    0x1000042304105ULL,
    0x10008830412a00ULL,
    0x2520081090008908ULL,
    0x40102000a0a60140ULL,
};

const int kRookShiftBits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12};

const int kBishopShiftBits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6};

uint64_t getBlockers(int index, Bitboard mask)
{
    uint64_t blockers = 0;
    int bits = mask.count();
    for (int i = 0; i < bits; i++) {
        int bit = mask.popLSB();
        if (index & (1 << i)) {
            blockers |= (1ULL << bit);
        }
    }

    return blockers;
}

// precompute magic bitboards for bishops and rooks
void initSlidingMoves()
{
    for (int square = 0; square < 64; square++) {
        for (int i = 0; i < (1 << kBishopShiftBits[square]); i++) {
            uint64_t blockers = getBlockers(i, kBishopMasks[square]);
            uint64_t key = (blockers * kBishopMagicNumbers[square]) >> (64 - kBishopShiftBits[square]);
            kBishopTable[square][key] = generateBishopMovesSlow(Square(square), Bitboard(blockers));
        }
    }

    for (int square = 0; square < 64; square++) {
        for (int i = 0; i < (1 << kRookShiftBits[square]); i++) {
            uint64_t blockers = getBlockers(i, kRookMasks[square]);
            uint64_t key = (blockers * kRookMagicNumbers[square]) >> (64 - kRookShiftBits[square]);
            kRookTable[square][key] = generateRookMovesSlow(Square(square), Bitboard(blockers));
        }
    }
}

Bitboard generateWhitePawnMoves(Square from, Bitboard all_pieces, Bitboard capture_pieces)
{
    Bitboard from_mask = Bitboard(1ULL << from.square_);

    Bitboard one_step_moves = (from_mask << 8) & ~all_pieces;
    Bitboard two_step_moves = ((one_step_moves & (0xFFULL << 16)) << 8) & ~all_pieces;
    Bitboard capture_moves = kWhitePawnCaptures[from.square_] & capture_pieces;

    return (one_step_moves | two_step_moves | capture_moves);
}

Bitboard generateBlackPawnMoves(Square from, Bitboard all_pieces, Bitboard capture_pieces)
{
    Bitboard from_mask = Bitboard(1ULL << from.square_);

    Bitboard one_step_moves = (from_mask >> 8) & ~all_pieces;
    Bitboard two_step_moves = ((one_step_moves & (0xFFULL << 40)) >> 8) & ~all_pieces;
    Bitboard capture_moves = kBlackPawnCaptures[from.square_] & capture_pieces;

    return (one_step_moves | two_step_moves | capture_moves);
}

Bitboard generateKnightMoves(Square from)
{
    return kKnightAttacks[from.square_];
}

Bitboard generateKingMoves(Square from)
{
    return kKingAttacks[from.square_];
}

Bitboard generateBishopMovesSlow(Square from, Bitboard all_pieces)
{
    int file = from.file_;
    int rank = from.rank_;

    Bitboard moves(0);

    for (int i = rank + 1, j = file + 1; i <= 7 && j <= 7; i++, j++) {
        moves |= (1ULL << (8 * i + j));
        if (all_pieces.bitboard_ & 1ULL << (8 * i + j)) break;
    }
    for (int i = rank - 1, j = file + 1; i >= 0 && j <= 7; i--, j++) {
        moves |= (1ULL << (8 * i + j));
        if (all_pieces.bitboard_ & 1ULL << (8 * i + j)) break;
    }
    for (int i = rank + 1, j = file - 1; i <= 7 && j >= 0; i++, j--) {
        moves |= (1ULL << (8 * i + j));
        if (all_pieces.bitboard_ & 1ULL << (8 * i + j)) break;
    }
    for (int i = rank - 1, j = file - 1; i >= 0 && j >= 0; i--, j--) {
        moves |= (1ULL << (8 * i + j));
        if (all_pieces.bitboard_ & 1ULL << (8 * i + j)) break;
    }

    return moves;
}

Bitboard generateRookMovesSlow(Square from, Bitboard all_pieces)
{
    int file = from.file_;
    int rank = from.rank_;

    Bitboard moves(0);

    for (int i = rank + 1; i < 8; i++) {
        moves |= (1ULL << (8 * i + file));
        if (all_pieces.bitboard_ & 1ULL << (8 * i + file)) break;
    }
    for (int i = rank - 1; i >= 0; i--) {
        moves |= (1ULL << (8 * i + file));
        if (all_pieces.bitboard_ & 1ULL << (8 * i + file)) break;
    }
    for (int i = file + 1; i < 8; i++) {
        moves |= (1ULL << (8 * rank + i));
        if (all_pieces.bitboard_ & 1ULL << (8 * rank + i)) break;
    }
    for (int i = file - 1; i >= 0; i--) {
        moves |= (1ULL << (8 * rank + i));
        if (all_pieces.bitboard_ & 1ULL << (8 * rank + i)) break;
    }

    return moves;
}

Bitboard generateBishopMoves(Square from, Bitboard all_pieces)
{
    uint64_t blockers = all_pieces.bitboard_ & kBishopMasks[from.square_].bitboard_;
    uint64_t key = (blockers * kBishopMagicNumbers[from.square_]) >> (64 - kBishopShiftBits[from.square_]);
    return kBishopTable[from.square_][key];
}

Bitboard generateRookMoves(Square from, Bitboard all_pieces)
{
    uint64_t blockers = all_pieces.bitboard_ & kRookMasks[from.square_].bitboard_;
    uint64_t key = (blockers * kRookMagicNumbers[from.square_]) >> (64 - kRookShiftBits[from.square_]);
    return kRookTable[from.square_][key];
}

Bitboard generateQueenMoves(Square from, Bitboard all_pieces)
{
    return generateRookMoves(from, all_pieces) | generateBishopMoves(from, all_pieces);
}

} // namespace minizero::env::chess
