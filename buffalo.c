#include <stdbool.h> /* bool, true and false */
#include <stdlib.h> /* realloc, malloc, calloc */
#include <string.h> /* memmove, strlen, strcpy */
#include <unistd.h> /* write */
#include <stdio.h> /* puts, BUFSIZ */
#include <fcntl.h> /* open, close */
#include "codes.h" /* because ncurses sucks more */

#define LINESIZE 80

#define LENGTH(blah) ((int)(sizeof(blah)/sizeof*(blah)))

typedef struct Line Line;
struct Line {
	char *c; /* contents, \0 terminated and may include a \n */
	int len; /* number of utf8 chars (one ut8 char can be many bytes)*/
	int mul; /* capacity as multiple of LINESIZE */
	bool dirty; /* been modified since last draw */
	Line *next; /* next line or null */
	Line *prev; /* prev line or null */
};

typedef struct { /* position in file */
	Line *l;
	int o; /* byte offset within the line (NB: not char due to multibyte utf8)*/
} Filepos;

typedef union { /* argument for callback funcs */
	const char *c;
	Filepos (*m_func)(Filepos);
} Arg;

typedef struct { /* key binding */
	char c[7]; /* key code to bind to */
	void (*f_func)(const Arg *arg); /* function to perform */
	const Arg arg; /* argument to func */
} Key;


/* Naughty global variables */
static Line *fstart=0, *fend=0; /* first and last lines */
static Line *sstart=0, *send=0; /* first and last lines on screen */
static Filepos cur = { 0, 0 }; /* current position in file */
static tstate orig; /* original terminal state */
static char *curfile; /* current file name */

/** Internal functions **/
static Filepos i_insert(Filepos pos, const char *buf); /* insert buf at pos and return new filepos after the inserted char */
static int i_utf8len(const char *c); /* return number of bytes of utf char c */
static void i_setup(void); /* setup the terminal for editing */
static void i_tidyup(void); /* clean up and return the terminal to it's original state */
static void i_draw(void); /* draw lines from sstart until either the screen if full or we run out of lines */
static int i_loadfile(char *fname); /* initialise data structure and read in file */
static void i_die(char *c); /* reset terminal, print c to stderr and exit */

/** Movement functions **/
static Filepos m_startofline(Filepos pos);
static Filepos m_endofline(Filepos pos);
static Filepos m_startoffile(Filepos pos); /* move to beginning of file */
static Filepos m_endoffile(Filepos pos); /* move to end of file */
static Filepos m_prevchar(Filepos pos); /* move cursor left one char */
static Filepos m_nextchar(Filepos pos); /* move cursor right one char */
static Filepos m_prevline(Filepos pos); /* move cursor to previous line */
static Filepos m_nextline(Filepos pos); /* move cusor to next line */
static Filepos m_prevword(Filepos pos);
static Filepos m_nextword(Filepos pos);
static Filepos m_prevscreen(Filepos pos);
static Filepos m_nextscreen(Filepos pos);

/** Functions to bind to a key **/
static void f_cur(const Arg *arg); /* call arg.func(cur) and set cur to return value */


#include "config.h"

/* Function definitions */
void /* run the Arg with the current cursor position */
f_cur(const Arg *arg){
	cur = arg->m_func(cur);
}

/* Movement functions definitions */
Filepos /* move cursor left one char */
m_prevchar(Filepos pos){
	if( ! pos.l )
		return pos;
	if( --pos.o < 0 ){
		if( pos.l->prev ){
			pos.l = pos.l->prev;
			pos.o = pos.l->len;
		} else
			pos.o = 0;
	}
	return pos;
}

Filepos /* move cursor right one char */
m_nextchar(Filepos pos){
	if( ! pos.l )
		return pos;
	if( ++pos.o >= pos.l->len ){
		if( pos.l->next ){
			pos.l = pos.l->next;
			pos.o = 0;
		} else
			pos.o = pos.l->len;
	}
	return pos;
}

Filepos /* move cursor to previous line */
m_prevline(Filepos pos){
	if( ! pos.l || ! pos.l->prev )
		return pos;
	pos.l = pos.l->prev;
	if( pos.o > pos.l->len )
		pos.o = pos.l->len;
	return pos;
}

Filepos /* move cursor to next line */
m_nextline(Filepos pos){
	if( ! pos.l || ! pos.l->next )
		return pos;
	pos.l = pos.l->next;
	if( pos.o > pos.l->len )
		pos.o = pos.l->len;
	return pos;
}

void /* reset terminal, print error, and exit */
i_die(char *c){
	i_tidyup();
	fputs(c, stderr);
	exit(1);
}

/* internal function definitions */
Filepos /* insert c at post and return new filepos after the inserted char */
i_insert(Filepos pos, const char *buf){
	int i;
	Line *l=pos.l, *ln=0;
	char c;
	for( i=0, c=buf[0]; buf[i] != '\0'; c=buf[++i] ){
		if( c == '\n' || c == '\r' ){
			if( ! (ln = (Line *) malloc(sizeof(Line))) ) i_die("failed to malloc in insert");
			if( ! (ln->c = (char *) calloc(sizeof(char), LINESIZE * l->mul)) ) i_die("failed to calloc in insert");
			ln->mul = l->mul;
			ln->len = 0;
			ln->dirty = true;
			/* correct pointers */
			ln->prev = l;
			ln->next = l->next;
			if( ln->next )
				ln->next->prev = ln;
			l->next = ln;
			/* copy rest of line over, can call self recursively */
			i_insert((Filepos){ln, 0}, &(l->c[pos.o]));
			/* insert c followed by \0 */
			l->c[pos.o] = c;
			l->c[pos.o+1] = '\0';
			/* actually pos has to be the character after the \n, as in the first char off the new line */
			pos = (Filepos){ln, 0};
		} else {
			if( l->len <= LINESIZE*l->mul )
				if( ! (l->c = realloc(l->c, LINESIZE*(++l->mul))) ) i_die("failed to realloc in insert");
			/* memmove down the bus */
			if( ! memmove( &(l->c[pos.o+1]), &(l->c[pos.o]), l->len-pos.o) ) i_die("failed to memmove in insert");
			/* insert char */
			l->c[pos.o] = c;
			/* possibly make sure last char is \0, needed as testing if we are appending is more expensive than just doing */
			l->c[l->len+1] = '\0';
			/* mark dirty */
			l->dirty = true;
			/* correct len */
			++l->len;
			++pos.o;
		}
	}
	return pos; /* FIXME should point at last char inserted*/
}

int /* return number of bytes of utf char c */
i_utf8len(const char *c){
	if( (unsigned char)*c >= 0xFC ) return 6;
	if( (unsigned char)*c >= 0xF8 ) return 5;
	if( (unsigned char)*c >= 0xF0 ) return 4;
	if( (unsigned char)*c >= 0xE0 ) return 3;
	if( (unsigned char)*c >= 0xC0 ) return 2;
	return 1;
}

void
i_setup(void){
	t_getstate(&orig);
	tstate nstate = t_initstate(&orig);
	t_setstate(&nstate);
	t_clear();
	f_normal();
	c_line0();
}

void
i_tidyup(void){
	t_setstate(&orig);
	/* FIXME add back in post testing */
	/*t_clear();
		f_normal();
		c_line0(); */
}

void
i_draw(void){
	int h = t_getheight();
	int i=0;
	Line *l = sstart;
	t_clear();
	for( ; i<h && l; ++i, l=l->next)
		puts(l->c);

	for( ; i<h-1; ++i)
		puts("");
	c_line0();
	//c_goto(0, cur.o); /* FIXME */
}

void /** FIXME actual draw operation **/
i_ndraw(void){
	/* check curs is on screen, need to go from firstine to start of screen and start of screen to end of line to verify, then move as appropriate */
	/* once we know the curs is on the screen... */
	Line *l;
	int row, col, i;
	for(l=fstart, row=0; l && l != cur.l; l=l->next, ++row) ;
	for(i=0, col=0; i < cur.o; i += i_utf8len(&(cur.l->c[i])), ++col) ;
	/* we now have col and row that we can goto :) */
}

int /* initialise data structure and read in file */
i_loadfile(char *fname){
	int fd;
	char *buf=0;
	ssize_t n;

	if( ! cur.o ){
		/* initialise data structure */
		if( ! (fstart = (Line*) malloc(sizeof(Line))) ) i_die("failed to malloc in loadfile");
		fend = fstart;
		cur.l = fstart;
		cur.o = 0;
		sstart = fstart;
		send = fstart;
	}

	if( fname == 0 || fname[0] == '-' )
		fd = 0;
	else{
		if( (fd=open(fname, O_RDONLY)) == -1 )
			i_die("failed to open file in loadfile");
		curfile = strcpy(malloc(strlen(fname) + 1), fname);
	}

	if( (buf=calloc(1, BUFSIZ+1)) == 0 ) i_die("failed to calloc in loadfile");
	while( (n=read(fd, buf, BUFSIZ)) > 0){
		buf[n] = '\0';
		cur = i_insert(cur, buf);
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
	int i;
	int running=1; /* set to false to stop, FIXME make into a naughty global later */
	char ch[7]; /* characters to read into, 6 is maximum utf8 or terminal character.
								 7th place adds a nice \0 onto the end */

	i_setup();

	if( argc > 1 )
		i_loadfile(argv[1]);

	i_draw();
	while( running ){
		t_read(ch, 7);
		if( ch[0] == '!' )
			running=0;
		else if( ch[0] == 'a' )
			write(1, "a", 1);
		else if( ch[0] == '\n' )
			write(1, "\n", 1);
		if( i_utf8len(ch) > 1 ){
			/* FIXME have fun inserting me */
		} else if( ch[0] == 0x1B ){ /* FIXME change to constant to support CONTROL */
			for( i=0; i<LENGTH(keys); ++i )
				if( memcmp( ch, keys[i].c, sizeof(keys[i].c)) == 0 ){
					keys[i].f_func( &(keys[i].arg) );
					break;
				}
		} else { /* ascii character */
			/* FIXME insert */
		}

	}
	i_tidyup();
}
