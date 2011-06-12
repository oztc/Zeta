/* 
    Zeta Dva, yet another amateur level chess engine
    Author: Srdja Matovic <srdja@matovic.de>
    Created at: 15-Jan-2011
    Updated at: 31-Jan-2011
    Description: Amateur chesss programm

    Copyright (C) 2011 Srdja Matovic
    This program is distributed under the GNU General Public License.
    See file COPYING or http://www.gnu.org/licenses/

*/


#if !defined(TYPES_H_INCLUDED)
#define TYPES_H_INCLUDED


#  if defined(_MSC_VER)
typedef unsigned __int64 U64;
#  else
typedef unsigned long long int U64;
#endif

typedef U64 Move;
typedef U64 Bitboard;
typedef U64 Hash;

typedef signed short Score;
typedef unsigned short MoveScore;
typedef unsigned char Cr;
typedef unsigned char Square;
typedef unsigned char Piece;

#define BLACK 1
#define WHITE 0

#define true          1
#define false         0
#define bool char

#define FLIPFLOP(square)    (((square)^7)^56)
#define SWITCH(stm) (((stm)==WHITE) ? (BLACK) : (WHITE) )

const Piece PEMPTY  = 0;
const Piece PAWN    = 1;
const Piece KNIGHT  = 2;
const Piece BISHOP  = 3;
const Piece ROOK    = 4;
const Piece QUEEN   = 5;
const Piece KING    = 6;


#endif
