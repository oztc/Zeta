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


__kernel void negamax_gpu(  __global Piece *globalboard,
                            __global Move *globalmoves,
                                    unsigned int som,
                                    unsigned int max_depth,
                                    Move Lastmove,
                            __global Move *bestmove,
                            __global long *NODECOUNT,
                            __global long *MOVECOUNT
                        )

{

    __local Piece board[129];
    Move move = 0;
    int pid = get_global_id(0);
    int boardindex = 0;
    int moveindex = 0;
    int x = 0;
    int y = 0;
    int r = 0;
    int j = 0;
    int i = 0;
    int kingpos = 0;
    int kic = 0;
    Piece u = PEMPTY;
    Piece p = PEMPTY;
    Piece t = PEMPTY;
    Piece promo = PEMPTY;
    Piece check_p = PEMPTY;
    Piece check_t = PEMPTY;
    int check_y = 0;
    int check_r = 0;
    int check_j = 0;
    int movecounter = 0;
    int search_depth = 0;
    event_t event = (event_t)0;


    boardindex = (search_depth*pid*129);
    moveindex = (search_depth*256)+pid;

    event = async_work_group_copy((__local Piece*)board, (const __global Piece* )&globalboard[boardindex], (size_t)129, (event_t)0);
    move = globalmoves[moveindex];

    search_depth++;


    // ##################################################################
    // ### Move Generator from MicroMax, TODO: En Passant and Castles ###
    // ##################################################################
    movecounter = 0;
    x = 0;
    y = 0;
    do {
        u = board[x];
        if (!(u&som)) continue; // only our pieces
        p = u&7;
        j = o[p+30];
        while(r=o[++j]) {
            y = x;
            do {
                y+= r;
                if (y&0x88) break; // out of board
                t = board[y];
                if (t&som) break; // our own piece
                if ( p<3 && (j==0 || j== 4) && t != PEMPTY) break;                  // pawn one step
                if (p == BPAWN && (r == -15 || r == -17) && t == PEMPTY ) break;    // pawn attack black
                if (p == WPAWN && (r == 15 || r == 17) && t == PEMPTY) break;       // pawn attack white

                promo = (p<3 && (y >=114 || y <= 7) ) ? (QUEEN|som):0;              // pawn promo piece

                // ##################################################
                // ### kingincheck? TODO: store king pos in board ###
                // ##################################################
                kic = 0;
                // domove
                board[y] = u;        
                board[x] = PEMPTY;                

                i=0;
                do {
                    if (board[i] == (KING|som) ) {
                        kingpos = i;
                        break;
                    }
                }while(i=i+9&~0x88);

                for (check_p = KNIGHT; check_p <= QUEEN; check_p++) {
                    // TODO: Pawn checks
                    check_j = o[check_p+30];
                
                    while(check_r=o[++check_j]) {
                        check_y = kingpos;
                        do {
                            check_y+= check_r;
                            if (check_y&0x88) break;
                            check_t = board[check_y];
                            if (check_t&som) break;
                            if (check_t == (check_p|(SWITCH(som))) ) {
                                kic = 1;
                                break;
                            }
                        	check_t+= check_p<5;
                        }while(!check_t);
                        if (kic) break;
                    }
                    if (kic) break;
                }

                // valid board, copy move to global
                if (!kic) {
                    moveindex = (search_depth*256*256)+(pid*256) + movecounter;
                    globalmoves[moveindex] = (x | (Move)y<<8 | (Move)t<<16 | (Move)promo<<24);;
                    movecounter++;
                }    
                // undomove
                board[y] = t;        
                board[x] = u;
                // ##################################################
                // ### kingincheck? TODO: store king pos in board ###
                // ##################################################

            	t+= p<5;    // make sure t!=0 for crawling pieces
                t = (p<3 && ((x>= 16 && x<=23) || (x>=96 && x <=113) ) && (abs(x-y)==16)) ? 0 : t; // pawn double square, TODO: simplify

            }while(!t);	
        }
    }while(x=x+9&~0x88);
    // ##################################################################
    // ### Move Generator from MicroMax, TODO: En Passant and Castles ###
    // ##################################################################


    if (pid == 0 ) {
        bestmove[0] = globalmoves[moveindex];
        MOVECOUNT[0] = movecounter;
        NODECOUNT[0] = 1;
    }
}

















