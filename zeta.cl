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

Square pop_1st_bit(Bitboard* b, __global int *BitTable) {
  Bitboard bb = *b;
  *b &= (*b - 1);
  return (Square)(BitTable[((bb & -bb) * 0x218a392cd3d5dbf) >> 58]);
}


Piece getPiece (__local Bitboard *board, Square sq) {
   return ((board[0] >> sq) & 1)
      + 2*((board[1] >> sq) & 1)
      + 4*((board[2] >> sq) & 1)
      + 8*((board[3] >> sq) & 1);
}


void domove(__local Bitboard *board, Move move, int som, __global Bitboard *SetMaskBB, __global Bitboard *ClearMaskBB) {

    Square from     =  move&0x3F;
    Square to       = (move>>6)&0x3F;
    Square cpt      = (move>>12)&0x3F;

    Piece pfrom     = (move>>18)&0xF;
    Piece pto       = (move>>22)&0xF;
    Piece pcpt      = (move>>26)&0xF;

    // unset from
    board[0] &= ClearMaskBB[from];
    board[1] &= ClearMaskBB[from];
    board[2] &= ClearMaskBB[from];
    board[3] &= ClearMaskBB[from];

    // unset cpt
    if (pcpt != 0) {
        board[0] &= ClearMaskBB[cpt];
        board[1] &= ClearMaskBB[cpt];
        board[2] &= ClearMaskBB[cpt];
        board[3] &= ClearMaskBB[cpt];
    }

    // unset to
    board[0] &= ClearMaskBB[to];
    board[1] &= ClearMaskBB[to];
    board[2] &= ClearMaskBB[to];
    board[3] &= ClearMaskBB[to];

    // set to
    board[0] |= (Bitboard)(pto&1)<<to;
    board[1] |= (Bitboard)((pto>>1)&1)<<to;
    board[2] |= (Bitboard)((pto>>2)&1)<<to;
    board[3] |= (Bitboard)((pto>>3)&1)<<to;


    // TODO: castle moves

}

void undomove(__local Bitboard *board, Move move, int som, __global Bitboard *SetMaskBB, __global Bitboard *ClearMaskBB) {

    Square from     =  move&0x3F;
    Square to       = (move>>6)&0x3F;
    Square cpt      = (move>>12)&0x3F;

    Piece pfrom     = (move>>18)&0xF;
    Piece pto       = (move>>22)&0xF;
    Piece pcpt      = (move>>26)&0xF;

    // unset to
    board[0] &= ClearMaskBB[to];
    board[1] &= ClearMaskBB[to];
    board[2] &= ClearMaskBB[to];
    board[3] &= ClearMaskBB[to];

    // restore cpt
    if (pcpt != 0) {
        board[0] |= (Bitboard)(pcpt&1)<<cpt;
        board[1] |= (Bitboard)((pcpt>>1)&1)<<cpt;
        board[2] |= (Bitboard)((pcpt>>2)&1)<<cpt;
        board[3] |= (Bitboard)((pcpt>>3)&1)<<cpt;
    }

    // restore from
    board[0] |= (Bitboard)(pfrom&1)<<from;
    board[1] |= (Bitboard)((pfrom>>1)&1)<<from;
    board[2] |= (Bitboard)((pfrom>>2)&1)<<from;
    board[3] |= (Bitboard)((pfrom>>3)&1)<<from;


    // TODO: castle moves

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
    int pidx = get_global_id(0);
    int pidy = get_local_id(1);
    int boardindex = 0;
    int moveindex = 0;
    int i = 0;
    int kic = 0;
    int qs = 0;
    Square pos;
    Square to;   
    Square cpt;   
    Piece piece;
    Piece pieceto;
    Piece piececpt;
    Piece kic_piece = 0;
    Piece kic_pos;
    Bitboard kic_pro;
    Bitboard kic_gen;
    int kic_r;
    int movecounter = 0;
    int search_depth = 0;
    signed int r;
    Bitboard bbTemp = 0;
    Bitboard bbWork = 0;
    Bitboard bbBoth[2] = {0,0};
    Bitboard bbBlockers = 0;
    Bitboard bbMoves = 0;
    Bitboard bbCaptures = 0;
    Bitboard bbNonCaptures = 0;
    Bitboard gen = 0;
    Bitboard pro = 0;
    event_t event = (event_t)0;

    boardindex = (search_depth*pidx*4);
    moveindex = (search_depth*256*256)+(pidx*256)+0;

    event = async_work_group_copy((__local Bitboard*)board, (const __global Bitboard* )&globalboard[boardindex], (size_t)4, (event_t)0);

    // #########################################
    // #### Kogge Stone like Move Generator ####
    // #########################################
    movecounter = 0;
    bbBoth[BLACK] = board[0];
    bbBoth[WHITE] = board[0] ^ (board[1] | board[2] | board[3]); 
    bbBlockers = bbBoth[BLACK] | bbBoth[WHITE];
    bbWork = bbBoth[som];

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
        bbCaptures = ((piece>>1) == 1) ? (bbMoves & bbBoth[!som] & PawnAttackTables[som*64+pos])         :   bbMoves & bbBoth[!som];

        // Non Captures, considering Pawn Attacks
        bbNonCaptures = ((piece>>1) == 1) ? ( bbMoves & ~ PawnAttackTables[som*64+pos] & ~bbBlockers)    : bbMoves & ~bbBlockers;

        // Quiscence Search?
        bbMoves = (qs)? (bbCaptures) : (bbCaptures | bbNonCaptures);

        // dirty but simple, considering non sliders and not allowed multible shifts
        bbMoves &= AttackTables[(som*7*64)+((piece>>1)*64)+pos];


        // TODO: think about parallizing this while with 8 threads
        while(bbMoves) {
            to = pop_1st_bit(&bbMoves, BitTable);
            cpt = to;        // TODO: en passant
            pieceto = piece; // TODO: Pawn promotion

            piececpt = getPiece(board, cpt);

            move = ((Move)pos | (Move)to<<6 | (Move)cpt<<12 | (Move)piece<<18 | (Move)pieceto<<22 | (Move)piececpt<<26 );

            if (pidx == 0 && pidy == 0 ) {
                OutputBB[movecounter] = move;
                globalmoves[moveindex] = move;
                moveindex++;
                movecounter++;
            }

        }
    }




    if (pidx == 0 && pidy == 0 ) {
        *bestmove = globalmoves[moveindex-1];
        *MOVECOUNT = movecounter;
        *NODECOUNT = 1;
    }
}


