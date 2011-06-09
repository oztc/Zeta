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
typedef unsigned char Piece;

#define BLACK 8
#define WHITE 16

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define FLIPFLOP(square)    (((square)^7)^56)
#define SWITCH(stm) (((stm)==WHITE) ? (BLACK) : (WHITE) )

// Move Generation from MicroMax
const int o[] = 
{
    16,15,17,0,		    	        /* upstream pawn 	  */
    -16,-15,-17,0,			        /* downstream pawn 	  */
    1,-1,16,-16,0,			        /* rook 		  */
    1,-1,16,-16,15,-15,17,-17,0,	/* king, queen and bishop */
    14,-14,18,-18,31,-31,33,-33,0,	/* knight 		  */
    -1,3,21,12,16,7,12		        /* directory 		  */
};


const Piece values[]={0,1,1,3,-1,3,5,9};


const Piece PEMPTY  = 0;
const Piece WPAWN   = 1;
const Piece BPAWN   = 2;
const Piece KNIGHT  = 3;
const Piece KING    = 4;
const Piece BISHOP  = 5;
const Piece ROOK    = 6;
const Piece QUEEN   = 7;


__kernel void negamax_gpu(  __global Piece *board,
                                    unsigned int som,
                                    unsigned int maxdepth,
                                    Move Lastmove,
                            __global Move *bestmove,
                            __global long *NODECOUNT,
                            __global long *MOVECOUNT
                        )

{

    int pid = get_global_id(0);
    int x = 0;
    int y = 0;
    int r = 0;
    int j = 0;
    Piece u = PEMPTY;
    Piece p = PEMPTY;
    Piece t = PEMPTY;
    Move move = 0;
    Move moves[218];
    int movecounter = 0;


    // ##############################################################ä
    // ### Move Generator from MicroMax, wo En Passant and Castles ###
    // ##############################################################ä
    movecounter = 0;
    x = 0;
    y = 0;
    do {
        u = board[x];
        // only our pieces
        if (!(u&som)) continue;
        p = u&7;
        j = o[p+30];
        while(r=o[++j]) {
            y = x;
            do {
                y+= r;
                // out of boad
                if (y&0x88) break;
                t = board[y];
                // our own piece
                if (t&som) break;
                // terminate if pawn and inappropiate mode, TODO: simplify
                if ( p<3 && (j==0 || j== 4) && t != PEMPTY) break;
                if (p == BPAWN && (r == -15 || r == -17) && t == PEMPTY ) break;
                if (p == WPAWN && (r == 15 || r == 17) && t == PEMPTY) break;

                // valid move
                // TODO: kingincheck?
                moves[movecounter] = (x | (Move)y<<8 | (Move)t<<16 | 0);
                movecounter++;

                // make sure t!=0 for crawling pieces
            	t+= p<5;
                // pawn double square, TODO: simplify
                t = (p<3 && ((x>= 16 && x<=23) || (x>=96 && x <=113) ) && (abs(x-y)==16)) ? 0 : t;

            }while(!t);	
        }
    }while(x=x+9&~0x88);


    if (pid == 0 ) {
        bestmove[0] = moves[0];
        MOVECOUNT[0] = movecounter;
        NODECOUNT[0] = 1;
    }
}

















