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

/* 
    Commercial Developer License available from srdja@matovic.de 
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

// TODO: inline faster?, parallilzing possible?
Square pop_1st_bit(Bitboard* b, __global int *BitTable) {
  Bitboard bb = *b;
  *b &= (*b - 1);
  return (Square)(BitTable[((bb & -bb) * 0x218a392cd3d5dbf) >> 58]);
}

// TODO: in parallel pleaze
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

    __local Bitboard board[4];
    __local Bitboard bbMove[8];
    Move move = 0;
    int pidw = get_global_id(0);
    int pidx = get_global_id(1);
    int pidy = get_local_id(2);
    int pidz = pidy%2 + (int)pidy/2;
    int loop = 0;
    char kic = 0;
    char qs = 0;
    Square pos;
    Square to;   
    Piece piece;
    Piece piececpt;
    U64 movecounter = 0;
    U64 nodecounter = 0;
    char search_depth = 0;
    signed char r;
    Bitboard bbWork = 0;
    Bitboard bbOpposite = 0;
    Bitboard bbBlockers = 0;
    Bitboard bbMoves = 0;
    Bitboard bbCaptures = 0;
    Bitboard bbNonCaptures = 0;
    Bitboard gen = 0;
    Bitboard pro = 0;
    event_t event = (event_t)0;


    for (nodecounter = 0; nodecounter < 10000; nodecounter++) {

        event = async_work_group_copy((__local Bitboard*)board, (const __global Bitboard* )&globalboard[(search_depth*0*4)], (size_t)4, (event_t)0);


        // #########################################
        // #### Kogge Stone like Move Generator ####
        // #########################################
        movecounter = 0;

        bbWork      = (som)? ( board[0] )                                      : (board[0] ^ (board[1] | board[2] | board[3]));
        bbOpposite  = (som)? ( board[0] ^ (board[1] | board[2] | board[3]))    : (board[0]);
        bbBlockers  = (bbWork | bbOpposite);

        while(bbWork) {
            pos = pop_1st_bit(&bbWork, BitTable);
            piece = getPiece(board, pos);
            bbMoves = 0;
            bbMove[pidy] = 0;

            pro = ~bbBlockers;
            gen = SetMaskBB[pos];
            r = shift[(piece>>1)*8+pidy];
            pro &= avoidWrap[(piece>>1)*8+pidy];

            // do kogge stone for all 8 directions in parallel
            gen |= pro & ((gen << r) | (gen >> (64-r)));
            pro &=       ((pro << r) | (pro >> (64-r)));
            gen |= pro & ((gen << 2*r) | (gen >> (64-2*r)));
            pro &=       ((pro << 2*r) | (pro >> (64-2*r)));
            gen |= pro & ((gen << 4*r) | (gen >> (64-4*r)));

            // Shift one for Captures
            bbMove[pidy] = ((gen << r) | (gen >> (64-r))) & avoidWrap[(piece>>1)*8+pidy];

            // collect parallel moves
            bbMoves = (bbMove[0] | bbMove[1] | bbMove[2] | bbMove[3] | bbMove[4] | bbMove[5] | bbMove[6] | bbMove[7]);

            // Captures, considering Pawn Attacks
            bbCaptures = ((piece>>1) == 1) ? (bbMoves & bbOpposite & PawnAttackTables[som*64+pos])         :  bbMoves & bbOpposite;

            // Non Captures, considering Pawn Attacks
            bbNonCaptures = ((piece>>1) == 1) ? ( bbMoves & ~ PawnAttackTables[som*64+pos] & ~bbBlockers)    :  bbMoves & ~bbBlockers;

            // Quiscence Search?
            bbMoves = (qs)? (bbCaptures) : (bbCaptures | bbNonCaptures);

            // dirty but simple, considering non sliders and not allowed multible shifts
            bbMoves &= AttackTables[(som*7*64)+((piece>>1)*64)+pos];


            // TODO: think about parallizing this while with 8 threads
            while(bbMoves) {
                to = pop_1st_bit(&bbMoves, BitTable);
//                cpt = to;        // TODO: en passant
//                pieceto = piece; // TODO: Pawn promotion

                piececpt = getPiece(board, to);

                // make move and stire in global
                move = ((Move)pos | (Move)to<<6 | (Move)to<<12 | (Move)piece<<18 | (Move)piece<<22 | (Move)piececpt<<26 );

                globalmoves[(search_depth*256*256)+(pidx*256)+movecounter] = move;
                movecounter++;

            }
        }

        // ##########################
        // #### legal moves only  ###
        // ##########################
/*
        for (movecounter; movecounter >= 0; movecounter--) {
            move = globalmoves[movecounter];

            // domove in parallel
            // unset from and unset cpt
            board[pidz] &= (pidy%2)? ClearMaskBB[pos] : ClearMaskBB[cpt];
            // unset to
            board[pidz] &= (pidy%2)? ClearMaskBB[to]  : 0xFFFFFFFFFFFFFFFF;
            // set to
            board[pidz] |= (pidy%2)? (Bitboard)((piece>>((int)pidy/2))&1)<<to  : 0x00;

            



            // undomove in parallel
            // unset to
            board[pidz] &= (pidy%2)? ClearMaskBB[to]  : 0xFFFFFFFFFFFFFFFF;
            // restore cpt  and restore from
            board[pidz] |= (pidy%2)? (Bitboard)((piececpt>>((int)pidy/2))&1)<<cpt  : (Bitboard)((piece>>((int)pidy/2))&1)<<pos;


        }
*/
            // ######################################
            // #### TODO: sort moves in parallel  ###
            // ######################################
    }

    if (pidx == 0 && pidy == 0 ) {
        *bestmove = globalmoves[(search_depth*256*256)+(pidx*256)+movecounter-1];
        *MOVECOUNT = movecounter;
        *NODECOUNT = nodecounter;
    }
}


