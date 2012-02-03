#include <stdbool.h> /* bool, true and false */
#include "codes.h" /* because ncurses sucks more */
#include <stdlib.h> /* realloc, malloc, calloc */
#include <string.h> /* memmove */

#define LINESIZE 80

typedef struct Line Line;
struct Line {
	char *c; /* contents */
	int l; /* number of bytes */
	int m; /* capacity as multiple of LINESIZE */
	bool d; /* been modified since last draw */
	Line *n; /* next line or null */
	Line *p; /* prev line or null */
};

typedef struct { /* position in file */
	Line *l;
	int o; /* byte offset within the line (NB: not char due to multibyte utf8)*/
} Filepos;

typedef union { /* argument for callback funcs */
	Filepos p;
	const char *c;
} Arg;

typedef struct { /* key binding */
	char c[7]; /* key code to bind to */
	void (*func)(const Arg *arg); /* function to perform */
	const Arg arg; /* argument to func */
} Key;


/* Naughty global variables */
static Line *start=0, *end=0; /* first and last lines */
static Line *screen = 0; /* first line on screen */
static Filepos cur = { 0, 0 }; /* current position in file */
static tstate orig; /* original terminal state */

/** Internal functions **/
static Filepos i_insert(const char *c, Filepos pos); /* insert c at pos and return new filepos after the inserted char */
static int i_utf8len(const unsigned char *c); /* return number of bytes of utf char c */
static void i_setup(void); /* setup the terminal for editing */
static void i_tidyup(void); /* clean up and return the terminal to it's original state */

/** Movement functions **/
static Filepos m_bof(Arg arg); /* move to beginning of file */


#include "config.h"

/* internal function definitions */
Filepos /* insert c at post and return new filepos after the inserted char */
i_insert(const char *c, Filepos pos){
	int i;
	Line *l=pos.l, *ln=0;
	for( i=0; c[i] != '\0'; ++i ){
		/* FIXME handle actual copying, newlines are a special case */
		/* FIXME handle reallocation if necessary */
		if( c[i] == '\n' ){
			/* FIXME handle newline special case */
		} else {
			if( l->l >= (l->m * LINESIZE) ) /* FIXME need to take \0 at end of each string */
				l->c = (char*)realloc(l->c, LINESIZE*(++(l->m)));
			memmove(l->c+pos.o+1, l->c+pos.o, l->l - pos.o); /* FIXME check black magic */
			*(l->c+pos.o) = c[i];
			l->d = true;
			/* FIXME need a \0 at the end of each string */
		}
	}
}

int /* return number of bytes of utf char c */
i_utf8len(const unsigned char *c){
	if( *c >= 0xFC ) return 6;
	if( *c >= 0xF8 ) return 5;
	if( *c >= 0xF0 ) return 4;
	if( *c >= 0xE0 ) return 3;
	if( *c >= 0xC0 ) return 2;
	return 1;
}

void
i_setup(void){
	/* TODO write state to orig, modify local copy and set */
	orig = t_getstate();
	tstate new = orig;

}

void
i_tidyup(void){
	/* TODO restore tstate and stuff */
}

int /* the magic main function */
main(int argc, char **argv){
	/* TODO setup */
	i_setup();
	while( 1 ){
		/* TODO main loop, check input, see what happend next */
	}
	/* TODO tidy up */
	i_tidyup();
}
