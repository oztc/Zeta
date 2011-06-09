/*
    Zeta CL, OpenCL Chess Engine
    Author: Srdja Matovic <srdja@matovic.de>
    Created at: 20-Jan-2011
    Updated at:
    Description: A Chess Engine written in OpenCL, a language suited for GPUs.

    Copyright (C) 2011 Srdja Matovic
    This program is distributed under the GNU General Public License.
    See file COPYING or http://www.gnu.org/licenses/
*/

typedef signed short Score;
typedef unsigned int Move;
typedef unsigned char Square;
typedef char Piece;


// Move Generation from MicroMax
const int o[] = 
{
    -16,-15,17,0,			        /* downstream pawn 	  */
    16,15,17,0,			            /* upstream pawn 	  */
    1,-1,16,-16,0,			        /* rook 		  */
    1,-1,16,-16,15,-15,17,-17,0,	/* king, queen and bishop */
    14,-14,18,-18,31,-31,33,-33,0,	/* knight 		  */
    -1,3,21,12,16,7,12		        /* directory 		  */
};


const Piece values[]={0,1,1,3,-1,3,5,9};


const Piece WPAWN   = 1;
const Piece BPAWN   = 2;
const Piece KNIGHT  = 3;
const Piece KING    = 4;
const Piece BISHOP  = 5;
const Piece ROOK    = 6;
const Piece QUEEN   = 7;


__kernel void negamax_gpu(  __global Piece *board,
                                    unsigned int som,
                                    Move Lastmove,
                            __global Move *bestmove
                        )

{

    bestmove[0] = 2;
}
