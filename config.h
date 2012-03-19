#define ALT(ch) { 0x1b, ch }
#define CTRL(ch) { (ch^0x40) }

/* defines width of a tab on screen */
#define TABSTOP 2

static const Key keys[] = {
	/* c        m_func    Arg */
	{ ALT('i'), f_cur, { .m_func = m_prevline } },
	{ ALT('k'), f_cur, { .m_func = m_nextline } },
	{ ALT('I'), f_cur, { .m_func = m_prevscreen } },
	{ ALT('K'), f_cur, { .m_func = m_nextscreen } },
	{ ALT('l'), f_cur, { .m_func = m_nextchar } },
	{ ALT('j'), f_cur, { .m_func = m_prevchar } },
	{ ALT('L'), f_cur, { .m_func = m_nextword } },
	{ ALT('J'), f_cur, { .m_func = m_prevword } },
	{ ALT('q'), f_quit, { .i=0 } },
	{ ALT('Q'), f_quit, { .i=1 } },
	{ ALT('w'), f_write, { .c=0 } },

	{ ALT('u'), f_cur, { .m_func = m_startofline } },
	{ ALT('U'), f_cur, { .m_func = m_startoffile } },
	{ ALT('o'), f_cur, { .m_func = m_endofline } },
	{ ALT('O'), f_cur, { .m_func = m_endoffile } },

	{ ALT('G'), f_mark, { .i = 0 } },
	{ ALT('g'), f_mark, { .i = 1 } },

	{ ALT('F'), f_sel, { .i=0 } },
	{ ALT('f'), f_sel, { .i=1 } },
	{ ALT('D'), f_sel, { .i=2 } },

	{ ALT('d'), f_paste, {.c=0 } },
	{ ALT('s'), f_copy, { .c=0 } },
	{ ALT('S'), f_cut, { .c=0 } },

	{ ALT('H'), f_newl, { .i=0 } },
	{ ALT('h'), f_newl, { .i=1 } },
	{ ALT('y'), f_align, { .i=0 } },
	{ ALT('Y'), f_align, { .i=1 } },

	{ CTRL('Z'), f_suspend, { .c=0 } },

	/*{ ALT('h'), f_cur, { .m_func = m_SOMETHING } },
	{ ALT('H'), f_cur, { .m_func = m_SOMETHING } },
	*/
};

