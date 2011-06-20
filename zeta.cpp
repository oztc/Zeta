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

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include "types.h"
#include "bitboard.h" // Magic Hashtables from <stockfish>



const char filename[]  = "zeta.cl";
char *source;
size_t sourceSize;

U64 MOVECOUNT = 0;
U64 NODECOUNT = 0;
int PLY = 0;
int SOM = WHITE;

// config
const int max_depth  = 40;
int search_depth = max_depth;
int max_mem_mb = 64;
int max_cores  = 1;
int force_mode   = false;
int random_mode         = false;


clock_t start, end;
double elapsed;


const Score  INF = 32000;

Bitboard BOARD[4];


extern unsigned int threadsX;
extern unsigned int threadsY;
// for exchange with OpenCL Device
Bitboard OutputBB[64];
Move *MOVES = (Move*)malloc((max_depth*threadsX*threadsY*threadsY) * sizeof (Move));
Bitboard *BOARDS = (Bitboard*)malloc((max_depth*threadsX*threadsY*4) * sizeof (Bitboard));




Bitboard AttackTables[2][7][64];
Bitboard AttackTablesTo[2][7][64];
Bitboard PawnAttackTables[4][64];

Move bestmove = 0;
Move Lastmove = 0;

// functions
Move move_parser(char *usermove, Bitboard *board, int som);
void setboard(char *fen);
void print_movealg(Move move);
static void print_bitboard(Bitboard board);
void print_board(Bitboard *board);
void print_stats();


// cl functions
extern int load_file_to_string(const char *filename, char **result);
extern int initializeCLDevice();
extern int initializeCL();
extern int  runCLKernels(unsigned int som, Move lastmove, unsigned int maxdepth);
extern int releaseCLDevice();


int BitTable[64] = {
  0, 1, 2, 7, 3, 13, 8, 19, 4, 25, 14, 28, 9, 34, 20, 40, 5, 17, 26, 38, 15,
  46, 29, 48, 10, 31, 35, 54, 21, 50, 41, 57, 63, 6, 12, 18, 24, 27, 33, 39,
  16, 37, 45, 47, 30, 53, 49, 56, 62, 11, 23, 32, 36, 44, 52, 55, 61, 22, 43,
  51, 60, 42, 59, 58
};


const Score EvalPieceValues[7] = {0, 100, 400, 0, 400, 600, 1200};

const Score EvalControl[64] = 

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

const Score EvalTable[7][64] = 

{
    // Empty 
    {
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0
    },

    // Black Pawn
    {
     0,  0,  0,  0,  0,  0 , 0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    30, 30, 30, 30, 30, 30, 30, 30,
     5,  5,  5, 10, 10,  5,  5,  5,
     3,  3,  3,  8,  8,  3,  3,  3,
     2,  2,  2,  2,  2,  2,  2,  2,
     0,  0,  0, -5, -5,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0
    },

    // Black Knight
    {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
    },
    // Black King
    {
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0
    },
    // Black Bishop
    {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
    },

    // Black Rook 
    {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0
    },

    // Black Queen 
    {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
    }

};



/* ############################# */
/* ###        inits          ### */
/* ############################# */

void inits() {

    int side;
    int from;
    int from88;
    int to;
    int to88;
    int piece;
    int direction;
    int index;

    // AttackTables MicroMax Style
    static int o[2][33] = 
    {
        {
            16,15,17,0,			            /* upstream pawn 	  */
            1,-1,16,-16,0,			        /* rook 		  */
            1,-1,16,-16,15,-15,17,-17,0,	/* king, queen and bishop */
            14,-14,18,-18,31,-31,33,-33,0,	/* knight 		  */
            -1,17,8,12,3,8,		            /* directory 		  */
        },
        {
            -16,-15,-17,0,			        /* downstream pawn 	  */
            1,-1,16,-16,0,			        /* rook 		  */
            1,-1,16,-16,15,-15,17,-17,0,	/* king, queen and bishop */
            14,-14,18,-18,31,-31,33,-33,0,	/* knight 		  */
            -1,17,8,12,3,8,		            /* directory 		  */
        }
    };

    // init biboard stuff
    init_bitboards();

    // init AttackTables
    // for each side
    for (side = 0; side < 2 ; side++) {
        // for each square
        for (from=0; from < 64; from++) {
            from88 = ((from&56)/8)*16 + (from&7);
            // for each piece
            for (piece= PAWN; piece <= QUEEN; piece++) {

                AttackTables[side][piece][from] = 0ULL;
                index = o[side][piece+26];
                while(direction=o[side][++index]) {
                    to88 = from88; 
                    do {
                        to88+= direction;
                        if(to88&0x88) break;	 

                        to = (to88/16)*8 + to88%16;    

                        if (piece == PAWN && (abs(direction) == 15 || abs(direction) == 17) ) {
                            PawnAttackTables[side][from]        |= 1ULL<<to;
                            AttackTablesTo[side][piece][to]     |= 1ULL<<from;
                        }
                        else if (piece == PAWN && (abs(direction) == 16) ) {
                            PawnAttackTables[side+2][from]      |= 1ULL<<to; 
                        }
                        else {
                            AttackTables[side][piece][from]     |= 1ULL<<to;
                            AttackTablesTo[side][piece][to]     |= 1ULL<<from;
                        }
            
                    }while(piece != PAWN && piece != KING && piece != KNIGHT);
                }
            }
        }
    }

/*
    for (side = 1 ; side <2; side++) {
        for (piece = PAWN; piece <= KING; piece++) {
            for (from = 0; from < 64; from++) {

                printf("Side: %i, Piece: %i, Square %i, \n", side, piece, from);
                print_bitboard(AttackTables[side][piece][from]);

            }
        }
    }    
*/

}


/* ############################# */
/* ###     move tools        ### */
/* ############################# */

static inline Move makemove(Square from, Square to, Square cpt, Piece pfrom, Piece pto, Piece pcpt ) {
    return (from | (Move)to<<6 |  (Move)cpt<<12 |  (Move)pfrom<<18 | (Move)pto<<22 | (Move)pcpt<<26);  
}
static inline Move getmove(Move move) {
    return (move & 0x3FFFFFFF);  
}
static inline Move setboardscore(Move move, Score score) {
    return ((move & 0xFFFF0000FFFFFFFF) | (Move)(score&0xFFFF)<<32);  
}
static inline Score getboardscore(Move move) {
    return ((move>>32) &0xFFFF);  
}
static inline Move setsearchscore(Move move, Score score) {
    return ((move & 0xFFFFFFFFFFFF) | (Move)(score&0xFFFF)<<48);  
}
static inline Score getsearchscore(Move move) {
    return ((move>>48) &0x3FFF);  
}
static inline Square getfrom(Move move) {
    return (move & 0x3F);
}
static inline Square getto(Move move) {
    return ((move>>6) & 0x3F);
}
static inline Square getcpt(Move move) {
    return ((move>>12) & 0x3F);
}
static inline Square getpfrom(Move move) {
    return ((move>>18) & 0xF);
}
static inline Square getpto(Move move) {
    return ((move>>22) & 0xF);
}
static inline Square getpcpt(Move move) {
    return ((move>>26) & 0xF);
}




Piece getPiece (Bitboard *board, Square sq) {

   return ((board[0] >> sq) & 1)
      + 2*((board[1] >> sq) & 1)
      + 4*((board[2] >> sq) & 1)
      + 8*((board[3] >> sq) & 1);
}

static int cmp_move_desc(const void *ap, const void *bp) {

    const Move *a = (Move *)ap;
    const Move *b = (Move *)bp;

    return getboardscore(*b) - getboardscore(*a);
}


/* ############################# */
/* ###     domove undomove   ### */
/* ############################# */

void domove(Bitboard *board, Move move, int som) {

    Square from = getfrom(move);
    Square to   = getto(move);
    Square cpt  = getcpt(move);

    Piece pto   = getpto(move);
    Piece pcpt  = getpcpt(move);

    // unset from
    board[0] &= ClearMaskBB[from];
    board[1] &= ClearMaskBB[from];
    board[2] &= ClearMaskBB[from];
    board[3] &= ClearMaskBB[from];

    // unset cpt
    if (pcpt != PEMPTY) {
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


void undomove(Bitboard *board, Move move, int som) {


    Square from = getfrom(move);
    Square to   = getto(move);
    Square cpt  = getcpt(move);

    Piece pfrom = getpfrom(move);
    Piece pcpt  = getpcpt(move);

    // unset to
    board[0] &= ClearMaskBB[to];
    board[1] &= ClearMaskBB[to];
    board[2] &= ClearMaskBB[to];
    board[3] &= ClearMaskBB[to];

    // unset cpt
    if (pcpt != PEMPTY) {
        board[0] &= ClearMaskBB[cpt];
        board[1] &= ClearMaskBB[cpt];
        board[2] &= ClearMaskBB[cpt];
        board[3] &= ClearMaskBB[cpt];
    }

    // restore cpt
    if (pcpt != PEMPTY) {
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

}

// #########################################
// ####         Evaluation              ####
// #########################################


Score eval(Bitboard *board, int som) {

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
            pos = pop_1st_bit(&bbWork);

            p = (getPiece(board, pos))>>1;

            score+= side? -EvalPieceValues[p]   : EvalPieceValues[p];
            score+= side? -EvalTable[p][pos]    : EvalTable[p][FLIPFLOP(pos)];
            score+= side? -EvalControl[pos]     : EvalControl[FLIPFLOP(pos)];
        }
    }
    return score;
}

Score evalMove(Piece piece, Square pos, int side) {

    Score score = 0;

    score+= side? EvalPieceValues[piece]    :   EvalPieceValues[piece];
    score+= side? EvalTable[piece][pos]     :   EvalTable[piece][FLIPFLOP(pos)];
    score+= side? EvalControl[pos]          :   EvalControl[FLIPFLOP(pos)];

    return score;
}

// #########################################
// ####         Move Generator          ####
// #########################################

static int genmoves_general(Bitboard *board, Move *moves, int movecounter, int som, Move lastmove, int qs) {

    int i;
    int kingpos;
    bool kic = false;
    Piece piece, pieceto, piececpt;
    Square pos, to, cpt; 
    Square lastto = getto(lastmove);
    Move move;
    Bitboard bbTemp = 0;
    Bitboard bbWork = 0;
    Bitboard bbWorkkic = 0;
    Bitboard bbMe = 0;
    Bitboard bbMekic = 0;    
    Bitboard bbOpposite = 0;
    Bitboard bbOppositekic = 0;
    Bitboard bbMoves = 0;
    Bitboard bbMoveskic = 0;
    Bitboard bbBlockers = 0;
    Bitboard bbBlockerskic = 0;

    bbMe        = (som)? ( board[0] )                                   : (board[0] ^ (board[1] | board[2] | board[3]));
    bbOpposite  = (som)? ( board[0] ^ (board[1] | board[2] | board[3])) : (board[0]);
    bbBlockers  = (bbMe | bbOpposite);
    bbWork      = bbMe;

    while(bbWork) {
        // pop 1st bit
        pos = ((Square)(BitTable[((bbWork & -bbWork) * 0x218a392cd3d5dbf) >> 58]) );
        bbWork &= (bbWork-1); 

        piece = getPiece(board, pos);

//            kingpos = ((piece>>1) == 3) ? pos : kingpos;

        // Knight and King
        bbTemp = ((piece>>1) == KNIGHT || (piece>>1) == KING )? AttackTables[som][(piece>>1)][pos] : 0x00;

        // Sliders
        // rook or queen
        bbTemp |= ((piece>>1) == ROOK || (piece>>1) == QUEEN)?    ( RAttacks[RAttackIndex[pos] + (((bbBlockers & RMask[pos]) * RMult[pos]) >> RShift[pos])] ) : 0x00;
        // bishop or queen
        bbTemp |= ((piece>>1) == BISHOP || (piece>>1) == QUEEN)?    ( BAttacks[BAttackIndex[pos] + (((bbBlockers & BMask[pos]) * BMult[pos]) >> BShift[pos])] ) : 0x00;

        // Pawn attacks
        bbTemp  |= ( (piece>>1)== PAWN) ? (PawnAttackTables[som][pos] & bbOpposite)   : 0x00 ;

        // White Pawn forward step
        bbTemp  |= ((piece>>1)== PAWN && !som) ? (PawnAttackTables[2][pos]&(~bbBlockers & SetMaskBB[pos+8]))            : 0x00 ;
        // White Pawn double square
        bbTemp  |= ((piece>>1)== PAWN && !som && ((pos&56)/8 == 1 ) && (~bbBlockers & SetMaskBB[pos+8]) && (~bbBlockers & SetMaskBB[pos+16]) ) ? SetMaskBB[pos+16] : 0x00;
        // Black Pawn forward step
        bbTemp  |= ((piece>>1)== PAWN &&  som) ? (PawnAttackTables[3][pos]&(~bbBlockers & SetMaskBB[pos-8]))            : 0x00 ;
        // Black Pawn double square
        bbTemp  |= ((piece>>1)== PAWN &&  som && ((pos&56)/8 == 6 ) && (~bbBlockers & SetMaskBB[pos-8]) && (~bbBlockers & SetMaskBB[pos-16]) ) ? SetMaskBB[pos-16] : 0x00 ;

        // Captures
        bbMoves = bbTemp&bbOpposite;            
        // Non Captures
        bbMoves |= (qs)? 0x00 : (bbTemp&~bbBlockers);


        while(bbMoves) {
            kic = false;
            // pop 1st bit
            to = ((Square)(BitTable[((bbMoves & -bbMoves) * 0x218a392cd3d5dbf) >> 58]) );
            bbMoves &= (bbMoves-1);

            cpt = to;        // TODO: en passant
            pieceto = piece; // TODO: Pawn promotion

            piececpt = getPiece(board, cpt);

            // make move and stire in global
            move = ((Move)pos | (Move)to<<6 | (Move)cpt<<12 | (Move)piece<<18 | (Move)pieceto<<22 | (Move)piececpt<<26 );


            // ################################
            // ####  legal moves only       ###
            // ################################
            domove(board, move, som);

            bbMekic         = (som)? ( board[0] )                                   : (board[0] ^ (board[1] | board[2] | board[3]));
            bbOppositekic   = (som)? ( board[0] ^ (board[1] | board[2] | board[3])) : (board[0]);
            bbBlockerskic   = (bbMekic | bbOppositekic);

            //get king position
            bbWorkkic = (bbMekic & (board[1]) & (board[2]) & (~board[3]) );
            kingpos = ((Square)(BitTable[((bbWorkkic & -bbWorkkic) * 0x218a392cd3d5dbf) >> 58]) );


            // Queens
            bbWorkkic = (bbOppositekic &  (board[2]) & (board[3]) );
            bbMoveskic = queen_attacks_bb(kingpos, bbBlockerskic) ;
            if (bbMoveskic & bbWorkkic) {
                kic = true;
            }
            // Rooks
            bbWorkkic = (bbOppositekic &  (board[1]) & (board[3]) );
            bbMoveskic = rook_attacks_bb(kingpos, bbBlockerskic);
            if (bbMoveskic & bbWorkkic) {
                kic = true;
            }
            // Bishops
            bbWorkkic = (bbOppositekic &  (~board[1]) & (board[3]) );
            bbMoveskic = bishop_attacks_bb(kingpos, bbBlockerskic);
            if (bbMoveskic & bbWorkkic) {
                kic = true;
            }
            // Knights
            bbWorkkic = (bbOppositekic & (~board[1]) & (board[2]) );
            bbMoveskic = AttackTablesTo[!som][KNIGHT][kingpos] ;
            if (bbMoveskic & bbWorkkic) {
                kic = true;
            }
            // Pawns
            bbWorkkic = (bbOppositekic & (board[1]) & (~board[2]) );
            bbMoveskic = AttackTablesTo[!som][PAWN][kingpos];
            if (bbMoveskic & bbWorkkic) {
                kic = true;
            }
            // King
            bbWorkkic = (bbOppositekic & (board[1]) & (board[2]) );
            bbMoveskic = AttackTablesTo[!som][KING][kingpos] ;
            if (bbMoveskic & bbWorkkic) {
                kic = true;
            }
           
            undomove(board, move, som);


            if (kic == false) {
                moves[movecounter] = move;
                movecounter++;
            }
        }
    }

    // ################################
    // #### TODO: Castle moves      ###
    // ################################

    // ################################
    // #### TODO: En passant moves  ###
    // ################################



    // ######################################
    // #### TODO: sort moves              ###
    // ######################################



    // sort the moves
//    qsort(moves, movecounter, sizeof(Move), cmp_move_desc);

    return movecounter;
}


/* ################################ */
/* ###      negamax search      ### */
/* ################################ */


Score negamax_cpu(Bitboard *board, int som, int depth, Move lastmove) {

    Score score = 0;
    Score bestscore = -32000;
    Move moves[128];
    int movecounter = 0;
    int i=0;

    NODECOUNT++;

    movecounter = genmoves_general(board, moves, movecounter, som, lastmove, false);

    if (depth >= search_depth && movecounter == 0)
        return -30000;

    if (depth >= search_depth)
        return som? -eval(board, som) : -eval(board, som);

    MOVECOUNT+=movecounter;


    for (i=0;i<movecounter;i++) {

        domove(board, moves[i], som);

        score = -negamax_cpu(board, !som, depth+1, moves[i]);

        if (score >= bestscore)
            bestscore = score;

        undomove(board, moves[i], som);
    }


    return bestscore;
}



/* ############################# */
/* ###      root search      ### */
/* ############################# */

Move rootsearch(Bitboard *board, int som, int depth, Move lastmove) {

    int status =0;
    bestmove = 0;
    Move moves[128];
    int movecounter = 0;
    int qs = 0;
    int i = 0;
    Score score = 0;
    Score bestscore = -32000;
    Move OutputMoves[128];

    NODECOUNT = 0;
    MOVECOUNT = 0;

    // gen first depth moves
    movecounter = genmoves_general(board, moves, movecounter, som, lastmove, qs);

/*
    for (i=0; i < movecounter; i++) {


        domove(board, moves[i], som);
        score = -negamax_cpu(board, !som, 1,lastmove);

        if (score >= bestscore) {
            bestscore = score;
            bestmove = moves[i];
        }

        undomove(board, moves[i], som);
    }
*/
    // copy board to membuffer
    BOARDS[0] = board[0];
    BOARDS[1] = board[1];
    BOARDS[2] = board[2];
    BOARDS[3] = board[3];

    // clear moves buffer
    for (i=0; i< 128; i++) {
        MOVES[i] = 0;
    }

    // initial eval and copy first depth moves to global
    for (i=0; i< movecounter; i++) {
        domove(board, moves[i], som);
        score = eval(board, !som);
        MOVES[i] = setsearchscore(moves[i], score);
        undomove(board, moves[i], som);
    }

    // when legal moves available
    if (movecounter > 0) {
        start = clock();
        status = initializeCL();
        status = runCLKernels(som, lastmove, depth-1);
    }

    end = clock();
    elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;


/*
    for (int i = 0; i <64; i++) {
        print_bitboard(OutputBB[i]);
    }
*/

    print_stats();

    fflush(stdout);

    return bestmove;
}


/* ############################# */
/* ### main loop, and Xboard ### */
/* ############################# */
int main(void) {

    int status = 0;
    char line[256];
    char command[256];
    char c_usermove[256];
    int my_side = WHITE;
    int go = 0;
    Move move;
    Move usermove;

	signal(SIGINT, SIG_IGN);

    inits();

    if (MOVES == NULL || BOARDS == NULL)
        exit(0);

    load_file_to_string(filename, &source);
    sourceSize    = strlen(source);
    status = initializeCLDevice();

/*
            setboard("setboard rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            Lastmove = 0;
            PLY =0;
            go = false;
            force_mode = false;
            random_mode = false;
            move = rootsearch(BOARD, SOM, search_depth, Lastmove);
            exit(0);
*/

    for(;;) {
        fflush(stdout);
		if (!fgets(line, 256, stdin)) {
        }
		if (line[0] == '\n') {
			continue;
        }
		sscanf(line, "%s", command);

		if (!strcmp(command, "test")) {
            setboard("setboard rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            Lastmove = 0;
            PLY =0;
            go = false;
            force_mode = false;
            random_mode = false;
            for (int i=0; i<20; i++) {
                move = rootsearch(BOARD, SOM, search_depth, Lastmove);
            }
            exit(0);
			continue;
        }

		if (!strcmp(command, "xboard")) {
			continue;
        }
        if (strstr(command, "protover")) {
			printf("feature myname=\"Zeta 0902 \" reuse=0 colors=1 setboard=1 memory=1 smp=1 usermove=1 san=0 time=0 debug=1 \n");
			continue;
        }
		if (!strcmp(command, "new")) {
            setboard("setboard rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            Lastmove = 0;
            PLY =0;
            go = false;
            force_mode = false;
            random_mode = false;
			continue;
		}
		if (!strcmp(command, "setboard")) {
            setboard(line);
            Lastmove = 0;
            PLY =0;
            go = false;
            force_mode = false;
            random_mode = false;
			continue;
		}
		if (!strcmp(command, "quit")) {
            free(MOVES);
            free(BOARDS);
            status = releaseCLDevice();
            exit(0);
        }
		if (!strcmp(command, "force")) {
            force_mode = true;
			continue;
		}
		if (!strcmp(command, "white")) {
			my_side = WHITE;
			continue;
		}
		if (!strcmp(command, "black")) {
			my_side = BLACK;
			continue;
		}
		if (!strcmp(command, "sd")) {
			sscanf(line, "sd %i", &search_depth);
            search_depth = (search_depth >= max_depth) ? max_depth : search_depth;
			continue;
		}
		if (!strcmp(command, "memory")) {
			sscanf(line, "memory %i", &max_mem_mb);
			continue;
		}
		if (!strcmp(command, "cores")) {
			sscanf(line, "cores %i", &max_cores);
			continue;
		}
		if (!strcmp(command, "go")) {
            force_mode = false;
            if (!go) {
                move = rootsearch(BOARD, SOM, search_depth, Lastmove);
                if (move != 0) { 
                    Lastmove = move;
                    PLY++;
                    domove(BOARD, move, SOM);
                    print_movealg(move);
                    SOM = !SOM;
                }
            }
            continue;
		}
        if (strstr(command, "usermove")){

            sscanf(line, "usermove %s", c_usermove);
            usermove = move_parser(c_usermove, BOARD, SOM);
            Lastmove = usermove;
            domove(BOARD, usermove, SOM);
            PLY++;
            print_board(BOARD);
            SOM = !SOM;
            if (!force_mode) {
                go = true;
                move = rootsearch(BOARD, SOM, search_depth, Lastmove);
                if (move != 0) { 
                    domove(BOARD, move, SOM);
                    PLY++;
                    Lastmove = move;
                    print_board(BOARD);
                    print_movealg(move);
                    SOM = !SOM;
                }
            }
        }
    }
   return 0;
}


/* ############################# */
/* ###        parser         ### */
/* ############################# */

Move move_parser(char *usermove, Bitboard *board, int som) {

    int file;
    int rank;
    Square from,to,cpt;
    Piece pto, pfrom, pcpt;
    Move move;
    char promopiece;

    file = (int)usermove[0] -97;
    rank = (int)usermove[1] -49;
    from = make_square(file,rank);
    file = (int)usermove[2] -97;
    rank = (int)usermove[3] -49;
    to = make_square(file,rank);

    pfrom = getPiece(board, from);
    pto = pfrom;
    cpt = to;

    /* en passant */
    cpt = ( pfrom == PAWN &&  som && (from&56)/8 == 3 && from-to != 8 && pto == PEMPTY ) ? to+8 : cpt;
    cpt = ( pfrom == PAWN && !som && (from&56)/8 == 4 && to-from != 8 && pto == PEMPTY ) ? to-8 : cpt;

    pcpt = getPiece(board, cpt);

    /* pawn promo piece */
    promopiece = usermove[4];
    if (promopiece == 'q' || promopiece == 'Q' )
        pto = QUEEN;
    else if (promopiece == 'n' || promopiece == 'N' )
        pto = KNIGHT;
    else if (promopiece == 'b' || promopiece == 'B' )
        pto = BISHOP;
    else if (promopiece == 'r' || promopiece == 'R' )
        pto = ROOK;

    move = makemove(from, to, cpt, pfrom, pto , pcpt);

    return move;
}

void setboard(char *fen) {

    int i, j, side;
    Square ep_sq;
    int index;
    int file = 0;
    int rank = 7;
    int pos  = 0;
    char temp;
    char position[255];
    char csom;
    char castle[5];
    char ep[3];
    int bla;
    int blubb;
    char string[] = {" PNKBRQ pnkbrq/12345678"};

	sscanf(fen, "setboard %s %c %s %s %i %i", position, &csom, castle, ep, &bla, &blubb);


    BOARD[0] = 0;
    BOARD[1] = 0;
    BOARD[2] = 0;
    BOARD[3] = 0;

    i =0;
    while (!(rank==0 && file==8)) {
        temp = position[i];
        i++;        
        for (j=0;j<24;j++) {
    		if (temp == string[j]) {
                if (j == 14) {
                    rank--;
                    file=0;
                }
                else if (j >=15) {
                    file+=j-14;
                }
                else {
                    pos = (rank*8) + file;
                    side = (j>6) ? 1 :0;
                    index = side? j-7 : j;
                    BOARD[0] |= (side)? ((Bitboard)1<<pos) : 0;
                    BOARD[1] |= ( (Bitboard)(index&1)<<pos);
                    BOARD[2] |= ( ( (Bitboard)(index>>1)&1)<<pos);
                    BOARD[3] |= ( ( (Bitboard)(index>>2)&1)<<pos);
                    file++;
                }
                break;                
            }
        }
    }

    // site on move
    if (csom == 'w' || csom == 'W') {
        SOM = WHITE;
    }
    if (csom == 'b' || csom == 'B') {
        SOM = BLACK;
    }

    // TODO: set castle rights

}


/* ############################# */
/* ###       print tools     ### */
/* ############################# */

void print_movealg(Move move) {

    char rankc[] = "12345678";
    char filec[] = "abcdefgh";
    char movec[5] = "";
    Square from = getfrom(move);
    Square to   = getto(move);
    Piece pfrom   = getpfrom(move);
    Piece pto   = getpto(move);


    movec[0] = filec[square_file(from)];
    movec[1] = rankc[square_rank(from)];
    movec[2] = filec[square_file(to)];
    movec[3] = rankc[square_rank(to)];

    /* pawn promo */
    if ( (pfrom == PAWN && ( ( (to&56)/8 == 7) || ((to&56)/8 == 0) ) )) {
        if (pto == QUEEN)
            movec[4] = 'q';
        if (pto == ROOK)
            movec[4] = 'r';
        if (pto == BISHOP)
            movec[4] = 'b';
        if (pto == KNIGHT)
            movec[4] = 'n';
    }

    printf("move %s\n", movec);
    fflush(stdout);

}

static void print_bitboard(Bitboard board) {

    int i,j,pos;
    printf("###ABCDEFGH###\n");
   
    for(i=8;i>0;i--) {
        printf("#%i ",i);
        for(j=0;j<8;j++) {
            pos = ((i-1)*8) + j;
            if (bit_is_set(board, pos)) 
                printf("x");
            else 
                printf("-");

        }
       printf("\n");
    }
    printf("###ABCDEFGH###\n");
    fflush(stdout);
}

void print_board(Bitboard *board) {

    int i,j,pos;
    Piece piece = PEMPTY;
    char wpchars[] = "-PNKBRQ";
    char bpchars[] = "-pnkbrq";

//    print_bitboard(board[0]);
//    print_bitboard(board[1]);
//    print_bitboard(board[2]);
//    print_bitboard(board[3]);

    printf("###ABCDEFGH###\n");

    for(i=8;i>0;i--) {
        printf("#%i ",i);
        for(j=0;j<8;j++) {
            pos = ((i-1)*8) + j;

            piece = getPiece(board, pos);

            if (piece != PEMPTY && (piece&BLACK))
                printf("%c", bpchars[piece>>1]);
            else if (piece != PEMPTY)
                printf("%c", wpchars[piece>>1]);
            else 
                printf("-");
       }
       printf("\n");
    }
    fflush(stdout);
}



void print_stats() {
    FILE 	*Stats;
    Stats = fopen("zeta_amd.debug", "ab+");
    fprintf(Stats, "nodes: %llu ,moves: %llu, bestmove: %llu ,sec: %f \n", NODECOUNT, MOVECOUNT, bestmove, elapsed);
    fclose(Stats);
}

