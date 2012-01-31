#include <stdbool.h> /* bool, true and false */

typedef struct Line Line;
struct Line {
	char *c; /* contents */
	size_t len; /* number of chars */
	bool dirty; /* been modified since last draw */
	Line *next; /* next line or null */
	Line *prev; /* prev line or null */
};

typedef struct { /* position in file */
	Line *l;
	size_t o; /* character offset within the line */
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


