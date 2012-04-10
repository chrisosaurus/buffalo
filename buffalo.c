#include <stdbool.h> /* bool, true, false */
#include <stdlib.h> /* realloc, malloc, calloc */
#include <string.h> /* memmove, strlen, strcpy */
#include <unistd.h> /* write */
#include <stdio.h> /* puts, BUFSIZ */
#include <fcntl.h> /* open, close */
#include <signal.h> /* signal, raise, SIGSTOP, SIGCONT */
#include <sys/stat.h> /* S_* */
#include <regex.h> /* regcomp, regexec, regfree */
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
	const int i;
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
static tstate orig, nstate; /* original and new terminal state */
static char *curfile; /* current file name */
static int height=0, width=0; /* height last time we drew */
static Filepos mark = {0,0}; /* mark in file */
static bool modified=false; /* has the file been modified since last save or load */
static Line *buffer; /* current 'copied' buffer */

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
static Line* i_newline(int mul); /* return new line containing mul * LINESIZE chars */

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
static void f_quit(const Arg *arg); /* tidyup and quit, if arg->c==0 then will not exit with modifications, otherwise will exit regardless */
static void f_write(const Arg *arg); /* ignore arg, save file to curfile */
static void f_suspend(const Arg *arg); /* suspend to terminal */
static void f_mark(const Arg *arg); /* perform mark operation based on arg->i, 0 is set, 1 is set & goto old */
static void f_sel(const Arg *arg); /* perform selecton operation based on arg->i, 0 is set end, 1 is set start, 2 is clear both */
static void f_newl(const Arg *arg); /* insert new line either before (arg->i == 0), or after (arg->i == 1) current line and set cur to first char of newline*/
static void f_cut(const Arg *arg); /* remove selection and copy into buffer */
static void f_copy(const Arg *arg); /* copy selection into buffer */
static void f_paste(const Arg *arg); /* paste contents of buffer at cursor */
static void f_del(const Arg *arg); /* delete selection, ignore arg */
static void f_align(const Arg *arg); /* align cursor line to top of screen */
static void f_goto(const Arg *arg); /* goto line specified in buffer, ignore arg */
static void f_searchf(const Arg *arg); /* search forward using regex in buffer */
static void f_searchb(const Arg *arg); /* search backwards using regex in buffer */

#include "config.h"

/* Function definitions */
void /* run the Arg with the current cursor position */
f_cur(const Arg *arg){
	if( cur.l )
		cur.l->dirty = true;
	cur = arg->m_func(cur);
	cur.l->dirty = true;
}

void /* tidyup and quit */
f_quit(const Arg *arg){
	if( arg->i == 0 && modified )
		return;
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
	fflush(stdout);
	signal(SIGCONT, i_sigcont);
	raise(SIGSTOP);
}

void /* mark operations, determine which by arg->c: 0 is set, 1 is set & goto */
f_mark(const Arg *arg){
	if( arg->i ){
		Filepos nmark = cur;
		if( mark.l )
			cur = mark;
		mark = nmark;
		mark.l->dirty = true;
	} else
		mark = cur;
	cur.l->dirty = true;
}

void /* selection operations, determine which by arg->i: 0 is set sele, 1 is set sels, 2 is clear */
f_sel(const Arg *arg){
	Line *l=0;
	height = 0;
	switch( arg->i ){
		case 0:
			sele = cur;
			/* sels < sele, otherwise unset sels */
			if( ! sels.l )
				return;
			/* sels < sele, otherwise unset sele FIXME or should I unset sels as sele is the most recently placed? */
			if( sels.l == sele.l && sels.o >= sele.o )
				sele = (Filepos){0,0};
			else
				for( l=sels.l; l!=sele.l; l=l->next )
					if( l == fend ){
						sele = (Filepos){0, 0};
						break;
					}
			break;
		case 1:
			sels = cur;
			/* sels < sele, otherwise unset sele */
			if( ! sele.l )
				return;
			if( sels.l == sele.l && sels.o >= sele.o )
				sele = (Filepos){0,0};
			else
				for( l=sels.l; l!=sele.l; l=l->next )
					if( l == fend ){
						sele = (Filepos){0, 0};
						break;
					}
			break;
		case 2:
			sele = (Filepos){0, 0};
			sels = (Filepos){0, 0};
			break;
	}
}

void /* insert newline before (arg->i == 0) or after (arg->i == 1) */
f_newl(const Arg *arg){
	cur.l->dirty = true;
	height = 0; /* sdirty */
	Line *l = i_newline(1);
	modified = true;
	if( arg->i ){
		l->prev = cur.l;
		l->next = cur.l->next;
		if( cur.l->next )
			cur.l->next->prev = l;
		else
			fend = l;
		cur.l->next = l;
		cur.l = l;
		cur.o = 0;
	} else {
		l->next = cur.l;
		l->prev = cur.l->prev;
		if( cur.l->prev )
			cur.l->prev->next = l;
		else
			fstart = l;
		cur.l->prev = l;
		cur.l = l;
		cur.o = 0;
	}
}

void /* copy contents of selection into buffer */
f_copy(const Arg *arg){
	Line *l=0;
	int i=0, c=0; /* i is position in a line, c is count of chars copied */
	if( ! sels.l || ! sele.l )
		return;
	for( l=sels.l, i=sels.o; l && (sele.o != i || sele.l != l) ; ++i, ++c ){
		/* copy each char into buffer->c[c] if there is room (c < buffer->mul * LINESIZE ) */
		if( (c+1) > (buffer->mul * LINESIZE) )
			if( ! (buffer->c = realloc(buffer->c, LINESIZE*(++buffer->mul)) ) )
				i_die("realloc failed in f_copy\n");
		if( i >= l->len ){
			buffer->c[c] = '\n';
			i = -1;
			l=l->next;
		} else
			buffer->c[c] = l->c[i];
	}
	buffer->len = c;
	buffer->c[c] = '\0';
}

void /* cut contents of selection into buffer */
f_cut(const Arg *arg){
	f_copy(arg);
	f_del(arg);
}

void /* paste contents of buffer at cursor */
f_paste(const Arg *arg){
	if( buffer->len )
		cur = i_insert(cur, buffer->c);
}

void /* delete selection, ignore arg */
f_del(const Arg *arg){
	Line *l=0, *lb=0; /* line and line backup */
	if( ! sels.l || ! sele.l )
		return;
	if( sels.l == sele.l) {
		memmove( &(sels.l->c[sels.o]), &(sele.l->c[sele.o]), sele.l->len - sele.o );
		sels.l->len = sels.o + (sele.l->len - sele.o);
		sels.l->c[sels.l->len] = '\0';
	} else {
		for( l=sels.l->next; l && l != sele.l ; ){
			if( l->prev )
				l->prev->next = l->next;
			if( l->next )
				l->next->prev = l->prev;
			free(l->c);
			lb = l->next;
			free(l);
			l = lb;
		}
		i_insert( sels, &(sele.l->c[sele.o]) );
		if( sele.l->prev )
			sele.l->prev->next = sele.l->next;
		if( sele.l->next )
			sele.l->next->prev = sele.l->prev;
		sels.l->len = sels.o + (sele.l->len - sele.o);
		sels.l->c[sels.l->len] = '\0';
		free(sele.l->c);
		free(sele.l);
	}
	/* tidy up */
	cur = sels;
	f_sel(&(Arg){.i=2}); /* clear selection */
}

void /* align cursor line to top of screen */
f_align(const Arg *arg){
	sstart = cur.l;
	height = 0; /* sdirty */
}

void /* goto line specifed in buffer, ignore arg */
f_goto(const Arg *arg){
	int ln = atoi(buffer->c);
	int i=1;
	Line *l=0;

	if( cur.l )
		cur.l->dirty = true;
	for( i=1, l=fstart; l->next && i < ln; ++i, l=l->next ) ;
	cur.l = l;
	cur.o = 0;
}

void /* set mark and search forward using regex in buffer */
f_searchf(const Arg *arg){
	regex_t re;
	regmatch_t matches[1];
	Line *l=cur.l;
	int status=REG_NOMATCH;

	f_mark( &(Arg){.c=0} );

	if( regcomp( &re, buffer->c, REG_EXTENDED ) )
		i_die("regcomp failed in f_searchf");

	if( cur.o ) /* if not start of line */
		status= regexec( &re, &(cur.l->c[cur.o+1]), 1, matches, REG_NOTBOL );
	else
		status= regexec( &re, &(cur.l->c[cur.o+1]), 1, matches, 0 );

	if( status == REG_NOMATCH )
		for( l=cur.l->next; l && status==REG_NOMATCH && l!=cur.l; ){
			status = regexec( &re, l->c, 1, matches, 0);
			if( ! status )
				break;

			if( ! l->next )
				l = fstart;
			else
				l = l->next;
		}

	regfree(&re);

	if( ! status ){
		/* match, address is matches[0].rm_so to matches[0].rm_eo */
		sels = (Filepos){l, matches[0].rm_so};
		sele = (Filepos){l, matches[0].rm_eo};
		cur = sels;
	}
}

void /* set mark and search backwards using regex in buffer */
f_searchb(const Arg *arg){
	f_mark( &(Arg){.c=0} );
	/* FIXME todo */
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
	int i=0;
	if( ! pos.l )
		return pos;
	for( i=0; i<height && pos.l->next; ++i, pos.l=pos.l->next ) ;
	if( pos.o > pos.l->len )
		pos.o = pos.l->len;
	return pos;
}

Filepos /* move cursor to prev screen */
m_prevscreen(Filepos pos){
	int i=0;
	if( ! pos.l )
		return pos;
	for( i=0; i<height && pos.l->prev; ++i, pos.l=pos.l->prev ) ;
	if( pos.o > pos.l->len )
		pos.o = pos.l->len;
	return pos;
}

void /* what to do on a SIGCONT */
i_sigcont(int unused){
	t_setstate(&nstate);
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
Line* /* return new line containing mul * LINESIZE chars */
i_newline(int mul){
	Line *l;
	if( ! (l = (Line *) malloc(sizeof(Line))) ) i_die("failed to malloc in i_newline");
	l->mul = mul;
	if( ! (l->c = (char *) calloc(sizeof(char), LINESIZE * l->mul)) ) i_die("failed to calloc in i_newline");
	l->c[0] = '\0';
	l->len = 0;
	l->dirty = true;
	return l;
}

Filepos /* insert c at post and return new filepos after the inserted char */
i_insert(Filepos pos, const char *buf){
	int i;
	Line *l=pos.l, *ln=0;
	char c;
	if( ! l )
		return pos;
	for( i=0, c=buf[0]; buf[i] != '\0'; c=buf[++i] ){
		if( c == '\n' || c == '\r' ){
			ln = i_newline(l->mul);
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
	modified = true;
	return pos;
}

Filepos /*trivial backspace */
i_backspace(Filepos pos){
	if( ! pos.l )
		return pos;

	if( pos.o <= 0 ){
		/* need to move everythin in this line onto the end of the previos */
		if( ! pos.l->prev ) return pos;
		Line *l = pos.l->prev;
		int nl = pos.l->len + l->len;
		int nmul = nl / LINESIZE +1;

		if( nmul > l->mul ){
			l->mul = nmul;
			if( ! (l->c = realloc(l->c, l->mul*LINESIZE)) ) i_die("failed to realloc in i_backspace\n");
		}
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
		/* move every character after me one down */
		if( ! memmove( &(pos.l->c[pos.o-1]), &(pos.l->c[pos.o]), (pos.l->len - pos.o)+1 ) )
			i_die("failed to memmove in i_backspace\n");
		--pos.o;
		--pos.l->len;
	}
	height = 0; /* set height to 0 to indicate sdirty */
	modified = true;
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
	nstate = t_initstate(&orig);
	t_setstate(&nstate);
	t_clear();
	f_normal();
	c_line0();
}

void
i_tidyup(void){
	Line *l=0, *lb=0;
	for( l=fstart; l; ){
		free(l->c);
		lb = l->next;
		free(l);
		l=lb;
	}
	free(curfile);
	t_setstate(&orig);
	t_clear();
	f_normal();
	c_line0();
	fflush(stdout);
}

void /* draw all dirty lines on screen or draw all lines if sdirty */
i_drawscr(bool sdirty, int crow, int ccol){
	Line *l;
	int n=1, c=0, i=0; /* n is line number, c is the char counter, i is used within the printing loop */
	bool selected = false;

	if( sels.l)
		fprintf(stderr, "so %d se %d cur.o %d ss.l->len %d\n", sels.o, sele.o, cur.o, sels.l->len);

	c_line0();
	for( n=1, l=sstart; l && n<height; l=l->next, ++n ){
		if( selected ){
			b_green();
			l->dirty = true;
		} else if( l == cur.l ){
			b_blue();
			l->dirty = true;
		} else
			b_default();

		if( l->dirty || sdirty ){
			l->dirty = false;
			c_clearline();
			for( c=0; c<=l->len && c<(width-1); ++c ){
				if( l == sels.l && c == sels.o ){
					selected = true;
					b_green();
					l->dirty = true;
				} else if( (l == sele.l && c == sele.o) || (! sele.l) ){
					selected = false;
					if( cur.l == l )
						b_blue();
					else
						b_default();
				}


				if( l->c[c] == '\t' )
					for( i=0; i<TABSTOP; ++i )
						fputc(' ', stdout);
				else if( l->c[c] != '\0' )
					fputc(l->c[c], stdout);
			}
			if( l == cur.l ){
				b_blue();
				for( ; c<(width-1); ++c )
					fputc(' ', stdout);
				if( selected )
					b_green();
				else
					b_default();
			}
		}
		c_nline();
	}
	b_default();
	for( ; n<height; ++n ){
		c_clearline();
		c_nline();
	}

	c_goto(crow, ccol);
	fflush(stdout);
	return;
}

void /* make sure cursor is on screen and screen is correct size, delegate to i_drawscr for actual drawing */
i_draw(void){
	int nh = t_getheight(), nw = t_getwidth();
	bool sdirty = false; /* is the entire range sstart->send dirty */
	int i=0, ccol=0; /* i is used as a general counter and as crow, ccol is column count */
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
			for( i=nh/2; l->prev && i > 1; --i, l=l->prev ) ;
			sdirty = true;
			sstart = l;
			i_drawscr(sdirty, nh/2, ccol);
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
			for( i=nh/2; l->prev && i > 1; --i, l=l->prev ) ;
			sdirty = true;
			sstart = l;
			i_drawscr(sdirty, nh/2, ccol);
			return;
		}
	}
	i_die("impossible case ocolcured in i_draw, *BOOM*\n");
}

int /* initialise data structure and read in file */
i_loadfile(char *fname){
	int fd;
	char *buf=0;
	ssize_t n;

	if( ! fstart ){
		buffer = i_newline(1);
		fstart = i_newline(1);
		fend = fstart;
		cur.l = fstart;
		cur.o = 0;
	}

	if( fname == 0 || fname[0] == '-' )
		fd = 0;
	else{
		if( (fd=open(fname, O_RDONLY)) == -1 )
			; /* FIXME new file, inform user */
		if( curfile )
			free( curfile );
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
	modified = false;
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

	for( l=fstart; l; l=l->next ){
		if( write(fd, l->c, l->len) == -1 ){
			error = true;
			break;
		}
		if( l != fend )
			if(write(fd, "\n", 1) == -1){
				error = true;
				break;
			}
	}

	modified = false;
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
