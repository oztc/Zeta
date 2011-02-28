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
#include "types.h"
#include "random.h"    /* Nandom by Heinz van Saanen */
#include "bitboard.h" /* magic hashtables and bitcount from Stockfish */

#define WHITE 0
#define BLACK 1


#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define FLIPFLOP(square)    (((square)^7)^56)

const char filename[]  = "zeta.cl";
char *source;
size_t sourceSize;

long MOVECOUNT = 0;
long NODECOUNT = 0;
int alpha_cut  = 0;
int beta_cut   = 0;
int TT_alpha  = 0;
int TT_beta   = 0;
int TT_move   = 0;
int PLY = 0;

/* config */
int max_depth  = 4;
int max_mem_mb = 64;
int max_cores  = 1;


clock_t start, end;
double elapsed;


const Score  INF = 32000;

Bitboard BOARD[11];

/*
00 White
01 Black
02 Pawns
03 Knights
04 Bishops
05 Rooks
06 Queens
07 Kings
08 empty/dummy
09 CR
10 hash
*/

const Boardindex boardEmpty = 8;
const Square biInv = 64;
const Square boardCR = 9;
const Square boardHash = 10;

Move bestmove = 0;


Boardindex BINDEX[65];
Bitboard AttackTables[7][64];
Move Lastmove = 0;

/* functions */
void print_bitboard(Bitboard board);
void print_board(Bitboard *board, Boardindex *bindex);
void print_movealg(Move move);
void print_debug_movealg(Move move);
void print_stats();
Move move_parser(char *usermove, Boardindex *bindex, int som);
int kingincheck(Bitboard *board, int som);
void setboard(char *fen);

extern int load_file_to_string(const char *filename, char **result);
extern int initializeCL(Bitboard *board, Boardindex *bindex);
extern int  runCLKernels(int som, Move lastmove);
extern int   cleanupCL(void);


/* ############################# */
/* ###        inits          ### */
/* ############################# */

void init_tables() {

    int i,j,k,l;

    /* Tables for White Pawns, Black Pawns, Knights and King */
    int step[7][8] =  
    {
        {0},
        {8,0},
        {7,9,0}, 
        {-8,0},
        {-7,-9,0}, 
        {17,15,10,6,-6,-10,-15,-17}, 
        {9,7,-7,-9,8,1,-1,-8} 
    };

    for (i = 0; i < 64; i++) {
        for (j = 0; j < 7; j++) {
            AttackTables[j][i] = 0ULL;
            for (k = 0; k < 8 && step[j][k] != 0; k++) {
                l = i + step[j][k];
                if (l >= 0 && l < 64 && abs((i & 7) - (l & 7)) < 3) {
                    AttackTables[j][i] |= (1ULL << l);
                }
            }
        }
    }
}


/* ############################# */
/* ###     move tools        ### */
/* ############################# */

inline Move makemove(Square from, Square to,  Piece pfrom, Piece pto, Piece pcpt, Cr cr) {
    return (from | to<<6 |  pfrom <<12 | pto<<16 | pcpt<<20 | cr<<24  );  
}
inline Move getmove(Move move) {
    return (move & 0xFFFFFFF);  
}
inline Move setscore(Move move, Score score) {
    return (move | (U64)score<<28);  
}
inline Score getscore(Move move) {
    return (move>>28 &0xFFFF);  
}
inline Square getfrom(Move move) {
    return (move & 0x3F);
}
inline Square getto(Move move) {
    return ((move>>6) & 0x3F);
}
inline Square getpfrom(Move move) {
    return ((move>>12) & 0xF);
}
inline Square getpto(Move move) {
    return ((move>>16) & 0xF);
}
inline Square getpcpt(Move move) {
    return ((move>>20) & 0xF);
}
inline int getcr(Move move) {
    return (move>>24 & 0xF);
}
inline Move setcr(Move move, int cr) {
    return ( (move & 0x0FFFFFF) & cr<<24);
}
inline void setcastle_kingside(Bitboard *board, int som) {
    board[boardCR] |= som? 4 : 1;
}
inline void setcastle_queenside(Bitboard *board, int som) {
    board[boardCR] |= som? 8 : 2;
}
inline void clearcastle_kingside(Bitboard *board, int som) {
    board[boardCR] &= som? 11: 14;
}
inline void clearcastle_queenside(Bitboard *board, int som) {
    board[boardCR] &= som? 7 : 13 ;
}
inline int checkcastle_kingside(Bitboard *board, int som) {
    return som ? (board[boardCR]>>2  &1 ) : (board[boardCR] &1 );
}
inline int checkcastle_queenside(Bitboard *board, int som) {
    return som ? (board[boardCR]>>3  &1 ) : (board[boardCR]>>1 &1 );
}
int is_pdsq_move(Move move) {

    Piece pfrom = getpfrom(move);
    int diff = abs(getfrom(move) - getto(move));

    return ( (pfrom == 0 || pfrom == 6 ) && diff == 16 );
}


/* ############################# */
/* ###     domove undomove   ### */
/* ############################# */

void domove(Bitboard *board, Boardindex *bindex, Move move, int som) {

    Square from  = getfrom(move);
    Square to    = getto(move);
    Square cpt   = to;

    Piece pfrom  = getpfrom(move);
    Piece pto    = getpto(move);
    Piece pcpt   = getpcpt(move);


    /* En passant move */
    if (pfrom == 2 && !som && (from&56)/8 == 4 && to-from != 8 && bindex[to] == boardEmpty ) {
        cpt = to-8;
        pcpt = bindex[cpt];
    }
    else if (pfrom == 2 && som && (from&56)/8 == 3 && from-to != 8 && bindex[to] == boardEmpty ) {
        cpt = to+8;
        pcpt = bindex[cpt];
    }

    /* Castle move kingside */
    if ( (pfrom == 7) && to-from == 2 ) {
        /* unset from */
        board[som]     &= ClearMaskBB[from];
        board[som]     &= ClearMaskBB[from+3];
        board[pfrom]   &= ClearMaskBB[from];
        board[pfrom-2] &= ClearMaskBB[from+3];
        bindex[from]    = boardEmpty;
        bindex[from+3]  = boardEmpty;
        /* set to */
        board[som]     |= SetMaskBB[to];
        board[som]     |= SetMaskBB[to-1];
        board[pto]     |= SetMaskBB[to];
        board[pto-2]   |= SetMaskBB[to-1];
        bindex[to]      = pto;
        bindex[to-1]    = pto-2;
    }
    /* Castle move queenside */
    else if ( (pfrom == 7) && from-to == 2 ) {
        /* unset from */
        board[som]     &= ClearMaskBB[from];
        board[som]     &= ClearMaskBB[from-4];
        board[pfrom]   &= ClearMaskBB[from];
        board[pfrom-2] &= ClearMaskBB[from-4];
        bindex[from]    = boardEmpty;
        bindex[from-4]  = boardEmpty;
        /* set to */
        board[som]  |= SetMaskBB[to];
        board[som]  |= SetMaskBB[to+1];
        board[pto]     |= SetMaskBB[to];
        board[pto-2]   |= SetMaskBB[to+1];
        bindex[to]      = pto;
        bindex[to+1]    = pto-2;
    }
    else {
        /* unset capture*/
        if (pcpt != boardEmpty) {
            board[!som]  &= ClearMaskBB[cpt];
            board[pcpt]  &= ClearMaskBB[cpt];
            bindex[cpt]   = boardEmpty;
        }

        /* set to */
        board[som]  |= SetMaskBB[to];
        board[pto]  |= SetMaskBB[to];
        bindex[to]   = pto;

        /* unset from */
        board[som]   &= ClearMaskBB[from];
        board[pfrom] &= ClearMaskBB[from];
        bindex[from]  = boardEmpty;


    }

    /* update castle rights */
    if (pfrom == 5 && !som && from == 0)
        clearcastle_queenside(board, WHITE);
    if (pfrom == 5 && !som && from == 7)
        clearcastle_kingside(board, WHITE);
    if (pfrom == 5 && som && from == 56)
        clearcastle_queenside(board, BLACK);
    if (pfrom == 5 && som && from == 63)
        clearcastle_kingside(board, BLACK);

    if (pcpt == 5 && !som && cpt == 0)
        clearcastle_queenside(board, WHITE);
    if (pcpt == 5 && !som && cpt == 7)
        clearcastle_kingside(board, WHITE);
    if (pcpt == 5 && som && cpt == 56)
        clearcastle_queenside(board, BLACK);
    if (pcpt == 5 && som && cpt == 63)
        clearcastle_kingside(board, BLACK);


    if (pfrom == 7 && !som && from == 4) {
        clearcastle_queenside(board, WHITE);
        clearcastle_kingside(board, WHITE);
    }
    else if (pfrom == 7 && som && from == 60) {
        clearcastle_queenside(board, BLACK);
        clearcastle_kingside(board, BLACK);
    }

    board[boardEmpty] = 0;
}

void undomove(Bitboard *board, Boardindex *bindex, Move move, int som) {

    Square from  = getfrom(move);
    Square to    = getto(move);
    Square cpt   = getto(move);

    Piece pfrom  = getpfrom(move);
    Piece pto    = getpto(move);
    Piece pcpt   = getpcpt(move);


    /* En passant move */
    if (pfrom == 2 && !som && (from&56)/8 == 4 && to-from != 8 && bindex[to] == boardEmpty ) {
        cpt = to-8;
        pcpt = bindex[cpt];
    }
    else if (pfrom == 2 && som && (from&56)/8 == 3 && from-to != 8 && bindex[to] == boardEmpty ) {
        cpt = to+8;
        pcpt = bindex[cpt];
    }

    /* Castle move kingside */
    if ( (pfrom == 7) && to-from == 2 ) {
        /* unset to */
        board[som]     &= ClearMaskBB[to];
        board[som]     &= ClearMaskBB[to-1];
        board[pto]      &= ClearMaskBB[to];
        board[pto-2]    &= ClearMaskBB[to-1];
        bindex[to]       = boardEmpty;
        bindex[to-1]     = boardEmpty;

        /* from restore */
        board[som]      |= SetMaskBB[from];
        board[som]      |= SetMaskBB[from+3];
        board[pfrom]    |= SetMaskBB[from];
        board[pfrom-2]  |= SetMaskBB[from+3];
        bindex[from]     = pfrom;
        bindex[from+3]   = pfrom-2;
    }
    /* Castle move queenside */
    else if ( (pfrom == 7) && from-to == 2 ) {
        /* unset to */
        board[som]      &= ClearMaskBB[to];
        board[som]      &= ClearMaskBB[to+1];
        board[pto]      &= ClearMaskBB[to];
        board[pto-2]    &= ClearMaskBB[to+1];
        bindex[to]       = boardEmpty;
        bindex[to+1]     = boardEmpty;

        /* from restore */
        board[som]      |= SetMaskBB[from];
        board[som]      |= SetMaskBB[from-4];
        board[pfrom]    |= SetMaskBB[from];
        board[pfrom-2]  |= SetMaskBB[from-4];
        bindex[from]     = pfrom;
        bindex[from-4]   = pfrom-2;
    }
    else {
        /* unset to */
        board[som]  &= ClearMaskBB[to];
        board[pto]  &= ClearMaskBB[to];
        bindex[to]   = boardEmpty;

        /* capture restore */
        if (pcpt != boardEmpty) {
            board[!som]  |= SetMaskBB[cpt];
            board[pcpt] |= SetMaskBB[cpt];
            bindex[cpt]  = pcpt;
        }

        /* from restore */
        board[som]   |= SetMaskBB[from];
        board[pfrom] |= SetMaskBB[from];
        bindex[from]  = pfrom;
    }

    /* restore castle rights */
    board[boardCR] = getcr(move);

    board[boardEmpty] = 0;

}

/* ############################# */
/* ###      root search      ### */
/* ############################# */

Move rootsearch(Bitboard *board, Boardindex *bindex, int som, int depth, Move lastmove) {

    int status =0;

    start = clock();

    status = initializeCL(board, bindex);

    status = runCLKernels(som, lastmove);

    end = clock();
    elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;

    
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

    init_bitboards();
    init_tables();

	signal(SIGINT, SIG_IGN);


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

		if (!strcmp(command, "xboard")) {
			continue;
        }
        if (strstr(command, "protover")) {
			printf("feature myname=\"Zeta 0.0.0.1 \" reuse=0 colors=1 setboard=1 memory=1 smp=1 usermove=1 san=0 time=0 debug=1 \n");
			continue;
        }
		if (!strcmp(command, "new")) {
            setboard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
			continue;
		}
		if (!strcmp(command, "setboard")) {
			sscanf(line, "setboard %s", fen);
            setboard(fen);
			continue;
		}
		if (!strcmp(command, "quit")) {
			return 0;
        }

		if (!strcmp(command, "force")) {
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
            go =1;
            move = rootsearch(BOARD, BINDEX, my_side, max_depth, Lastmove);
            Lastmove = move;
            PLY++;
            domove(BOARD, BINDEX, move, my_side);
            print_movealg(move);
			continue;
		}
        if (strstr(command, "usermove")){
            if (!go) my_side = BLACK;
			sscanf(line, "usermove %s", c_usermove);
            usermove = move_parser(c_usermove, BINDEX, !my_side);
            Lastmove = usermove;
            domove(BOARD, BINDEX, usermove, !my_side);
            PLY++;
            print_board(BOARD, BINDEX);
            if (go) {
                move = rootsearch(BOARD, BINDEX, my_side, max_depth, Lastmove);
                domove(BOARD, BINDEX, move, my_side);
                PLY++;
                Lastmove = move;
                print_board(BOARD, BINDEX);
                print_movealg(move);
            }
			continue;
        }
    }
   return 0;
}


/* ############################# */
/* ###        parser         ### */
/* ############################# */

Move move_parser(char *usermove, Boardindex *bindex, int som) {

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

    pfrom = bindex[from];
    pto = bindex[from];
    cpt = to;
    /* en passant */
    cpt = ( pfrom == 6 && (from&56)/8 == 3 && from-to != 8 && bindex[to] == boardEmpty ) ? to+8 : cpt;
    cpt = ( pfrom == 0 && (from&56)/8 == 4 && to-from != 8 && bindex[to] == boardEmpty ) ? to-8 : cpt;
    pcpt = bindex[cpt];

    /* pawn promo piece */
    promopiece = usermove[4];
    if (promopiece == 'q' || promopiece == 'Q' )
        pto = 6;
    else if (promopiece == 'n' || promopiece == 'N' )
        pto = 3;
    else if (promopiece == 'b' || promopiece == 'B' )
        pto = 4;
    else if (promopiece == 'r' || promopiece == 'R' )
        pto = 5;

    move = makemove(from, to, pfrom, pto , pcpt, BOARD[boardCR] & 0xF);

    return move;
}

void setboard(char *fen) {

    int i, j, side;
    int index;
    int file = 0;
    int rank = 7;
    int pos  = 0;
    char temp;
    char position[255];
    char som[1];
    char castle[4];
    char ep[2];
    char string[26] = {"  PNBRQK  pnbrqk/12345678"};

	sscanf(fen, "%s %s %s %s ", position, som, castle, ep);

    for(i=0;i<64;i++) {
        BINDEX[i] = boardEmpty;
    }
    BOARD[boardCR] = 0;
    for(i=0;i<12;i++) {
        BOARD[i] = 0;
    }

    i =0;
    while (!(rank==0 && file==8)) {
        temp = fen[i];
        i++;        
        for (j=0;j<25;j++) {
    		if (temp == string[j]) {
                if (j == 16) {
                    rank--;
                    file=0;
                }
                else if (j >=17) {
                    file+=j-16;
                }
                else {
                    pos = (rank*8) + file;
                    side = (j>7) ? 1 :0;
                    index = side? j-8 : j;
                    BOARD[side]  |= SetMaskBB[pos];
                    BOARD[index] |= SetMaskBB[pos];
                    BINDEX[pos] = index;
                    file++;
                }
                break;                
            }
        }
    }

    /* Castle Rights */
    if (castle[0] == 'K')
        setcastle_kingside(BOARD, WHITE);
    else
        clearcastle_kingside(BOARD, WHITE);
    if (castle[1] == 'Q')
        setcastle_queenside(BOARD, WHITE);
    else
        clearcastle_queenside(BOARD, WHITE);
    if (castle[2] == 'k')
        setcastle_kingside(BOARD, BLACK);
    else
        clearcastle_kingside(BOARD, BLACK);
    if (castle[3] == 'q')
        setcastle_queenside(BOARD, BLACK);
    else
        clearcastle_queenside(BOARD, BLACK);
     
    print_board(BOARD,BINDEX);
}


/* ############################# */
/* ###       print tools     ### */
/* ############################# */

void print_movealg(Move move) {

    char rankc[9] = "12345678";
    char filec[9] = "abcdefgh";
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
    if ( (pfrom == 0 && (to&56)/8 == 7) || (pfrom == 6 && (to&56)/8 == 0)) {
        if (pto == 4 || pto == 10)
            movec[4] = 'q';
        if (pto == 3 || pto == 9)
            movec[4] = 'r';
        if (pto == 2 || pto == 8)
            movec[4] = 'b';
        if (pto == 1 || pto == 7)
            movec[4] = 'n';
    }

    printf("move %s\n", movec);
    fflush(stdout);

}

void print_debug_movealg(Move move) {

    char rankc[9] = "12345678";
    char filec[9] = "abcdefgh";
    char movec[6] = "";
    Square from = getfrom(move);
    Square to   = getto(move);
    Piece pfrom   = getpfrom(move);
    Piece pto   = getpto(move);

/*
    printf("#from: %i ,to: %i, pfrom: %i ,pto: %i ,pcpt: %i ,cr: %i ,\n", getfrom(move), getto(move), getpfrom(move),getpto(move),getpcpt(move),getcr(move));
*/
    movec[0] = filec[square_file(from)];
    movec[1] = rankc[square_rank(from)];
    movec[2] = filec[square_file(to)];
    movec[3] = rankc[square_rank(to)];

    /* pawn promo */
    if ( (pfrom == 0 && (to&56)/8 == 7) || (pfrom == 6 && (to&56)/8 == 0)) {
        if (pto == 4 || pto == 10)
            movec[4] = 'q';
        if (pto == 3 || pto == 9)
            movec[4] = 'r';
        if (pto == 2 || pto == 8)
            movec[4] = 'b';
        if (pto == 1 || pto == 7)
            movec[4] = 'n';
    }


    printf("# %s\n", movec);
    fflush(stdout);
}

void print_bitboard(Bitboard board) {

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

void print_board(Bitboard *board, Boardindex *bindex) {

    int i,j,pos;
    char wpchars[10] = "  PNBRQK-";
    char bpchars[10] = "  pnbrqk-";

    printf("###ABCDEFGH###\n");
    for(i=8;i>0;i--) {
        printf("#%i ",i);
        for(j=0;j<8;j++) {
            pos = ((i-1)*8) + j;
            if (board[0] & SetMaskBB[pos] )
                printf("%c", wpchars[bindex[pos]]);
            else if (board[1] & SetMaskBB[pos] )
                printf("%c", bpchars[bindex[pos]]);
            else 
                printf("-");
       }
       printf("\n");
    }
    fflush(stdout);
}

void print_checkmate(int som) {
    if (som)
        printf("RESULT 0-1 {Black mates}\n");
    else
        printf("RESULT 1-0 {White mates}\n");
}

void print_draw_by_repetition() {
    printf("1/2-1/2 {Draw by repetition}\n");
}
void print_stalemate() {
    printf("1/2-1/2 {Stalemate}\n");
}



void print_stats() {
    FILE 	*Stats;
    Stats = fopen("zeta_amd.debug", "ab+");
    fprintf(Stats, "nodes: %lu ,moves: %lu ,beta-cuts: %i ,alpha-cuts: %i, TT_MOVE: %i ,TT_BETA: %i ,TT_ALPHA: %i ,sec: %f \n", NODECOUNT, MOVECOUNT,beta_cut, alpha_cut, TT_move,  TT_beta, TT_alpha, elapsed);
    fclose(Stats);
}

