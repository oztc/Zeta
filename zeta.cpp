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
#include <unistd.h>


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
static int force_mode   = false;
int random_mode         = false;


clock_t start, end;
double elapsed;


const Score  INF = 32000;

Piece BOARD[129];

Move MOVES[100*128*128];


Move bestmove = 0;

Move Lastmove = 0;


// functions
Move move_parser(char *usermove, Piece *board, int som);
void setboard(char *fen);
void print_movealg(Move move);
void print_board(Piece *board);
void print_stats();


// cl functions
extern int load_file_to_string(const char *filename, char **result);
extern int initializeCL(Piece *board);
extern int  runCLKernels(unsigned int som, Move lastmove, unsigned int maxdepth);
extern int   cleanupCL(void);


/* ############################# */
/* ###        inits          ### */
/* ############################# */


/* ############################# */
/* ###     move tools        ### */
/* ############################# */

inline Move makemove(Square from, Square to, Piece pcpt, Piece promo) {
    return (from | (Move)to<<8 |  (Move)pcpt <<16 |  (Move)promo <<24);  
}
inline Square getfrom(Move move) {
    return (move & 0xFF);
}
inline Square getto(Move move) {
    return ((move>>8) & 0xFF);
}
inline Piece getpcpt(Move move) {
    return ((move>>16) & 0xFF);
}
inline Piece getpromo(Move move) {
    return ((move>>24) & 0xFF);
}

static inline Square make_square(int f, int r) {
  return ( f |  (r << 3));
}

static inline int square_file(Square s) {
  return (s & 7);
}

static inline int square_rank(Square s) {
  return (s >> 3);
}



/* ############################# */
/* ###     domove undomove   ### */
/* ############################# */

void domove(Piece *board, Move move, int som) {

    // set to, unset capture
    if (getpromo(move) != PEMPTY)
        board[getto(move)] = getpromo(move);        
    else
        board[getto(move)] = board[getfrom(move)];        
    // unset from
    board[getfrom(move)] = PEMPTY;        

    // Todo: handle en passant and castle


}

void undomove(Piece *board, Move move, int som) {


    // restore from
    board[getfrom(move)] = board[getto(move)];        
    // restore capture
    board[getto(move)] = board[getpcpt(move)];        

    // Todo: handle en passant and castle

}

/* ############################# */
/* ###      root search      ### */
/* ############################# */

Move rootsearch(Piece *board, int som, int depth, Move lastmove) {

    int status =0;
    bestmove = 0;

    start = clock();

    status = initializeCL(board);

    status = runCLKernels(som, lastmove, depth);

    end = clock();
    elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;


printf("\n# from %i\n", getfrom(bestmove));
printf("\n# to %i\n", getto(bestmove));
    
printf("\n # BOARD %i \n", board[96]);

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
			printf("feature myname=\"Zeta 011 \" reuse=0 colors=1 setboard=1 memory=1 smp=1 usermove=1 san=0 time=0 debug=1 \n");
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

Move move_parser(char *usermove, Piece *board, int som) {

    int file;
    int rank;
    Square from,to;
    Piece promo = PEMPTY;
    Piece pcpt = PEMPTY;
    Move move;
    char promopiece;

    file = (int)usermove[0] -97;
    rank = (int)usermove[1] -49;
    from = make_square(file,rank);
    file = (int)usermove[2] -97;
    rank = (int)usermove[3] -49;
    to = make_square(file,rank);

    from = ((from&56)/8)*16 + (from&7);
    to   = ((to&56)/8)*16 + (to&7);

    pcpt = board[to];

    // pawn promo piece
    promopiece = usermove[4];
    if (promopiece == 'q' || promopiece == 'Q' )
        promo = QUEEN;
    else if (promopiece == 'n' || promopiece == 'N' )
        promo = KNIGHT;
    else if (promopiece == 'b' || promopiece == 'B' )
        promo = BISHOP;
    else if (promopiece == 'r' || promopiece == 'R' )
        promo = ROOK;

    move = makemove(from, to, pcpt, (promo|som));

    return move;
}

void setboard(char *fen) {

/*
((pos&56)/8)*16 + (pos&7);

(pos88/16)*8 + pos88%16
*/

    int i, j, side;
    Square ep_sq;
    int index;
    int file = 0;
    int rank = 7;
    int pos  = 0;
    int pos88 = 0;
    char temp;
    char position[255];
    char csom;
    char castle[5];
    char ep[3];
    int bla;
    int blubb;
    char string[26] = {" P NKBRQ  pnkbrq/12345678"};

	sscanf(fen, "setboard %s %c %s %s %i %i", position, &csom, castle, ep, &bla, &blubb);


    for(i=0;i<64;i++) {
        BOARD[i] = PEMPTY;
    }
    i =0;
    while (!(rank==0 && file==8)) {
        temp = position[i];
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
                    pos88 = rank*16 + file;
                    side = (j>7) ? 1 :0;
                    index = side? j-8 : j;
                    BOARD[pos88] = (side)? (index|BLACK) : (index|WHITE);
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

    char rankc[9] = "12345678";
    char filec[9] = "abcdefgh";
    char movec[5] = "";
    Square from = getfrom(move);
    Square to   = getto(move);
    Piece promo   = getpromo(move);

    from = (from/16)*8 + from%16;
    to   = (to/16)*8 + to%16;

    movec[0] = filec[square_file(from)];
    movec[1] = rankc[square_rank(from)];
    movec[2] = filec[square_file(to)];
    movec[3] = rankc[square_rank(to)];

    // pawn promo
    if ( promo) {
        if (promo == QUEEN )
            movec[4] = 'q';
        if (promo == ROOK )
            movec[4] = 'r';
        if (promo == BISHOP )
            movec[4] = 'b';
        if (promo == KNIGHT )
            movec[4] = 'n';
    }

    printf("move %s\n", movec);
    fflush(stdout);

}

void print_board(Piece *board) {

    int i,j,pos;
    int pos88;
    char wpchars[10] = "-P NKBRQ";
    char bpchars[10] = "- pnkbrq";

    printf("###ABCDEFGH###\n");

    for(i=8;i>0;i--) {
        printf("#%i ",i);
        for(j=0;j<8;j++) {
            pos = ((i-1)*8) + j;
            pos88 = ((pos&56)/8)*16 + (pos&7);

            if (board[pos88]&BLACK)
                printf("%c", bpchars[board[pos88]&7]);
            else if (board[pos88]&WHITE)
                printf("%c", wpchars[board[pos88]&7]);
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

