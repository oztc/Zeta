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

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics       : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics   : enable

typedef signed int Score;
typedef unsigned char Square;
typedef unsigned char Piece;
typedef unsigned long U64;

typedef U64 Move;
typedef U64 Bitboard;
typedef U64 Hash;

// max internal search depth of GPU
#define max_depth   40

#define WHITE 0
#define BLACK 1

#define ALPHA 0
#define BETA  1


#define INF 32000
#define MATESCORE 29000

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define FLIPFLOP(square)    (((square)^7)^56)
#define SwitchSide(som) ((som) == (WHITE) ? (BLACK) : (WHITE))

#define PEMPTY  0
#define PAWN    1
#define KNIGHT  2
#define KING    3
#define BISHOP  4
#define ROOK    5
#define QUEEN   6


__constant int BitTable[64] = {
  0, 1, 2, 7, 3, 13, 8, 19, 4, 25, 14, 28, 9, 34, 20, 40, 5, 17, 26, 38, 15,
  46, 29, 48, 10, 31, 35, 54, 21, 50, 41, 57, 63, 6, 12, 18, 24, 27, 33, 39,
  16, 37, 45, 47, 30, 53, 49, 56, 62, 11, 23, 32, 36, 44, 52, 55, 61, 22, 43,
  51, 60, 42, 59, 58
};

__constant U64 BMult[64] = {
  0x440049104032280, 0x1021023c82008040, 0x404040082000048,
  0x48c4440084048090, 0x2801104026490000, 0x4100880442040800,
  0x181011002e06040, 0x9101004104200e00, 0x1240848848310401,
  0x2000142828050024, 0x1004024d5000, 0x102044400800200,
  0x8108108820112000, 0xa880818210c00046, 0x4008008801082000,
  0x60882404049400, 0x104402004240810, 0xa002084250200,
  0x100b0880801100, 0x4080201220101, 0x44008080a00000,
  0x202200842000, 0x5006004882d00808, 0x200045080802,
  0x86100020200601, 0xa802080a20112c02, 0x80411218080900,
  0x200a0880080a0, 0x9a01010000104000, 0x28008003100080,
  0x211021004480417, 0x401004188220806, 0x825051400c2006,
  0x140c0210943000, 0x242800300080, 0xc2208120080200,
  0x2430008200002200, 0x1010100112008040, 0x8141050100020842,
  0x822081014405, 0x800c049e40400804, 0x4a0404028a000820,
  0x22060201041200, 0x360904200840801, 0x881a08208800400,
  0x60202c00400420, 0x1204440086061400, 0x8184042804040,
  0x64040315300400, 0xc01008801090a00, 0x808010401140c00,
  0x4004830c2020040, 0x80005002020054, 0x40000c14481a0490,
  0x10500101042048, 0x1010100200424000, 0x640901901040,
  0xa0201014840, 0x840082aa011002, 0x10010840084240a,
  0x420400810420608, 0x8d40230408102100, 0x4a00200612222409,
  0xa08520292120600
};

__constant U64 RMult[64] = {
  0xa8002c000108020, 0x4440200140003000, 0x8080200010011880,
  0x380180080141000, 0x1a00060008211044, 0x410001000a0c0008,
  0x9500060004008100, 0x100024284a20700, 0x802140008000,
  0x80c01002a00840, 0x402004282011020, 0x9862000820420050,
  0x1001448011100, 0x6432800200800400, 0x40100010002000c,
  0x2800d0010c080, 0x90c0008000803042, 0x4010004000200041,
  0x3010010200040, 0xa40828028001000, 0x123010008000430,
  0x24008004020080, 0x60040001104802, 0x582200028400d1,
  0x4000802080044000, 0x408208200420308, 0x610038080102000,
  0x3601000900100020, 0x80080040180, 0xc2020080040080,
  0x80084400100102, 0x4022408200014401, 0x40052040800082,
  0xb08200280804000, 0x8a80a008801000, 0x4000480080801000,
  0x911808800801401, 0x822a003002001894, 0x401068091400108a,
  0x4a10a00004c, 0x2000800640008024, 0x1486408102020020,
  0x100a000d50041, 0x810050020b0020, 0x204000800808004,
  0x20048100a000c, 0x112000831020004, 0x9000040810002,
  0x440490200208200, 0x8910401000200040, 0x6404200050008480,
  0x4b824a2010010100, 0x4080801810c0080, 0x400802a0080,
  0x8224080110026400, 0x40002c4104088200, 0x1002100104a0282,
  0x1208400811048021, 0x3201014a40d02001, 0x5100019200501,
  0x101000208001005, 0x2008450080702, 0x1002080301d00c,
  0x410201ce5c030092
};

__constant int BShift[64] = {
  58, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 59, 59,
  59, 59, 57, 57, 57, 57, 59, 59, 59, 59, 57, 55, 55, 57, 59, 59,
  59, 59, 57, 55, 55, 57, 59, 59, 59, 59, 57, 57, 57, 57, 59, 59,
  59, 59, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 58
};

__constant int RShift[64] = {
  52, 53, 53, 53, 53, 53, 53, 52, 53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53, 52, 53, 53, 53, 53, 53, 53, 52
};

__constant Score EvalPieceValues[7] = {0, 100, 400, 0, 400, 600, 1200};

__constant Score EvalControl[64] = 

{
    0,  0,  5,  5,  5,  5,  0,  0,
    5,  0,  5,  5,  5,  5,  0,  5,
    0,  0, 10,  5,  5, 10,  0,  0,
    0,  5,  5, 10, 10,  5,  5,  0,
    0,  5,  5, 10, 10,  5,  5,  0,
    0,  0, 10,  5,  5, 10,  0,  0,
    0,  0,  5,  5,  5,  5,  0,  0,
    0,  0,  5,  5,  5,  5,  0,  0
};

__constant Score EvalTable[] = 

{
    // Empty 
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,

    // Black Pawn
     0,  0,  0,  0,  0,  0 , 0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    30, 30, 30, 30, 30, 30, 30, 30,
     5,  5,  5, 10, 10,  5,  5,  5,
     3,  3,  3,  8,  8,  3,  3,  3,
     2,  2,  2,  2,  2,  2,  2,  2,
     0,  0,  0, -5, -5,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,

    // Black Knight
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,

    // Black King
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,

    // Black Bishop
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,

    // Black Rook 
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0,

    // Black Queen 
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20

};


void domove(__local Bitboard *board, Square from, Square to, Square cpt, Piece piece, Piece pieceto, Piece piececpt, __global Bitboard *ClearMaskBB) {

    // unset from
    board[0] &= ClearMaskBB[from];
    board[1] &= ClearMaskBB[from];
    board[2] &= ClearMaskBB[from];
    board[3] &= ClearMaskBB[from];

    // unset cpt
    board[0] &= ClearMaskBB[cpt];
    board[1] &= ClearMaskBB[cpt];
    board[2] &= ClearMaskBB[cpt];
    board[3] &= ClearMaskBB[cpt];

    // unset to
    board[0] &= ClearMaskBB[to];
    board[1] &= ClearMaskBB[to];
    board[2] &= ClearMaskBB[to];
    board[3] &= ClearMaskBB[to];

    // set to
    board[0] |= (Bitboard)(pieceto&1)<<to;
    board[1] |= (Bitboard)((pieceto>>1)&1)<<to;
    board[2] |= (Bitboard)((pieceto>>2)&1)<<to;
    board[3] |= (Bitboard)((pieceto>>3)&1)<<to;


}

void undomove(__local Bitboard *board, Square from, Square to, Square cpt, Piece piece, Piece pieceto, Piece piececpt, __global Bitboard *ClearMaskBB) {

    // unset to
    board[0] &= ClearMaskBB[to];
    board[1] &= ClearMaskBB[to];
    board[2] &= ClearMaskBB[to];
    board[3] &= ClearMaskBB[to];

    // unset cpt
    board[0] &= ClearMaskBB[cpt];
    board[1] &= ClearMaskBB[cpt];
    board[2] &= ClearMaskBB[cpt];
    board[3] &= ClearMaskBB[cpt];

    // restore cpt
    board[0] |= (Bitboard)(piececpt&1)<<cpt;
    board[1] |= (Bitboard)((piececpt>>1)&1)<<cpt;
    board[2] |= (Bitboard)((piececpt>>2)&1)<<cpt;
    board[3] |= (Bitboard)((piececpt>>3)&1)<<cpt;

    // restore from
    board[0] |= (Bitboard)(piece&1)<<from;
    board[1] |= (Bitboard)((piece>>1)&1)<<from;
    board[2] |= (Bitboard)((piece>>2)&1)<<from;
    board[3] |= (Bitboard)((piece>>3)&1)<<from;

}




int PieceInCheck(   __local Bitboard *board, 
                    Square sq, 
                    int som, 
                    __global Bitboard *AttackTablesTo,  
                    __global int *RAttackIndex, 
                    __global int *BAttackIndex, 
                    __global Bitboard *RMask, 
                    __global Bitboard *BMask,
                    __global U64 *RAttacks, 
                    __global U64 *BAttacks
                ) 


{

    Bitboard bbWork = 0;
    Bitboard bbMoves = 0;
    Bitboard bbMe        = (som == BLACK)? ( board[0] )  : (board[0] ^ (board[1] | board[2] | board[3]));
    Bitboard bbOpposite  = (som == BLACK)? ( board[0] ^ (board[1] | board[2] | board[3]))     : (board[0]);
    Bitboard bbBlockers  = (bbMe | bbOpposite);
    Bitboard bbOdd       = (board[1] ^ board[2] ^ board[3]);




    // Rooks and Queens
    bbWork = (bbOpposite & board[1] &  board[3] ) | (bbOpposite & board[1] & board[2] );
    bbMoves = ( RAttacks[RAttackIndex[sq] + (((bbBlockers & RMask[sq]) * RMult[sq]) >> RShift[sq])] );
    if (bbMoves & bbWork) {
        return 1;
    }
    // Bishops and Queen
    bbWork = (bbOpposite &bbOdd & board[3] ) | (bbOpposite & board[2] & board[3] );
    bbMoves = ( BAttacks[BAttackIndex[sq] + (((bbBlockers & BMask[sq]) * BMult[sq]) >> BShift[sq])] ) ;
    if (bbMoves & bbWork) {
        return 1;
    }
    // Knights
    bbWork = (bbOpposite & bbOdd & board[2] );
    bbMoves = AttackTablesTo[(SwitchSide(som)*7*64)+(KNIGHT*64)+sq] ;
    if (bbMoves & bbWork) {
        return 1;
    }
    // Pawns
    bbWork = (bbOpposite & bbOdd & board[1]  );
    bbMoves = AttackTablesTo[(SwitchSide(som)*7*64)+(PAWN*64)+sq];
    if (bbMoves & bbWork) {
        return 1;
    }
    // King
    bbWork = (bbOpposite & board[1] & board[2] );
    bbMoves = AttackTablesTo[(SwitchSide(som)*7*64)+(KING*64)+sq] ;
    if (bbMoves & bbWork) {
        return 1;
    } 

    return 0;

}


Score evalBoard(__local Bitboard *board, int som) {

    int p, side;
    Square pos;
    Score score = 0;
    Bitboard bbBoth[2];
    Bitboard bbWork = 0;

    bbBoth[0]   = ( board[0] ^ (board[1] | board[2] | board[3]));
    bbBoth[1]   =   board[0];

    // for each side
    for(side=0; side<2;side++) {
        bbWork = bbBoth[side];

        // each piece
        while (bbWork) {
            // pop 1st bit
            pos     = ((Square)(BitTable[((bbWork & -bbWork) * 0x218a392cd3d5dbf) >> 58]) );
            bbWork &= (bbWork-1); 

            p = ( ((board[0]>>pos) &1) + 2*((board[1]>>pos) &1) + 4*((board[2]>>pos) &1) + 8*((board[3]>>pos) &1))>>1;

            score+= side? -EvalPieceValues[p]   : EvalPieceValues[p];
            score+= side? -EvalTable[p*64+pos]  : EvalTable[p*64+FLIPFLOP(pos)];
            score+= side? -EvalControl[pos]     : EvalControl[FLIPFLOP(pos)];
        }
    }
    return score;
}


Score evalMove(Piece pfrom, Piece pto, Square from, Square to, int som ) {

    Score score = 0;

    score+= som? EvalPieceValues[pto]   : EvalPieceValues[pto];
    score+= som? EvalTable[pto*64+to] : EvalTable[pto*64+FLIPFLOP(to)];
    score+= som? EvalControl[to]        : EvalControl[FLIPFLOP(to)];


    score-= som? EvalPieceValues[pfrom]   : EvalPieceValues[pfrom];
    score-= som? EvalTable[pfrom*64+from] : EvalTable[pfrom*64+FLIPFLOP(from)];
    score-= som? EvalControl[from]        : EvalControl[FLIPFLOP(from)];

    return score;
}

__kernel void negamax_gpu(  __global Bitboard *globalboard,
                            __global Move *globalmoves,
                            __global Score *globalscores,
                            __global int *globaldone,
                                    unsigned int som,
                                    unsigned int search_depth,
                                    Move Lastmove,
                            __global Move *Bestmove,
                            __global U64 *COUNTERS,
                            __global Bitboard *SetMaskBB,
                            __global Bitboard *ClearMaskBB,
                            __global Bitboard *AttackTables,
                            __global Bitboard *AttackTablesTo,
                            __global Bitboard *PawnAttackTables,
                                    unsigned int threadsX,
                                    unsigned int threadsY,
                            __global int *RAttackIndex,
                            __global U64 *RAttacks,
                            __global Bitboard *RMask,
                            __global int *BAttackIndex,
                            __global U64 *BAttacks,
                            __global Bitboard *BMask,
                            __global Score *AlphaBeta
                        )

{

    __local Bitboard board[128*4];
    int pidx = get_global_id(0);
    int pidy = get_local_id(1);
    int bindex = pidy*4;
    Score alpha = 0;
    Score beta  = 0;
    Move move = 0;
    Move tempmove = 0;
    Score score = 0;
    Score bestscore = 0;
    Square pos;
    Square to;
    Square cpt;   
    Piece piece;
    Piece pieceto;
    Piece piececpt;
    U64 moveindex = 0;
    signed int sd = 0;
    Square kingpos = 0;
    char kic = 0;
    int i = 0;
    int n = 0;
    Bitboard bbTemp = 0;
    Bitboard bbWork = 0;
    Bitboard bbMe = 0;
    Bitboard bbOpposite = 0;
    Bitboard bbBlockers = 0;
    Bitboard bbMoves = 0;
    event_t event = (event_t)0;

    // init arrays
    for (i=0; i<max_depth;i++) {
        globaldone[(pidy*max_depth)+i] = 0;
        globalscores[i*max_depth+pidy] = -INF;
    }
    
    
    if (pidy == 0) {
        AlphaBeta[ALPHA] = -INF;
        AlphaBeta[BETA]  = +INF;
    }

    COUNTERS[pidy] = 0;

    // for each search depth
    while (sd >= 0) {

        barrier(CLK_GLOBAL_MEM_FENCE);
        barrier(CLK_LOCAL_MEM_FENCE);
    
        // for each possible board in fix search depth
        while (globaldone[(pidy*max_depth)+sd] < 128 && sd < search_depth) {
//        while (globaldone[(pidy*max_depth)+sd] < 128 ) {

            barrier(CLK_GLOBAL_MEM_FENCE);
            barrier(CLK_LOCAL_MEM_FENCE);

            // get board for next computation
            event = async_work_group_copy((__local Bitboard*)&board[bindex], (const __global Bitboard* )&globalboard[(sd*128+(globaldone[(pidy*max_depth)+sd]))*4], (size_t)4, (event_t)0);

            // get apropiate move
            move = globalmoves[(sd*128*128)+((globaldone[(pidy*max_depth)+sd])*128)+pidy];

            // AB-Pruning
            if (AlphaBeta[sd*2+ALPHA] >= AlphaBeta[sd*2+BETA]) {
                break;
            }

            // if end of board list reached
            if (board[bindex] == 0)
                break;

            // next board
            atom_inc(&globaldone[(pidy*max_depth)+sd]);

            // move up in tree
            sd++;
            
            // AB set new values from previous depth
            AlphaBeta[sd*2+ALPHA] = -AlphaBeta[(sd-1)*2+BETA];
            AlphaBeta[sd*2+BETA]  = -AlphaBeta[(sd-1)*2+ALPHA];

            // switch site
            som = SwitchSide(som);

            // set global move index for local process
            moveindex = (sd*128*128) + (pidy*128);

            // only if a move is available
            if (move == 0) {
                continue;
            }

            // domove
            domove(&board[bindex], (move & 0x3F), ((move>>6) & 0x3F), ((move>>12) & 0x3F),  ((move>>18) & 0xF), ((move>>22) & 0xF),  ((move>>26) & 0xF), ClearMaskBB);


            // #########################################
            // ####         Move Generator          ####
            // #########################################
            bbMe        = (som == BLACK)? ( board[bindex+0] ) : (board[bindex+0] ^ (board[bindex+1] | board[bindex+2] | board[bindex+3]));
            bbOpposite  = (som  == BLACK)? ( board[bindex+0] ^ (board[bindex+1] | board[bindex+2] | board[bindex+3]))   : (board[bindex+0]);
            bbBlockers  = (bbMe | bbOpposite);
            bbWork = bbMe;

            // init local move counter
            n = 0;
            while(bbWork) {

                // pop 1st bit
                pos     = ((Square)(BitTable[((bbWork & -bbWork) * 0x218a392cd3d5dbf) >> 58]) );
                bbWork &= (bbWork-1); 

                piece   = ((board[bindex+0]>>pos) &1) + 2*((board[bindex+1]>>pos) &1) + 4*((board[bindex+2]>>pos) &1) + 8*((board[bindex+3]>>pos) &1);

                // Knight and King
                bbTemp  = ((piece>>1) == KNIGHT || (piece>>1) == KING)? AttackTables[(som*7*64)+((piece>>1)*64)+pos] : 0;

                // Sliders
                // rook or queen
                bbTemp  |= ((piece>>1) == ROOK || (piece>>1) == QUEEN)?      ( RAttacks[RAttackIndex[pos] + (((bbBlockers & RMask[pos]) * RMult[pos]) >> RShift[pos])] ) : 0;
                // bishop or queen
                bbTemp  |= ((piece>>1) == BISHOP || (piece>>1) == QUEEN)?    ( BAttacks[BAttackIndex[pos] + (((bbBlockers & BMask[pos]) * BMult[pos]) >> BShift[pos])] ) : 0;

                // Pawn attacks
                bbTemp  |= ( (piece>>1)== PAWN) ? (PawnAttackTables[som*64+pos] & bbOpposite)   : 0 ;

                // White Pawn forward step
                bbTemp  |= ((piece>>1)== PAWN &&  som == WHITE ) ? (PawnAttackTables[2*64+pos]&(~bbBlockers & SetMaskBB[pos+8]))            : 0 ;
                // White Pawn double square
                bbTemp  |= ((piece>>1)== PAWN &&  som == WHITE && ((pos&56)/8 == 1 ) && (~bbBlockers & SetMaskBB[pos+8]) && (~bbBlockers & SetMaskBB[pos+16]) ) ? SetMaskBB[pos+16] : 0;
                // Black Pawn forward step
                bbTemp  |= ((piece>>1)== PAWN &&  som == BLACK) ? (PawnAttackTables[3*64+pos]&(~bbBlockers & SetMaskBB[pos-8]))            : 0 ;
                // Black Pawn double square
                bbTemp  |= ((piece>>1)== PAWN &&  som == BLACK && ((pos&56)/8 == 6 ) && (~bbBlockers & SetMaskBB[pos-8]) && (~bbBlockers & SetMaskBB[pos-16]) ) ? SetMaskBB[pos-16] : 0 ;

                // Captures
                bbMoves  = bbTemp & bbOpposite;            
                // Non Captures
                bbMoves |= (sd > search_depth)? 0x00 : (bbTemp&~bbBlockers);

                while(bbMoves) {
                    kic = 0;
                    // pop 1st bit
                    to = ((Square)(BitTable[((bbMoves & -bbMoves) * 0x218a392cd3d5dbf) >> 58]) );
                    bbMoves &= (bbMoves-1);

                    cpt = to;        // TODO: en passant
                    pieceto = piece; // TODO: Pawn promotion

                    piececpt = ((board[bindex+0]>>cpt) &1) + 2*((board[bindex+1]>>cpt) &1) + 4*((board[bindex+2]>>cpt) &1) + 8*((board[bindex+3]>>cpt) &1);

                    // make move and store in global
                    move = ((Move)pos | (Move)to<<6 | (Move)cpt<<12 | (Move)piece<<18 | (Move)pieceto<<22 | (Move)piececpt<<26 );

                    // Eval Move, Values or MVV-LVA
                    score = (piececpt == PEMPTY)?  evalMove((piece>>1), (pieceto>>1), pos, to, som) : EvalPieceValues[(piececpt>>1)]*16 - EvalPieceValues[(pieceto>>1)];
                    move = (move & 0x0000FFFFFFFFFFFF) | (Move)score<<48;

                    // domove
                    domove(&board[bindex], pos, to, cpt, piece, pieceto, piececpt, ClearMaskBB);

                    //get king position
                    bbMe    = (som == BLACK)? ( board[bindex+0] )     :   (board[bindex+0] ^ (board[bindex+1] | board[bindex+2] | board[bindex+3]));
                    bbTemp  = (bbMe & (board[bindex+1]) & (board[bindex+2]) );
                    kingpos = ((Square)(BitTable[((bbTemp & -bbTemp) * 0x218a392cd3d5dbf) >> 58]) );

                    // king in check?
                    kic = PieceInCheck(&board[bindex], kingpos, som, AttackTablesTo, RAttackIndex, BAttackIndex, RMask, BMask, RAttacks, BAttacks);

                    if (kic == 0) {
                        // copy move to global
                        globalmoves[moveindex] = move;
                        // set counters
                        moveindex++;
                        n++;
                        COUNTERS[pidy]++;

                        //do negamax scoring
                        if (sd == search_depth) {
                            // get board score TODO: incremental eval
                            score = evalBoard(&board[bindex], som);
                            // for negamax only positive scoring
                            score = (som == BLACK)? -score :score;

                            atom_max(&globalscores[(sd)*threadsY+pidy], score);
                            // AB Update
                            atom_max(&AlphaBeta[sd*2+ALPHA], score);
                        }
                    }
                    // undomove
                    undomove(&board[bindex], pos, to, cpt, piece, pieceto, piececpt, ClearMaskBB);
                }
            }
            if (n == 0) {
                score = -MATESCORE;
                atom_max(&globalscores[(sd)*threadsY+pidy], score);
                // AB Update
                atom_max(&AlphaBeta[sd*2+ALPHA], score);
            }
/*
            //do negamax scoring
            if (n == 0) {
                score = evalBoard(&board[bindex], som);
                bestscore = atom_max(&globalscores[(sd)*threadsY+pidy], score);
                // AB Update
                atom_min(&AlphaBeta[sd*2+ALPHA], bestscore);
            }
*/


            // ################################
            // #### TODO: Castle moves      ###
            // ################################

            // ################################
            // #### TODO: En passant moves  ###
            // ################################

            // copy local board to global if not checkmate
            if ( n > 0) {
                globalboard[(((sd*128)+pidy)*4)+0] = board[bindex+0];
                globalboard[(((sd*128)+pidy)*4)+1] = board[bindex+1];
                globalboard[(((sd*128)+pidy)*4)+2] = board[bindex+2];
                globalboard[(((sd*128)+pidy)*4)+3] = board[bindex+3];
            }

            // ########################################
            // ####  TODO:sort moves                ###
            // ########################################
        }

        // if not root
        if (sd > 0) {
            // clear moves
            for (i =  (sd*128*128) + (pidy*128); i < (sd*128*128) + (pidy*128)+128; i++) {
                globalmoves[i] = 0;
            }
            // clear board
            globalboard[(((sd*128)+pidy)*4)+0] = 0;
            globalboard[(((sd*128)+pidy)*4)+1] = 0;
            globalboard[(((sd*128)+pidy)*4)+2] = 0;
            globalboard[(((sd*128)+pidy)*4)+3] = 0;

        }
        //do negamax scoring
        if (sd > 0) {
            score = globalscores[(sd)*threadsY+pidy];
            // handle empty slots
            score = (score == -INF) ? +INF : score;

            //do negamax scoring
            bestscore = atom_max(&globalscores[(sd-1)*threadsY+(globaldone[(pidy*max_depth)+sd-1]-1)], -score);

            // handle empty slots
            score = AlphaBeta[sd*2+ALPHA];
            score = (score == -INF) ? +INF : score;
            // AB Update
            atom_max(&AlphaBeta[(sd-1)*2+ALPHA], -score);


            // reset scores
            if (sd > 1)
                globalscores[(sd)*threadsY+pidy] = -INF;
        }
        // move down in tree
        globaldone[(pidy*max_depth)+sd] = 0;
        som = SwitchSide(som);
        sd--;
    }

    // return bestmove to host
    if (pidy == 0 ) {
        bestscore = -INF;
        for (i=0;i<128;i++) {
            score = -globalscores[128+i];
            // handle empty slots
            score = (score == INF) ? -INF : score;
            // set return counters for host
            COUNTERS[128+i] = score;

            // get bestmove
            if (score >= bestscore) {
//                *Bestmove = 0;
                *Bestmove = globalmoves[i];
                bestscore = score;
            }
            
        }
    }

}


