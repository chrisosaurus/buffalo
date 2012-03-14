#include <stdbool.h> /* bool, true, false */
#include <stdlib.h> /* realloc, malloc, calloc */
#include <string.h> /* memmove, strlen, strcpy */
#include <unistd.h> /* write */
#include <stdio.h> /* puts, BUFSIZ */
#include <fcntl.h> /* open, close */
#include <signal.h> /* signal, raise, SIGSTOP, SIGCONT */
#include <sys/stat.h> /* S_* */
#include "codes.h" /* because ncurses sucks more */

#define LINESIZE 80

#define ISCTRL(ch) ((unsigned char)ch < 0x20)
#define ISALT(ch) ((unsigned char)ch == 0x1b)

#define LENGTH(blah) ((int)(sizeof(blah)/sizeof*(blah)))

typedef struct Line Line;
struct Line {
	char *c; /* contents, \0 terminated and may include a \n */
	int len; /* number of bytes excluding trailing \0 (not chars as utf8 chars are multibyte) */
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
static Line *sstart=0; /* first line on screen */
static Filepos cur = { 0, 0 }; /* current position in file */
static Filepos sels = {0, 0}, sele = {0, 0}; /* start and end of selection */
static tstate orig; /* original terminal state */
static char *curfile; /* current file name */
static int height=0, width=0; /* height last time we drew */
static Filepos mark = {0,0}; /* mark in file */

/** Internal functions **/
static Filepos i_insert(Filepos pos, const char *buf); /* insert buf at pos and return new filepos after the inserted char */
static int i_utf8len(const char *c); /* return number of bytes of utf char c */
static void i_setup(void); /* setup the terminal for editing */
static void i_tidyup(void); /* clean up and return the terminal to it's original state */
static void i_draw(void); /* force cursor to be on screen, make sure screen is correct size, then delegate to i_drawscr for actual drawing */
static void i_drawscr(bool sdirty, int crow, int ccol); /* draw all dirty lines on screen or draw all lines if sdirty */
static int i_loadfile(char *fname); /* initialise data structure and read in file */
static bool i_savefile(char *fname); /* write the fstart to fend to the file named in curfile */
static void i_die(char *c); /* reset terminal, print c to stderr and exit */
static Filepos i_backspace(Filepos pos); /* trivial backspace, delete prev char */
static void i_sigcont(int unused); /* what to do on a SIGCONT, used by f_suspend */

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
static void f_quit(const Arg *arg); /* ignore arg, tidyup and quit */
static void f_write(const Arg *arg); /* ignore arg, save file to curfile */
static void f_suspend(const Arg *arg); /* suspend to terminal */
static void f_mark(const Arg *arg); /* perform mark operation, g is goto, s is set, t(toggle) is set and goto old */

#include "config.h"

/* Function definitions */
void /* run the Arg with the current cursor position */
f_cur(const Arg *arg){
	cur = arg->m_func(cur);
}

void /* tidyup and quit */
f_quit(const Arg *arg){
	i_tidyup();
	exit(0);
}

void /* write file to curfile */
f_write(const Arg *arg){
	i_savefile(curfile);
}

void /* suspend to terminal */
f_suspend(const Arg *arg){
	t_clear();
	signal(SIGCONT, i_sigcont);
	raise(SIGSTOP);
}

void /* mark operations, determine which by arg->c g is goto mark, s is set mark, t(toggle) is set and goto */
f_mark(const Arg *arg){
	if( arg->c[0] == 'g' ){
		if( mark.l )
			cur = mark;
	} else if( arg->c[0] == 's' )
		mark = cur;
	else if( arg->c[0] == 't' ){
		Filepos nmark = cur;
		cur = mark;
		mark = nmark;
	}
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
	if( ++pos.o > pos.l->len ){
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

Filepos /* move cursor to start of line */
m_startofline(Filepos pos){
	if( ! pos.l )
		return pos;
	pos.o = 0;
	return pos;
}

Filepos /* move cursor to end of line */
m_endofline(Filepos pos){
	if( ! pos.l )
		return pos;
	pos.o = pos.l->len;
	return pos;
}

Filepos /* move cursor to start of file */
m_startoffile(Filepos pos){
	if( ! pos.l || ! fstart )
		return pos;
	pos.l = fstart;
	pos.o = 0;
	return pos;
}

Filepos /* move cursor to end of file */
m_endoffile(Filepos pos){
	if( ! pos.l || ! fend )
		return pos;
	pos.l = fend;
	pos.o = pos.l->len;
	return pos;
}

Filepos /* move cursor to next space */
m_prevword(Filepos pos){
	if( ! pos.l )
		return pos;
	for( pos = m_prevchar(pos); pos.l->c[pos.o] != ' '; pos = m_prevchar(pos));
	return pos;
}

Filepos /* move cursor to next space */
m_nextword(Filepos pos){
	if( ! pos.l )
		return pos;
	for( pos = m_nextchar(pos); pos.l->c[pos.o] != ' '; pos = m_nextchar(pos));
	return pos;
}

Filepos /* move cursor to next screen */
m_nextscreen(Filepos pos){
	/* FIXME
	if( ! pos.l || ! send )
		return pos;
	pos.o = 0;
	pos.l = send->next ? send->next : send ;
	*/
	return pos;
}

Filepos /* move cursor to prev screen */
m_prevscreen(Filepos pos){
	/* FIXME
	if( ! pos.l || ! sstart )
		return pos;
	pos.o = 0;
	pos.l = sstart->prev ? sstart->prev : sstart;
	*/
	return pos;
}

void /* what to do on a SIGCONT */
i_sigcont(int unused){
	height=0;
	i_draw();
}

void /* reset terminal, print error, and exit */
i_die(char *c){
	i_tidyup();
	fputs(c, stderr);
	fflush(stderr);
	exit(1);
}

/* internal function definitions */
Filepos /* insert c at post and return new filepos after the inserted char */
i_insert(Filepos pos, const char *buf){
	int i;
	Line *l=pos.l, *ln=0;
	char c;
	if( ! l )
		return pos;
	for( i=0, c=buf[0]; buf[i] != '\0'; c=buf[++i] ){
		if( c == '\n' || c == '\r' ){
			if( ! (ln = (Line *) malloc(sizeof(Line))) ) i_die("failed to malloc in insert");
			if( ! (ln->c = (char *) calloc(sizeof(char), LINESIZE * l->mul)) ) i_die("failed to calloc in insert");
			ln->mul = l->mul;
			ln->len = 0;
			ln->c[0] = '\0';
			ln->dirty = true;
			/* correct pointers */
			ln->prev = l;
			ln->next = l->next;
			if( l->next )
				l->next->prev = ln;
			l->next = ln;
			/* copy rest of line over, can call self recursively */
			if( pos.o < l->len )
				i_insert((Filepos){ln, 0}, &(l->c[pos.o]));
			/* insert c followed by \0 */
			l->c[pos.o] = '\0';
			l->len = pos.o;
			l->dirty = true;
			/* possibly need to correct fend if we have gone past it */
			if( l == fend )
				fend = ln;
			/* actually pos has to be the character after the \n, as in the first char of the new line */
			l = ln;
			pos = (Filepos){l, 0};
			height = 0; /* insertin a \n requires a redraw of the screen */
		} else {
			if( l->len+2 >= LINESIZE*l->mul )
				if( ! (l->c = realloc(l->c, LINESIZE*(++l->mul))) ) i_die("failed to realloc in insert");
			/* memmove down the bus */
			if( pos.o < l->len )
				if( ! memmove( &(l->c[pos.o+1]), &(l->c[pos.o]), (l->len-pos.o)) ) i_die("failed to memmove in insert");
			/* insert char */
			l->c[pos.o] = c;
			/* correct len */
			++l->len;
			/* possibly make sure last char is \0, needed as testing if we are appending is more expensive than just doing */
			l->c[l->len] = '\0';
			/* mark dirty */
			l->dirty = true;
			/* move along */
			++pos.o;
		}
	}
	return pos; /* FIXME should point at last char inserted*/
}

Filepos /*trivial backspace */
i_backspace(Filepos pos){
	if( ! pos.l )
		return pos;

	if( pos.o <= 0 ){
		if( ! pos.l->prev ) return pos;
		Line *l = pos.l->prev;
		int nl = pos.l->len + l->len;
		l->mul = nl / LINESIZE +1;

		if( ! (l->c = realloc(l->c, l->mul*LINESIZE)) ) i_die("failed to realloc in i_backspace\n");
		if( ! (memcpy( &(l->c[l->len]), pos.l->c, pos.l->len+1 )) ) i_die("failed to memcpy in i_backspace\n");

		pos.o = l->len;
		l->len = nl;
		l->next = pos.l->next;
		l->dirty = true;
		if( l->next )
			l->next->prev = l;
		free(pos.l->c);
		free(pos.l);
		pos.l = l;
	} else {
		if( ! memmove( &(pos.l->c[pos.o-1]), &(pos.l->c[pos.o]), (pos.l->len - pos.o)+1 ) )
		 i_die("failed to memmove in i_backspace\n");	/* FIXME off by one in length? */
		--pos.o;
		--pos.l->len;
	}
	height = 0; /* set height to 0 to indicate sdirty */
	return pos;
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
	t_clear();
	f_normal();
	c_line0();
	fflush(stdout);
}

void /* perform actual drawing to screen */
i_odraw(void){
	int nh = t_getheight();
	int i=0, crow=0, ccol=0;

	t_clear();
	c_line0();

	if( ! sstart )
		sstart = fstart;

	/* FIXME force cursor to be on screen, TODO work out if we can calculate crow and ccol here */
	Line *l = sstart;
	for( ; i<(nh-1) && l; ++i, l=l->next ){
		if( l == cur.l )
			crow = i;
		fputs(l->c, stdout);
		l->dirty = false;
	}
	if( i == nh-1 ){
		write(1, l->c, l->len-1); /* FIXME ideally we should get rid of this in favour of buffered */
		l->dirty = false;
		if( l == cur.l )
			crow = nh;
	}

	/* find cursor column */
	for(i=0, ccol=0; i < cur.o; i += i_utf8len(&(cur.l->c[i])), ++ccol) ;
	c_goto(crow+1, ccol+1); /* FIXME should I move this elsewhere */
	fflush(stdout); /* FIXME need to add a note to codes about how fflush-ing is needed */
}

void /* draw all dirty lines on screen or draw all lines if sdirty */
i_drawscr(bool sdirty, int crow, int ccol){
	Line *l;
	int n=1, c=0, i=0; /* n is line number, c is the char counter, i is used within the printing loop */

	c_line0();
	for( n=1, l=sstart; l && n<height; l=l->next, ++n ){
		if( l == cur.l ){
			c_clearline();
			b_blue();
			for( c=0; c<l->len && c<width; ++c ){
				if( l->c[c] == '\t' )
					for( i=0; i<TABSTOP; ++i)
						fputc(' ', stdout);
				else
					fputc(l->c[c], stdout);
			}
			b_default();
			l->dirty = true;
		} else if( l->dirty || sdirty ){
			c_clearline();
			for( c=0; c<l->len && c<width; ++c ){
				if( l->c[c] == '\t' )
					for( i=0; i<TABSTOP; ++i)
						fputc(' ', stdout);
				else
					fputc(l->c[c], stdout);
			}
			l->dirty = false;
		}
		c_nline();
	}
	for( ; n<height; ++n ){
		c_nline();
		c_clearline();
	}
	c_goto(crow, ccol);
	fflush(stdout);
	return;
}

void /* make sure cursor is on screen and screen is correct size, delegate to i_drawscr for actual drawing */
i_draw(void){
	int nh = t_getheight(), nw = t_getwidth();
	bool sdirty = false; /* is the entire range sstart->send dirty */
	int i=0, ccol=0; /* i is used as a general counter and as crow */
	Line *l;

	if( ! fstart )
		return ; /* FIXME if we havent loaded a file yet */

	if( ! sstart )
		sstart = fstart;

	/* if the width or height has changed, every line needs to be redrawn so we can see the missing characters */
	if( nh != height || nw != width ){
		sdirty = true;
		height = nh;
		width = nw;
	}

	/* find cursor column */
	for(i=0, ccol=1; i < cur.o; i += i_utf8len(&(cur.l->c[i]))){
		if( cur.l->c[i] == '\t' )
			ccol += TABSTOP;
		else
			++ ccol;
	}

	/* handle the three cases of cursor position; on screen, before screen, and after screen resp. */
	for( l=sstart, i=1; l && i < nh; ++i, l=l->next )
		if( l == cur.l ){
			i_drawscr(sdirty, i, ccol);
			return;
		}
	/* continue searching off screen using old l */
	for( i=1; l; ++i, l=l->next ){
		if( l == cur.l ){
			if( i > nh ){
				/* if i is greater than screen heights, scrolling wont save us anything, so have to redraw
				 * print lines such that h/2 is cur.l */
				/* FIXME adjust sstart and send, set dirty lines */
			} else {
				/*	   scroll up by i
				 *	   goto start
				 *	   draw i lines - draw last highlighted
				 */
				/* FIXME adjust sstart and send, set dirty lines */
			}
			sdirty = true;
			sstart = l; /* FIXME temporary hack */
			i_drawscr(sdirty, i, ccol);
			return;
		}
	}
	for( l=fstart, i=1; l!=sstart && l->next; ++i, l=l->next ){
		if( l == cur.l ){
			if( i > nh ){
				/* if i is greater than screen heights, scrolling wont save us anything, so have to redraw
				 * print lines such that h/2 is cur.l */
				/* FIXME adjust sstart and send, set dirty lines */
			} else {
				/*    scroll down by i
				 *     goto sstart
				 *     draw i line - draw first highlighted
				 */
				/* FIXME adjust sstart and send, set dirty lines */
			}
			sdirty = true;
			sstart = l; /* FIXME temporary hack */
			i_drawscr(sdirty, i, ccol);
			return;
		}
	}
	i_die("impossible case occured in i_draw, *BOOM*\n");
}

int /* initialise data structure and read in file */
i_loadfile(char *fname){
	int fd;
	char *buf=0;
	ssize_t n;

	if( ! fstart ){
		/* initialise data structure */
		if( ! (fstart = (Line*) malloc(sizeof(Line))) ) i_die("failed to malloc in loadfile");
		if( ! (fstart->c = (char*) calloc(LINESIZE, sizeof(char))) ) i_die("failed to malloc in loadfile");
		fstart->mul = 0;
		fstart->len = 0;
		fstart->c[0] = '\0';
		fend = fstart;
		cur.l = fstart;
		cur.o = 0;
	}

	if( fname == 0 || fname[0] == '-' )
		fd = 0;
	else{
		if( (fd=open(fname, O_RDONLY)) == -1 )
			; /* FIXME new file, inform user */
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

bool /* write fstart to fend to file named in curfile */
i_savefile(char *fname){
	int fd;
	Line *l;
	bool error = false;

	if( ! fstart )
		return false;

	if( (fd=open(fname, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) == -1 )
		i_die("failed to open file for writing in savefile");

	for( l=fstart; l; l=l->next )
		if( write(fd, l->c, l->len) == -1 || write(fd, "\n", 1) == -1){
			error = true;
			break;
		}

	return error;
}

int /* the magic main function */
main(int argc, char **argv){
	int i;
	int running=1; /* set to false to stop, FIXME make into a naughty global later */
	char ch[7]; /* 6 is maximum len of utf8, 7th adds a nice \0 FIXME but it this needed? */

	i_setup();

	if( argc > 1 )
		i_loadfile(argv[1]);

	while( running ){
		i_draw();
		t_read(ch, 7);
		if( i_utf8len(ch) > 1 ){
			cur = i_insert(cur, ch);
		} else if( ch[0] == 127 ){
			cur = i_backspace(cur);
		} else if( ch[0] == 10 && ch[1] == 0 ){
			cur = i_insert(cur, ch); /* FIXME \n special case */
		} else if( ch[0] == 9 && ch[1] == 0 ){
			cur = i_insert(cur, ch); /* FIXME \t special case */
		} else if( ISALT(ch[0]) || ISCTRL(ch[0]) ){
			for( i=0; i<LENGTH(keys); ++i )
				if( memcmp( ch, keys[i].c, sizeof(keys[i].c)) == 0 ){
					keys[i].f_func( &(keys[i].arg) );
					break;
				}
		} else { /* ascii character */
			cur = i_insert(cur, ch);
		}

	}
	i_tidyup();
}

