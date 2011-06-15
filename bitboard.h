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


#if !defined(BITBOARD_H_INCLUDED)
#define BITBOARD_H_INCLUDED

#include "types.h"

extern Bitboard EmptyBoardBB;

extern Bitboard SetMaskBB[65];
extern Bitboard ClearMaskBB[65];

extern const U64 RMult[64];
extern const int RShift[64];
extern Bitboard RMask[64];
extern int RAttackIndex[64];
extern Bitboard RAttacks[0x19000];

extern const U64 BMult[64];
extern const int BShift[64];
extern Bitboard BMask[64];
extern int BAttackIndex[64];
extern Bitboard BAttacks[0x1480];

/* Inline functions */

static inline Square make_square(int f, int r) {
  return ( f |  (r << 3));
}

static inline int square_file(Square s) {
  return (s & 7);
}

static inline int square_rank(Square s) {
  return (s >> 3);
}


/* Functions for testing whether a given bit is set in a bitboard, and for
setting and clearing bits. */

static inline Bitboard bit_is_set(Bitboard b, Square s) {
  return b & SetMaskBB[s];
}

static inline void set_bit(Bitboard *b, Square s) {
  *b |= SetMaskBB[s];
}

static inline void clear_bit(Bitboard *b, Square s) {
  *b &= ClearMaskBB[s];
}


/* Functions used to update a bitboard after a move. This is faster
then calling a sequence of clear_bit() + set_bit() */

static inline Bitboard make_move_bb(Square from, Square to) {
  return SetMaskBB[from] | SetMaskBB[to];
}

static inline void do_move_bb(Bitboard *b, Bitboard move_bb) {
  *b ^= move_bb;
}



/* Functions for computing sliding attack bitboards. rook_attacks_bb(),
 bishop_attacks_bb() and queen_attacks_bb() all take a square and a
bitboard of occupied squares as input, and return a bitboard representing
all squares attacked by a rook, bishop or queen on the given square. */


static inline Bitboard rook_attacks_bb(Square s, Bitboard blockers) {
  Bitboard b = blockers & RMask[s];
  return RAttacks[RAttackIndex[s] + ((b * RMult[s]) >> RShift[s])];
}

static inline Bitboard bishop_attacks_bb(Square s, Bitboard blockers) {
  Bitboard b = blockers & BMask[s];
  return BAttacks[BAttackIndex[s] + ((b * BMult[s]) >> BShift[s])];
}

static inline Bitboard queen_attacks_bb(Square s, Bitboard blockers) {
  return rook_attacks_bb(s, blockers) | bishop_attacks_bb(s, blockers);
}

/* first_1() finds the least significant nonzero bit in a nonzero bitboard.
 pop_1st_bit() finds and clears the least significant nonzero bit in a
 nonzero bitboard. */

static inline int count_1s(Bitboard b) {
  b -= ((b>>1) & 0x5555555555555555ULL);
  b = ((b>>2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
  b = ((b>>4) + b) & 0x0F0F0F0F0F0F0F0FULL;
  b *= 0x0101010101010101ULL;
  return (int)(b >> 56);
}

extern Square first_1(Bitboard b);
extern Square pop_1st_bit(Bitboard* b);
extern void init_bitboards();


#endif
