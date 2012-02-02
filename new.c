#include <stdbool.h> /* bool, true and false */
#include "codes.h" /* because ncurses sucks more */

typedef struct Line Line;
struct Line {
	char *c; /* contents */
	int len; /* number of chars */
	bool dirty; /* been modified since last draw */
	Line *next; /* next line or null */
	Line *prev; /* prev line or null */
};

typedef struct { /* position in file */
	Line *l;
	int o; /* character offset within the line (NB: not byte offset as utf8)*/
} Filepos;

typedef union { /* argument for callback funcs */
    Filepos pos;
    const char *c;
} Arg;

typedef struct { /* key binding */
	char c[2]; /* key code to bind to */
	void (*func)(const Arg *arg); /* function to perform */
	const Arg arg; /* argument to func */
} Key;


/* Naughty global variables */
static Line *start, *end; /* first and last lines */
static Line *screen; /* first line on screen */
static Filepos cur; /* current position in file */

/** Internal functions **/
static Filepos i_insert(char *c, Filepos pos); /* insert c at pos and return new filepos after the inserted char */
static int i_utf8len(const unsigned char *c); /* return number of bytes of utf char c */

/** Movement functions **/
static Filepos m_bof(Arg arg); /* move to beginning of file */


#include "config.h"

/* internal function definitions */
Filepos /* insert c at post and return new filepos after the inserted char */
i_insert(char *c, Filepos pos){}

int /* return number of bytes of utf char c */
i_utf8len(const unsigned char *c){
    if( *ch >= 0xFC ) return 6;
    if( *ch >= 0xF8 ) return 5;
    if( *ch >= 0xF0 ) return 4;
    if( *ch >= 0xE0 ) return 3;
    if( *ch >= 0xC0 ) return 2;
    return 1;
}

int /* the magic main function */
main(int argc, char **argv){
	
}
