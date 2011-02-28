/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2010 Marco Costalba, Joona Kiiski, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


////
//// Includes
////

#include "bitboard.h"

Bitboard EmptyBoardBB = 0;


const U64 BMult[64] = {
  0x440049104032280ULL, 0x1021023c82008040ULL, 0x404040082000048ULL,
  0x48c4440084048090ULL, 0x2801104026490000ULL, 0x4100880442040800ULL,
  0x181011002e06040ULL, 0x9101004104200e00ULL, 0x1240848848310401ULL,
  0x2000142828050024ULL, 0x1004024d5000ULL, 0x102044400800200ULL,
  0x8108108820112000ULL, 0xa880818210c00046ULL, 0x4008008801082000ULL,
  0x60882404049400ULL, 0x104402004240810ULL, 0xa002084250200ULL,
  0x100b0880801100ULL, 0x4080201220101ULL, 0x44008080a00000ULL,
  0x202200842000ULL, 0x5006004882d00808ULL, 0x200045080802ULL,
  0x86100020200601ULL, 0xa802080a20112c02ULL, 0x80411218080900ULL,
  0x200a0880080a0ULL, 0x9a01010000104000ULL, 0x28008003100080ULL,
  0x211021004480417ULL, 0x401004188220806ULL, 0x825051400c2006ULL,
  0x140c0210943000ULL, 0x242800300080ULL, 0xc2208120080200ULL,
  0x2430008200002200ULL, 0x1010100112008040ULL, 0x8141050100020842ULL,
  0x822081014405ULL, 0x800c049e40400804ULL, 0x4a0404028a000820ULL,
  0x22060201041200ULL, 0x360904200840801ULL, 0x881a08208800400ULL,
  0x60202c00400420ULL, 0x1204440086061400ULL, 0x8184042804040ULL,
  0x64040315300400ULL, 0xc01008801090a00ULL, 0x808010401140c00ULL,
  0x4004830c2020040ULL, 0x80005002020054ULL, 0x40000c14481a0490ULL,
  0x10500101042048ULL, 0x1010100200424000ULL, 0x640901901040ULL,
  0xa0201014840ULL, 0x840082aa011002ULL, 0x10010840084240aULL,
  0x420400810420608ULL, 0x8d40230408102100ULL, 0x4a00200612222409ULL,
  0xa08520292120600ULL
};

const U64 RMult[64] = {
  0xa8002c000108020ULL, 0x4440200140003000ULL, 0x8080200010011880ULL,
  0x380180080141000ULL, 0x1a00060008211044ULL, 0x410001000a0c0008ULL,
  0x9500060004008100ULL, 0x100024284a20700ULL, 0x802140008000ULL,
  0x80c01002a00840ULL, 0x402004282011020ULL, 0x9862000820420050ULL,
  0x1001448011100ULL, 0x6432800200800400ULL, 0x40100010002000cULL,
  0x2800d0010c080ULL, 0x90c0008000803042ULL, 0x4010004000200041ULL,
  0x3010010200040ULL, 0xa40828028001000ULL, 0x123010008000430ULL,
  0x24008004020080ULL, 0x60040001104802ULL, 0x582200028400d1ULL,
  0x4000802080044000ULL, 0x408208200420308ULL, 0x610038080102000ULL,
  0x3601000900100020ULL, 0x80080040180ULL, 0xc2020080040080ULL,
  0x80084400100102ULL, 0x4022408200014401ULL, 0x40052040800082ULL,
  0xb08200280804000ULL, 0x8a80a008801000ULL, 0x4000480080801000ULL,
  0x911808800801401ULL, 0x822a003002001894ULL, 0x401068091400108aULL,
  0x4a10a00004cULL, 0x2000800640008024ULL, 0x1486408102020020ULL,
  0x100a000d50041ULL, 0x810050020b0020ULL, 0x204000800808004ULL,
  0x20048100a000cULL, 0x112000831020004ULL, 0x9000040810002ULL,
  0x440490200208200ULL, 0x8910401000200040ULL, 0x6404200050008480ULL,
  0x4b824a2010010100ULL, 0x4080801810c0080ULL, 0x400802a0080ULL,
  0x8224080110026400ULL, 0x40002c4104088200ULL, 0x1002100104a0282ULL,
  0x1208400811048021ULL, 0x3201014a40d02001ULL, 0x5100019200501ULL,
  0x101000208001005ULL, 0x2008450080702ULL, 0x1002080301d00cULL,
  0x410201ce5c030092ULL
};

const int BShift[64] = {
  58, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 59, 59,
  59, 59, 57, 57, 57, 57, 59, 59, 59, 59, 57, 55, 55, 57, 59, 59,
  59, 59, 57, 55, 55, 57, 59, 59, 59, 59, 57, 57, 57, 57, 59, 59,
  59, 59, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 58
};

const int RShift[64] = {
  52, 53, 53, 53, 53, 53, 53, 52, 53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53, 52, 53, 53, 53, 53, 53, 53, 52
};




Bitboard RMask[64];
int RAttackIndex[64];
Bitboard RAttacks[0x19000];

Bitboard BMask[64];
int BAttackIndex[64];
Bitboard BAttacks[0x1480];

Bitboard SetMaskBB[65];
Bitboard ClearMaskBB[65];

void init_masks();
Bitboard sliding_attacks(int sq, Bitboard block, int dirs, int deltas[][2],
                           int fmin, int fmax, int rmin, int rmax);
void init_sliding_attacks(Bitboard attacks[], int attackIndex[], Bitboard mask[],
                            const int shift[], const Bitboard mult[], int deltas[][2]);


////
//// Functions
////

/// init_bitboards() initializes various bitboard arrays.  It is called during
/// program initialization.

void init_bitboards() {

  int rookDeltas[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};
  int bishopDeltas[4][2] = {{1,1},{-1,1},{1,-1},{-1,-1}};

  init_masks();
  init_sliding_attacks(RAttacks, RAttackIndex, RMask, RShift, RMult, rookDeltas);
  init_sliding_attacks(BAttacks, BAttackIndex, BMask, BShift, BMult, bishopDeltas);
}


/// first_1() finds the least significant nonzero bit in a nonzero bitboard.
/// pop_1st_bit() finds and clears the least significant nonzero bit in a
/// nonzero bitboard.

const int BitTable[64] = {
  0, 1, 2, 7, 3, 13, 8, 19, 4, 25, 14, 28, 9, 34, 20, 40, 5, 17, 26, 38, 15,
  46, 29, 48, 10, 31, 35, 54, 21, 50, 41, 57, 63, 6, 12, 18, 24, 27, 33, 39,
  16, 37, 45, 47, 30, 53, 49, 56, 62, 11, 23, 32, 36, 44, 52, 55, 61, 22, 43,
  51, 60, 42, 59, 58
};

Square first_1(Bitboard b) {
  return (Square)(BitTable[((b & -b) * 0x218a392cd3d5dbfULL) >> 58]);
}

Square pop_1st_bit(Bitboard* b) {
  Bitboard bb = *b;
  *b &= (*b - 1);
  return (Square)(BitTable[((bb & -bb) * 0x218a392cd3d5dbfULL) >> 58]);
}



/* All functions below are used to precompute various bitboards during
 program initialization.  Some of the functions may be difficult to
 understand, but they all seem to work correctly, and it should never
 be necessary to touch any of them. */

void init_masks() {

    SetMaskBB[64] = 0ULL;
    ClearMaskBB[64] = ~SetMaskBB[64];

    for (Square s = 0; s <= 63; s++)
    {
        SetMaskBB[s] = (1ULL << s);
        ClearMaskBB[s] = ~SetMaskBB[s];
    }
}

Bitboard sliding_attacks(int sq, Bitboard block, int dirs, int deltas[][2], int fmin, int fmax, int rmin, int rmax) {
    Bitboard result = 0ULL;
    int rk = sq / 8;
    int fl = sq % 8;

    for (int i = 0; i < dirs; i++)
    {
        int dx = deltas[i][0];
        int dy = deltas[i][1];
        int f = fl + dx;
        int r = rk + dy;

        while (   (dx == 0 || (f >= fmin && f <= fmax))
               && (dy == 0 || (r >= rmin && r <= rmax)))
        {
            result |= (1ULL << (f + r*8));
            if (block & (1ULL << (f + r*8)))
                break;

            f += dx;
            r += dy;
        }
    }
    return result;
}

Bitboard index_to_bitboard(int index, Bitboard mask) {

    Bitboard result = 0ULL;
    int bits = count_1s(mask);

    for (int i = 0; i < bits; i++)
    {
        int j = pop_1st_bit(&mask);
        if (index & (1 << i))
            result |= (1ULL << j);
    }
    return result;
}


void init_sliding_attacks(Bitboard attacks[], int attackIndex[], Bitboard mask[], const int shift[], const Bitboard mult[], int deltas[][2]) {

    for (int i = 0, index = 0; i < 64; i++)
    {
        attackIndex[i] = index;
        mask[i] = sliding_attacks(i, 0, 4, deltas, 1, 6, 1, 6);

        int j = (1 << (64 - shift[i]));

        for (int k = 0; k < j; k++)
        {
            Bitboard b = index_to_bitboard(k, mask[i]);
            attacks[index + ((b * mult[i]) >> shift[i])] = sliding_attacks(i, b, 4, deltas,0,7,0,7);
        }
        index += j;
    }
}

