#include <stdbool.h> /* bool, true and false */
#include "codes.h" /* because ncurses sucks */

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
	int o; /* character offset within the line */
} Filepos;

typedef struct { /* argument for callback */
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

/* commands that can be bounc to keys */

/* internal functions */
static Filepos i_insert(char c, Filepos pos); /* insert c at pos and return new filepos after the inserted char */

#include "config.h"

/* internal functions */
Filepos /* insert c at post and return new filepos after the inserted char */
i_insert(char c, Filepos pos){}


int /* the magic main function */
main(int argc, char **argv){
	
}
