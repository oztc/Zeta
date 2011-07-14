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

typedef signed int Score;
typedef unsigned short MoveScore;
typedef unsigned char Cr;
typedef unsigned char Square;
typedef unsigned char Piece;

// max internal search depth of GPU
#define max_depth   40


#define WHITE 0
#define BLACK 1

#define true          1
#define false         0
#define bool char

#define FLIPFLOP(square)    (((square)^7)^56)

#define PEMPTY  0
#define PAWN    1
#define KNIGHT  2
#define KING    3
#define BISHOP  4
#define ROOK    5
#define QUEEN   6




#endif
