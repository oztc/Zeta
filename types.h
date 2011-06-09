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

typedef signed short Score;
typedef unsigned int Move;
typedef unsigned char Square;
typedef unsigned char Piece;

#define BLACK 8
#define WHITE 16

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define FLIPFLOP(square)    (((square)^7)^56)
#define SWITCH(stm) (((stm)==WHITE) ? (BLACK) : (WHITE) )

const Piece PEMPTY  = 0;
const Piece WPAWN   = 1;
const Piece BPAWN   = 2;
const Piece KNIGHT  = 3;
const Piece KING    = 4;
const Piece BISHOP  = 5;
const Piece ROOK    = 6;
const Piece QUEEN   = 7;


#endif
