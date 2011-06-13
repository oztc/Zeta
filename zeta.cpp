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

long MOVECOUNT = 0;
long NODECOUNT = 0;
int PLY = 0;
int SOM = WHITE;

// config
int max_depth  = 4;
int max_mem_mb = 64;
int max_cores  = 1;
int force_mode   = false;
int random_mode         = false;


clock_t start, end;
double elapsed;


const Score  INF = 32000;

Bitboard BOARD[4];

Bitboard OutputBB[64];
Move MOVES[100*256*256];

Bitboard AttackTables[2][7][64];
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
extern int initializeCL(Bitboard *board);
extern int  runCLKernels(unsigned int som, Move lastmove, unsigned int maxdepth);
extern int   cleanupCL(void);


Bitboard avoidWrap[] =
{
    // PEMPTY
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,

    // Pawn
    0xfefefefefefefe00,
    0xffffffffffffff00,
    0x00fefefefefefefe,
    0x00ffffffffffffff,
    0x007f7f7f7f7f7f7f,
    0x00ffffffffffffff,
    0x7f7f7f7f7f7f7f00,
    0xffffffffffffff00,

    // Knight
    0xFFFFFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFFFF,

    // Bishop
    0xfefefefefefefe00,
    0xfefefefefefefefe,
    0x00fefefefefefefe,
    0x00ffffffffffffff,
    0x007f7f7f7f7f7f7f,
    0x7f7f7f7f7f7f7f7f,
    0x7f7f7f7f7f7f7f00,
    0xffffffffffffff00,

    // Rook
    0xfefefefefefefe00,
    0xfefefefefefefefe,
    0x00fefefefefefefe,
    0x00ffffffffffffff,
    0x007f7f7f7f7f7f7f,
    0x7f7f7f7f7f7f7f7f,
    0x7f7f7f7f7f7f7f00,
    0xffffffffffffff00,

    // Queen
    0xfefefefefefefe00,
    0xfefefefefefefefe,
    0x00fefefefefefefe,
    0x00ffffffffffffff,
    0x007f7f7f7f7f7f7f,
    0x7f7f7f7f7f7f7f7f,
    0x7f7f7f7f7f7f7f00,
    0xffffffffffffff00,

    // KING
    0xfefefefefefefe00,
    0xfefefefefefefefe,
    0x00fefefefefefefe,
    0x00ffffffffffffff,
    0x007f7f7f7f7f7f7f,
    0x7f7f7f7f7f7f7f7f,
    0x7f7f7f7f7f7f7f00,
    0xffffffffffffff00,
};
/*
{
   0xfefefefefefefe00,
   0xfefefefefefefefe,
   0x00fefefefefefefe,
   0x00ffffffffffffff,
   0x007f7f7f7f7f7f7f,
   0x7f7f7f7f7f7f7f7f,
   0x7f7f7f7f7f7f7f00,
   0xffffffffffffff00,
};
*/

signed int shift[] = 
{
     0,  0,  0,  0,  0,  0,  0,  0,   // EMTPY
     9, 16, -7, -8, -9,-16,  7,  8,   // PAWN
    17, 10, -6,-15,-17,-10,  6, 15,   // KNIGTH
     9,  0, -7,  0, -9,  0,  7,  0,   // BISHOP
     0,  1,  0, -8,  0, -1,  0,  8,   // ROOK
     9,  1, -7, -8, -9, -1,  7,  8,   // QUEEN
     9,  1, -7, -8, -9, -1,  7,  8    // KING
};


int BitTable[64] = {
  0, 1, 2, 7, 3, 13, 8, 19, 4, 25, 14, 28, 9, 34, 20, 40, 5, 17, 26, 38, 15,
  46, 29, 48, 10, 31, 35, 54, 21, 50, 41, 57, 63, 6, 12, 18, 24, 27, 33, 39,
  16, 37, 45, 47, 30, 53, 49, 56, 62, 11, 23, 32, 36, 44, 52, 55, 61, 22, 43,
  51, 60, 42, 59, 58
};



/* ############################# */
/* ###        inits          ### */
/* ############################# */

void inits() {

    int i;
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

                        if (piece == PAWN && (abs(direction) == 15 || abs(direction) == 17) )
                            PawnAttackTables[side][from] |= 1ULL<<to;
                        else if (piece == PAWN && (abs(direction) == 16) )
                            PawnAttackTables[side+2][from] |= 1ULL<<to;
                        else
                            AttackTables[side][piece][from] |= 1ULL<<to;

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

static inline Move makemove(Square from, Square to, Square cpt, Piece pfrom, Piece pto, Piece pcpt, Cr cr) {
    return (from | (Move)to<<6 |  (Move)cpt<<12 |  (Move)pfrom<<18 | (Move)pto<<22 | (Move)pcpt<<26 | (Move)cr<<30  );  
}
static inline Move getmove(Move move) {
    return (move & 0x3FFFFFFF);  
}
static inline Move setscore(Move move, Score score) {
    return ((move & 0x3FFFFFFFF) | (U64)score<<34);  
}
static inline Score getscore(Move move) {
    return ((move>>34) &0xFFFF);  
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
static inline Cr getcr(Move move) {
    return (move>>30 & 0xF);
}
static inline Move setcr(Move move, Cr cr) {
    return ( (move & 0x3FFFFFFF) & cr<<30);
}


Piece getPiece (Bitboard *board, Square sq) {

   return ((board[0] >> sq) & 1)
      + 2*((board[1] >> sq) & 1)
      + 4*((board[2] >> sq) & 1)
      + 8*((board[3] >> sq) & 1);
}



/* ############################# */
/* ###     domove undomove   ### */
/* ############################# */

void domove(Bitboard *board, Move move, int som) {

    Square from = getfrom(move);
    Square to   = getto(move);
    Square cpt  = getcpt(move);

    Piece pfrom = getpfrom(move);
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
    Piece pto   = getpto(move);
    Piece pcpt  = getpcpt(move);

    // unset to
    board[0] &= ClearMaskBB[to];
    board[1] &= ClearMaskBB[to];
    board[2] &= ClearMaskBB[to];
    board[3] &= ClearMaskBB[to];

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

/* ############################# */
/* ###      root search      ### */
/* ############################# */

Move rootsearch(Bitboard *board, int som, int depth, Move lastmove) {

    int status =0;
    bestmove = 0;

    start = clock();

    status = initializeCL(board);
    status = runCLKernels(som, lastmove, depth);

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

    char line[256];
    char command[256];
    char c_usermove[256];
    char fen[256];
    int my_side = WHITE;
    int go = 0;
    Move move;
    Move usermove;

	signal(SIGINT, SIG_IGN);

    inits();

    load_file_to_string(filename, &source);
    sourceSize    = strlen(source);

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
            for (int i=0; i<10; i++) {
                move = rootsearch(BOARD, SOM, max_depth, Lastmove);
            }
            
			continue;
        }

		if (!strcmp(command, "xboard")) {
			continue;
        }
        if (strstr(command, "protover")) {
			printf("feature myname=\"Zeta 020 \" reuse=0 colors=1 setboard=1 memory=1 smp=1 usermove=1 san=0 time=0 debug=1 \n");
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
			return 0;
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
			sscanf(line, "sd %i", &max_depth);
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
                move = rootsearch(BOARD, SOM, max_depth, Lastmove);
                if (move != 0) { 
                    Lastmove = move;
                    PLY++;
                    domove(BOARD, move, SOM);
                    print_movealg(move);
                    SOM = SWITCH(SOM);
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
            SOM = SWITCH(SOM);
            if (!force_mode) {
                go = true;
                move = rootsearch(BOARD, SOM, max_depth, Lastmove);
                if (move != 0) { 
                    domove(BOARD, move, SOM);
                    PLY++;
                    Lastmove = move;
                    print_board(BOARD);
                    print_movealg(move);
                    SOM = SWITCH(SOM);
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

    move = makemove(from, to, cpt, pfrom, pto , pcpt, 0 & 0xF);

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
                    BOARD[0] |= (side)? ((Bitboard)BLACK<<pos) : (WHITE<<pos);
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


    print_board(BOARD);
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

    print_bitboard(board[0]);
    print_bitboard(board[1]);
    print_bitboard(board[2]);
    print_bitboard(board[3]);

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
    Stats = fopen("zeta_nv.debug", "ab+");
    fprintf(Stats, "nodes: %lu ,moves: %lu, ,sec: %f \n", NODECOUNT, MOVECOUNT, elapsed);
    fclose(Stats);
}

