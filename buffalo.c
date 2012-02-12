#include <stdbool.h> /* bool, true and false */
#include <stdlib.h> /* realloc, malloc, calloc */
#include <string.h> /* memmove */
#include <unistd.h> /* write */
#include <stdio.h> 
#include "codes.h" /* because ncurses sucks more */

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
static Line *fstart=0, *fend=0; /* first and last lines */
static Line *vstart=0, *vend=0; /* first and last lines on screen */
static Filepos cur = { 0, 0 }; /* current position in file */
static tstate orig; /* original terminal state */
static char *curfile; /* current file name */

/** Internal functions **/
static Filepos i_insert(const char *c, Filepos pos); /* insert c at pos and return new filepos after the inserted char */
static int i_utf8len(const unsigned char *c); /* return number of bytes of utf char c */
static void i_setup(void); /* setup the terminal for editing */
static void i_tidyup(void); /* clean up and return the terminal to it's original state */
static void i_draw(void); /* draw lines from vstart until either the screen if full or we run out of lines */
static int i_loadfile(char *fname); /* initialise data structure and read in file */

/** Movement functions **/
static Filepos m_bof(Arg arg); /* move to beginning of file */
static Filepos m_eof(Arg arg); /* move to end of file */
static Filepos m_lc(Arg arg); /* move cursor left one char */
static Filepos m_rc(Arg arg); /* move cursor right one char */
static Filepos m_pl(Arg arg); /* move cursor to previous line */
static Filepos m_nl(Arg arg); /* move cusor to next line */


#include "config.h"

/* Movement functions definitions */
Filepos /* move cursor left one char */
m_lc(Arg arg){
	if( ! arg.p.l || ! arg.p.o)
		return arg.p;
	--arg.p.o;
	c_left();
	return arg.p;
}

Filepos /* move cursor right one char */
m_rc(Arg arg){
	if( ! arg.p.l )
		return arg.p;
	if( arg.p.o >= arg.p.l->l )
		return arg.p;
	++arg.p.o;
	c_right();
	return arg.p;
}

Filepos /* move cursor to previous line */
m_pl(Arg arg){
	if( ! arg.p.l || ! arg.p.l->p )
		return arg.p;
	arg.p.l = arg.p.l->p;
	if( arg.p.o > arg.p.l->l )
		arg.p.o = arg.p.l->l;
	c_scrlu(); /* FIXME pline, up, or scrlu ? */
	return arg.p;
}

Filepos /* move cursor to next line */
m_nl(Arg arg){
	if( ! arg.p.l || ! arg.p.l->n )
		return arg.p;
	arg.p.l = arg.p.l->n;
	if( arg.p.o > arg.p.l->l )
		arg.p.o = arg.p.l->l;
	c_scrld(); /* FIXME nline, down, or scrld ? */
	return arg.p;
}

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
	t_getstate(&orig);
	tstate nstate = t_initstate(&orig);
	t_setstate(&nstate);
	t_clear();
	f_default();
	c_line0();
}

void
i_tidyup(void){
	t_setstate(&orig);
    /* FIXME add back in post testing */
	/*t_clear();
	f_default();
	c_line0(); */
}

void
i_draw(void){
	int h = t_getheight();
	int i=0;
	Line *l = vstart;
	t_clear();
	for( ; i<h, l; ++i, l=l->n)
		puts(l->c);

	for( ; i<h-1; ++i)
		puts("");
	c_line0();
	//c_goto(0, cur.o); /* FIXME */
}

int /* initialise data structure and read in file */
i_loadfile(char *fname){
	int fd;
	char *buf=0;
	ssize_t n;


	if( fname == 0 || fname[0] == '-' )
		fd = 0;
	else{
		if( (fd=open(fname, "r")) == -1 )
			return -1; /* FIXME can't open file */
		curfile = strdup(fname);
	}

	if( (buf=calloc(1, BUFSIZ+1)) == 0 ) return -1; /* FIXME can't malloc */
	while( (n=read(fd, buf, BUFSIZ)) > 0){
		buf[n] = '\0';
		cur = i_insert(buf, cur);
	}

	if( fd != 0 )
		close (fd);

	free(buf);
	cur.l = fstart;
	cur.o = 0;
	return 0;
}

int /* the magic main function */
main(int argc, char **argv){
	char ch[7]; /* characters to read into, 6 is maximum utf8 or terminal character.
                   7th place adds a nice \0 onto the end */

	i_setup();
	/* FIXME testing data */
	vstart = (Line *) malloc( sizeof(Line) );
	vstart->c = "hello ";
	vstart->l = 6;
	vstart->n = (Line *) malloc( sizeof(Line) );
	vstart->n->p = vstart;
	vstart->n->c = "world";
	vstart->n->l = 5;
	vstart->n->n = (Line *) malloc(sizeof(Line) );
	vstart->n->n->p = vstart->n;
	vstart->n->n->c = "DUDE";
	vstart->n->n->l = 4;
	vend = vstart->n->n;

	cur.l = vstart;

	i_draw();
	while( 1 ){
		t_read(ch, 7);
		if( ch[0] == '!' )
			break;
		else if( ch[0] == 'h' )
			cur = m_lc( (Arg) {.p=cur} );
		else if( ch[0] == 'j')
			cur = m_nl( (Arg) {.p=cur} );
		else if( ch[0] == 'k')
			cur = m_pl( (Arg) {.p=cur} );
		else if( ch[0] == 'l')
			cur = m_rc( (Arg) {.p=cur} );
		else if( ch[0] == 'a' )
			write(1, "a", 1);
		else if( ch[0] == '\n' )
			write(1, "\n", 1);
		/* TODO main loop, check input, see what happend next */
	}
	i_tidyup();
}
