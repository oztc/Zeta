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
typedef unsigned char Square;
typedef unsigned char Piece;
typedef unsigned long U64;

typedef U64 Move;
typedef U64 Bitboard;
typedef U64 Hash;

#define WHITE 0
#define BLACK 1

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define FLIPFLOP(square)    (((square)^7)^56)

/*
__constant Score values[]={0,100,100,300,0,300,500,900};


__constant Piece PEMPTY  = 0;
__constant Piece PAWN    = 1;
__constant Piece KNIGHT  = 2;
__constant Piece BISHOP  = 3;
__constant Piece ROOK    = 4;
__constant Piece QUEEN   = 5;
__constant Piece KING    = 6;
*/

Piece getPiece (__local Bitboard *board, Square sq) {
   return ((board[0] >> sq) & 1)
      + 2*((board[1] >> sq) & 1)
      + 4*((board[2] >> sq) & 1)
      + 8*((board[3] >> sq) & 1);
}


__kernel void negamax_gpu(  __global Bitboard *globalboard,
                            __global Move *globalmoves,
                                    unsigned int som,
                                    unsigned int max_depth,
                                    Move Lastmove,
                            __global Move *bestmove,
                            __global long *NODECOUNT,
                            __global long *MOVECOUNT,
                            __global Bitboard *SetMaskBB,
                            __global Bitboard *ClearMaskBB,
                            __global Bitboard *AttackTables,
                            __global Bitboard *PawnAttackTables,
                            __global Bitboard *OutputBB,
                            __global Bitboard *avoidWrap,
                            __global signed int *shift,
                            __global int *BitTable
                        )

{

    __local Bitboard board[128*4];
    Move move = 0;
    int pidx = get_global_id(0);
    char pidy = get_local_id(1);
    Square pos;
    Square to;   
    Piece piece;
    Piece piececpt;
    char movecounter = 0;
    int  nodecounter = 0;
    char search_depth = 0;
    Bitboard bbTemp = 0;
    Bitboard bbWork = 0;
    Bitboard bbOpposite = 0;
    Bitboard bbBlockers = 0;
    Bitboard bbMoves = 0;
    event_t event = (event_t)0;


    for (nodecounter = 0; nodecounter < 10000; nodecounter++) {

        event = async_work_group_copy((__local Bitboard*)&board[(pidy*4)], (const __global Bitboard* )&globalboard[(search_depth*128*128)+(pidx*128)+pidy], (size_t)4, (event_t)0);


        // #########################################
        // #### Kogge Stone like Move Generator ####
        // #########################################
        movecounter = 0;

        bbWork      = (som)? ( board[(pidy*4)+0] )                                      : (board[(pidy*4)+0] ^ (board[1] | board[(pidy*4)+2] | board[(pidy*4)+3]));
        bbOpposite  = (som)? ( board[(pidy*4)+0] ^ (board[(pidy*4)+1] | board[(pidy*4)+2] | board[(pidy*4)+3]))    : (board[(pidy*4)+0]);
        bbBlockers  = (bbWork | bbOpposite);

        while(bbWork) {
            // pop 1st bit
            pos = ((Square)(BitTable[((bbWork & -bbWork) * 0x218a392cd3d5dbf) >> 58]) );
            bbWork &= (bbWork-1); 

            piece = getPiece(board, pos);

            // Knight and King
            bbTemp = !((piece>>1)&4)? AttackTables[(som*7*64)+((piece>>1)*64)+pos] : 0x00;

            // Sliders
            bbTemp = !((piece>>1)&4)? AttackTables[(som*7*64)+((piece>>1)*64)+pos] : 0x00;
    

            // Pawn attacks
            bbTemp  |= ( (piece>>1)==1) ? (PawnAttackTables[som*64+pos] & bbOpposite)   : 0 ;

            // White Pawn forward step
            bbTemp  |= ((piece>>1)==1 && !(search_depth >= max_depth) && !som) ? (PawnAttackTables[2*64+pos]&(~bbBlockers & SetMaskBB[pos+8]))            : 0 ;
            // White Pawn double square
            bbTemp  |= ((piece>>1)==1 && !(search_depth >= max_depth) && !som && ((pos&56)/8 == 1 ) && (~bbBlockers & SetMaskBB[pos+8]) && (~bbBlockers & SetMaskBB[pos+16]) ) ? SetMaskBB[pos+16] : 0;
            // Black Pawn forward step
            bbTemp  |=  ((piece>>1)==1 && !(search_depth >= max_depth) && som) ? (PawnAttackTables[3*64+pos]&(~bbBlockers & SetMaskBB[pos-8]))            : 0 ;
            // Black Pawn double square
            bbTemp  |= ((piece>>1)==1 && !(search_depth >= max_depth) &&  som && ((pos&56)/8 == 6 ) && (~bbBlockers & SetMaskBB[pos-8]) && (~bbBlockers & SetMaskBB[pos-16]) ) ? SetMaskBB[pos-16] : 0 ;

            // Captures
            bbMoves = bbTemp&bbOpposite;            
            // Non Captures
            bbMoves |= ((search_depth >= max_depth))? 0x00 : (bbTemp&~bbBlockers);


            while(bbMoves) {
                // pop 1st bit
                to = ((Square)(BitTable[((bbMoves & -bbMoves) * 0x218a392cd3d5dbf) >> 58]) );
                bbMoves &= (bbMoves-1);

                //cpt = to;        // TODO: en passant
                //pieceto = piece; // TODO: Pawn promotion

                piececpt = getPiece(board, to);

                // make move and stire in global
                move = ((Move)pos | (Move)to<<6 | (Move)to<<12 | (Move)piece<<18 | (Move)piece<<22 | (Move)piececpt<<26 );

//                globalmoves[(search_depth*128*128)+(pidy*128)+movecounter] = move;
                movecounter++;

            }
        }

        // ################################
        // #### TODO: legal moves only  ###
        // ################################

        // ######################################
        // #### TODO: sort moves in parallel  ###
        // ######################################

    }

    if (pidx == 0 && pidy == 0) {
//        *bestmove = globalmoves[(search_depth*128*128)+(pidy*128)+movecounter-1];
        *bestmove = move;
        *MOVECOUNT = movecounter;
        *NODECOUNT = nodecounter;
    }
}


