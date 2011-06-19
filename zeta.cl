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

#define PEMPTY  0
#define PAWN    1
#define KNIGHT  2
#define KING    3
#define BISHOP  4
#define ROOK    5
#define QUEEN   6


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


Piece getPiece (__local Bitboard *board, Square sq) {
   return ((board[0] >> sq) & 1)
      + 2*((board[1] >> sq) & 1)
      + 4*((board[2] >> sq) & 1)
      + 8*((board[3] >> sq) & 1);
}

Score evalMove(Piece piece, Square pos) {

    return (EvalPieceValues[piece] + EvalTable[piece*64+pos] + EvalControl[pos]);    

}


__kernel void negamax_gpu(  __global Bitboard *globalboard,
                            __global Move *globalmoves,
                                    unsigned int som,
                                    unsigned int max_depth,
                                    Move Lastmove,
                            __global Move *Bestmove,
                            __global U64 *NODECOUNT,
                            __global U64 *MOVECOUNT,
                            __global Bitboard *SetMaskBB,
                            __global Bitboard *ClearMaskBB,
                            __global Bitboard *AttackTables,
                            __global Bitboard *AttackTablesTo,
                            __global Bitboard *PawnAttackTables,
                            __global Bitboard *OutputBB,
                                    unsigned int threadsX,
                                    unsigned int threadsY,
                            __global int *BitTable,
                            __global int *RAttackIndex,
                            __global U64 *RAttacks,
                            __global Bitboard *RMask,
                            __global int *BAttackIndex,
                            __global U64 *BAttacks,
                            __global Bitboard *BMask
                        )

{

    __local Bitboard board[128*4];
    __local int done[40];
    __local U64 nodecounter[128];
    __local U64 movecounter[128];
    Score score = 0;
    Move move = 0;
    int pidx = get_global_id(0);
    int pidy = get_local_id(1);
    Square pos;
    Square to;
    Square cpt;   
    Piece piece;
    Piece pieceto;
    Piece piececpt;
    long moveindex = 0;
    signed char sd = 0;
    int kingpos = 0;
    int kic = 0;
    int i = 0;
    Bitboard bbTemp = 0;
    Bitboard bbWork = 0;
    Bitboard bbMe = 0;
    Bitboard bbOpposite = 0;
    Bitboard bbBlockers = 0;
    Bitboard bbMoves = 0;
    event_t event = (event_t)0;

    if (pidy == 0) {

        *NODECOUNT = 0;
        *MOVECOUNT = 0;

        for(i=0; i<40; i++) {
            done[i] = 0;
        }
    }
    
    nodecounter[pidy] = 0;
    movecounter[pidy] = 0;

    // for each search depth
    while (sd >= 0) {

        // for each possible board in fix search depth
        while (done[sd] < 128 && sd < max_depth) {

            if (pidy == 0) {
                done[sd]++;
            }

            // get board for next computation
            event = async_work_group_copy((__local Bitboard*)&board[(pidy*4)], (const __global Bitboard* )&globalboard[(sd*128+(done[sd]-1))*4], (size_t)4, (event_t)0);

            // get apropiate move
            move = globalmoves[(sd*128*128)+((done[sd]-1)*128)+pidy];

            // empty board
            if (board[0] == 0)
                break;

            // move up in tree
            sd++;

            // set global move index for local process
            moveindex = (sd*128*128) + (pidy*128);

            // only if a move is available
            if (move == 0)
                continue;

            nodecounter[pidy]++;

            // domove
            if (move) {
                pos     = (move & 0x3F);
                to      = ((move>>6) & 0x3F);
                cpt     = ((move>>12) & 0x3F);

                piece       = ((move>>18) & 0xF);
                pieceto     = ((move>>22) & 0xF);
                piececpt    = ((move>>26) & 0xF);

                // unset from
                board[(pidy*4)+0] &= ClearMaskBB[pos];
                board[(pidy*4)+1] &= ClearMaskBB[pos];
                board[(pidy*4)+2] &= ClearMaskBB[pos];
                board[(pidy*4)+3] &= ClearMaskBB[pos];

                // unset cpt
                if (piececpt != 0) {
                    board[0] &= ClearMaskBB[cpt];
                    board[1] &= ClearMaskBB[cpt];
                    board[2] &= ClearMaskBB[cpt];
                    board[3] &= ClearMaskBB[cpt];
                }

                // unset to
                board[(pidy*4)+0] &= ClearMaskBB[to];
                board[(pidy*4)+1] &= ClearMaskBB[to];
                board[(pidy*4)+2] &= ClearMaskBB[to];
                board[(pidy*4)+3] &= ClearMaskBB[to];

                // set to
                board[(pidy*4)+0] |= (Bitboard)(pieceto&1)<<to;
                board[(pidy*4)+1] |= (Bitboard)((pieceto>>1)&1)<<to;
                board[(pidy*4)+2] |= (Bitboard)((pieceto>>2)&1)<<to;
                board[(pidy*4)+3] |= (Bitboard)((pieceto>>3)&1)<<to;

            }

            // copy local board to global
            globalboard[(((sd*128)+pidy)*4)+0] = board[(pidy*4)+0];
            globalboard[(((sd*128)+pidy)*4)+1] = board[(pidy*4)+1];
            globalboard[(((sd*128)+pidy)*4)+2] = board[(pidy*4)+2];
            globalboard[(((sd*128)+pidy)*4)+3] = board[(pidy*4)+3];


            // switch site
            som = !som;


            // #########################################
            // ####         Move Generator          ####
            // #########################################
            bbMe      = (som)? ( board[(pidy*4)+0] )                                                                    : (board[(pidy*4)+0] ^ (board[(pidy*4)+1] | board[(pidy*4)+2] | board[(pidy*4)+3]));
            bbOpposite  = (som)? ( board[(pidy*4)+0] ^ (board[(pidy*4)+1] | board[(pidy*4)+2] | board[(pidy*4)+3]))     : (board[(pidy*4)+0]);
            bbBlockers  = (bbMe | bbOpposite);
            bbWork = bbMe;

            while(bbWork) {
                // pop 1st bit
                pos = ((Square)(BitTable[((bbWork & -bbWork) * 0x218a392cd3d5dbf) >> 58]) );
                bbWork &= (bbWork-1); 

                piece = getPiece(board, pos);

    //            kingpos = ((piece>>1) == 3) ? pos : kingpos;

                // Knight and King
                bbTemp = !((piece>>1)&4)? AttackTables[(som*7*64)+((piece>>1)*64)+pos] : 0x00;

                // Sliders
                // rook or queen
                bbTemp |= ((piece>>1) == ROOK || (piece>>1) == QUEEN)?    ( RAttacks[RAttackIndex[pos] + (((bbBlockers & RMask[pos]) * RMult[pos]) >> RShift[pos])] ) : 0x00;
                // bishop or queen
                bbTemp |= ((piece>>1) == BISHOP || (piece>>1) == QUEEN)?    ( BAttacks[BAttackIndex[pos] + (((bbBlockers & BMask[pos]) * BMult[pos]) >> BShift[pos])] ) : 0x00;

                // Pawn attacks
                bbTemp  |= ( (piece>>1)== PAWN) ? (PawnAttackTables[som*64+pos] & bbOpposite)   : 0 ;

                // White Pawn forward step
                bbTemp  |= ((piece>>1)== PAWN && !som) ? (PawnAttackTables[2*64+pos]&(~bbBlockers & SetMaskBB[pos+8]))            : 0x00 ;
                // White Pawn double square
                bbTemp  |= ((piece>>1)== PAWN && !som && ((pos&56)/8 == 1 ) && (~bbBlockers & SetMaskBB[pos+8]) && (~bbBlockers & SetMaskBB[pos+16]) ) ? SetMaskBB[pos+16] : 0x00;
                // Black Pawn forward step
                bbTemp  |= ((piece>>1)== PAWN &&  som) ? (PawnAttackTables[3*64+pos]&(~bbBlockers & SetMaskBB[pos-8]))            : 0x00 ;
                // Black Pawn double square
                bbTemp  |= ((piece>>1)== PAWN &&  som && ((pos&56)/8 == 6 ) && (~bbBlockers & SetMaskBB[pos-8]) && (~bbBlockers & SetMaskBB[pos-16]) ) ? SetMaskBB[pos-16] : 0x00 ;

                // Captures
                bbMoves = bbTemp&bbOpposite;            
                // Non Captures
                bbMoves |= ((sd > max_depth))? 0x00 : (bbTemp&~bbBlockers);


                while(bbMoves) {
                    // pop 1st bit
                    to = ((Square)(BitTable[((bbMoves & -bbMoves) * 0x218a392cd3d5dbf) >> 58]) );
                    bbMoves &= (bbMoves-1);

                    cpt = to;        // TODO: en passant
                    pieceto = piece; // TODO: Pawn promotion

                    piececpt = getPiece(board, to);

                    // make move and store in global
                    move = ((Move)pos | (Move)to<<6 | (Move)cpt<<12 | (Move)piece<<18 | (Move)pieceto<<22 | (Move)piececpt<<26 );

                    // TODO: set incremental board score

                    globalmoves[moveindex] = move;
                    moveindex++;

                }
            }
            // ################################
            // #### TODO: Castle moves      ###
            // ################################

            // ################################
            // #### TODO: En passant moves  ###
            // ################################

            // ################################
            // ####   legal moves only      ###
            // ################################
            for (i = (sd*128*128) + (pidy*128); i < moveindex; i++) {

                move = globalmoves[i];

                // domove
                pos     =  (move & 0x3F);
                to      = ((move>>6) & 0x3F);
                cpt     = ((move>>12) & 0x3F);

                piece       = ((move>>18) & 0xF);
                pieceto     = ((move>>22) & 0xF);
                piececpt    = ((move>>26) & 0xF);

                // unset from
                board[(pidy*4)+0] &= ClearMaskBB[pos];
                board[(pidy*4)+1] &= ClearMaskBB[pos];
                board[(pidy*4)+2] &= ClearMaskBB[pos];
                board[(pidy*4)+3] &= ClearMaskBB[pos];

                // unset cpt
                if (piececpt != 0) {
                    board[(pidy*4)+0] &= ClearMaskBB[cpt];
                    board[(pidy*4)+1] &= ClearMaskBB[cpt];
                    board[(pidy*4)+2] &= ClearMaskBB[cpt];
                    board[(pidy*4)+3] &= ClearMaskBB[cpt];
                }

                // unset to
                board[(pidy*4)+0] &= ClearMaskBB[to];
                board[(pidy*4)+1] &= ClearMaskBB[to];
                board[(pidy*4)+2] &= ClearMaskBB[to];
                board[(pidy*4)+3] &= ClearMaskBB[to];

                // set to
                board[(pidy*4)+0] |= (Bitboard)(pieceto&1)<<to;
                board[(pidy*4)+1] |= (Bitboard)((pieceto>>1)&1)<<to;
                board[(pidy*4)+2] |= (Bitboard)((pieceto>>2)&1)<<to;
                board[(pidy*4)+3] |= (Bitboard)((pieceto>>3)&1)<<to;

                // set board score with incremental eval
                score = EvalPieceValues[pieceto] + EvalTable[pieceto*64+to] + EvalControl[to] - EvalPieceValues[piece] + EvalTable[piece*64+pos] + EvalControl[pos];
                score = som? -score : score;
                score+= ((move>>32) &0x3FFF);
                move = (move & 0xFFFF0000FFFFFFFF) | (Move)(score&0x3FFF)<<32;

                // king in check?
                bbMe        = (som)? ( board[(pidy*4)+0] )                                                                  : (board[(pidy*4)+0] ^ (board[(pidy*4)+1] | board[(pidy*4)+2] | board[(pidy*4)+3]));
                bbOpposite  = (som)? ( board[(pidy*4)+0] ^ (board[(pidy*4)+1] | board[(pidy*4)+2] | board[(pidy*4)+3]))     : (board[(pidy*4)+0]);
                bbBlockers  = (bbMe | bbOpposite);

                //get king position
                bbWork = (bbMe & (board[(pidy*4)+1]) & (board[(pidy*4)+2]) & (~board[(pidy*4)+3]) );
                kingpos = ((Square)(BitTable[((bbWork & -bbWork) * 0x218a392cd3d5dbf) >> 58]) );

                // Queens and Rooks
                bbWork = (bbOpposite & (~board[(pidy*4)+1]) & (board[(pidy*4)+2]) & (board[(pidy*4)+3])) | (bbOpposite & (board[(pidy*4)+1]) & (~board[(pidy*4)+2]) & (board[(pidy*4)+3]));
                bbMoves =  ( RAttacks[RAttackIndex[kingpos] + (((bbBlockers & RMask[kingpos]) * RMult[kingpos]) >> RShift[kingpos])] ) ;
                if (bbMoves & bbWork) {
                    move = 0x00;
                }
                // Queens and Bishops
                bbWork = (bbOpposite & (~board[(pidy*4)+1]) & (board[(pidy*4)+2]) & (board[(pidy*4)+3])) | (bbOpposite & (~board[(pidy*4)+1]) & (~board[(pidy*4)+2]) & (board[(pidy*4)+3]));
                bbMoves =  ( BAttacks[BAttackIndex[kingpos] + (((bbBlockers & BMask[kingpos]) * BMult[kingpos]) >> BShift[kingpos])] );
                if (bbMoves & bbWork) {
                    move = 0x00;
                }
                // Knights
                bbWork = (bbOpposite & (~board[(pidy*4)+1]) & (board[(pidy*4)+2]) & (~board[(pidy*4)+3]));
                bbMoves = AttackTablesTo[(!som*7*64)+(KNIGHT*64)+kingpos] ;
                if (bbMoves & bbWork) {
                    move = 0x00;
                }
                // Pawns
                bbWork = (bbOpposite & (board[(pidy*4)+1]) & (~board[(pidy*4)+2]) & (~board[(pidy*4)+3]));
                bbMoves = AttackTablesTo[(!som*7*64)+(PAWN*64)+kingpos];
                if (bbMoves & bbWork) {
                    move = 0x00;
                }
                // King
                bbWork = (bbOpposite & (board[(pidy*4)+1]) & (board[(pidy*4)+2]) & (~board[(pidy*4)+3]& 0x00));
                bbMoves = AttackTablesTo[(!som*7*64)+(KING*64)+kingpos] ;
                if (bbMoves & bbWork) {
                    move = 0x00;
                }

                // store move in global
                globalmoves[i] = move;
                movecounter[pidy]+= (move)? 1:0;


                // undomove
                // unset to
                board[(pidy*4)+0] &= ClearMaskBB[to];
                board[(pidy*4)+1] &= ClearMaskBB[to];
                board[(pidy*4)+2] &= ClearMaskBB[to];
                board[(pidy*4)+3] &= ClearMaskBB[to];

                // unset cpt
                if (piececpt != PEMPTY) {
                    board[(pidy*4)+0] &= ClearMaskBB[cpt];
                    board[(pidy*4)+1] &= ClearMaskBB[cpt];
                    board[(pidy*4)+2] &= ClearMaskBB[cpt];
                    board[(pidy*4)+3] &= ClearMaskBB[cpt];
                }

                // restore cpt
                if (piececpt != PEMPTY) {
                    board[(pidy*4)+0] |= (Bitboard)(piececpt&1)<<cpt;
                    board[(pidy*4)+1] |= (Bitboard)((piececpt>>1)&1)<<cpt;
                    board[(pidy*4)+2] |= (Bitboard)((piececpt>>2)&1)<<cpt;
                    board[(pidy*4)+3] |= (Bitboard)((piececpt>>3)&1)<<cpt;
                }

                // restore from
                board[(pidy*4)+0] |= (Bitboard)(piece&1)<<pos;
                board[(pidy*4)+1] |= (Bitboard)((piece>>1)&1)<<pos;
                board[(pidy*4)+2] |= (Bitboard)((piece>>2)&1)<<pos;
                board[(pidy*4)+3] |= (Bitboard)((piece>>3)&1)<<pos;



            }

            // ######################################
            // #### TODO: sort moves              ###
            // ######################################


            // if legal moves == 0 then checkmate
        }
        if (sd > 0) {
            // clear moves
            for (i =  (sd*128*128) + (pidy*128); i < moveindex; i++) {
                globalmoves[i] = 0x00;
            }
            // clear board
            globalboard[(((sd*128)+pidy)*4)+0] = 0;
            globalboard[(((sd*128)+pidy)*4)+1] = 0;
            globalboard[(((sd*128)+pidy)*4)+2] = 0;
            globalboard[(((sd*128)+pidy)*4)+3] = 0;
        }

        // move down
        if (pidy == 0) {
            done[sd] = 0;
        }
        som = !som;
        sd--;
    }

    if (pidy == 0) {
        *Bestmove  = globalmoves[0*128*128];
        for (i=0; i<128;i++) {
            *NODECOUNT+= nodecounter[i];
            *MOVECOUNT+= movecounter[i];
        }
    }
}


