static int tabstop = 2;

#define ALT(ch) { 0x1b, ch }

static const Key keys[] = {
	/* c        m_func    Arg */
	{ ALT('i'), f_cur, { .m_func = m_prevline } },
//{ ALT('I'), f_cur, { .m_func = m_prevscreen } },
	{ ALT('k'), f_cur, { .m_func = m_nextline } },
//	{ ALT('K'), f_cur, { .m_func = m_nextscreen } },
	{ ALT('l'), f_cur, { .m_func = m_nextchar } },
//	{ ALT('L'), f_cur, { .m_func = m_nextword } },
	{ ALT('j'), f_cur, { .m_func = m_prevchar } },
//	{ ALT('J'), f_cur, { .m_func = m_prevword } },
/*
	{ ALT('u'), f_cur, { .m_func = m_startofline } },
	{ ALT('U'), f_cur, { .m_func = m_startoffile } },
	{ ALT('o'), f_cur, { .m_func = m_endofline } },
	{ ALT('O'), f_cur, { .m_func = m_endoffile } }
*/
	/*{ ALT('h'), f_cur, { .m_func = m_SOMETHING } },
	{ ALT('H'), f_cur, { .m_func = m_SOMETHING } },
	*/
};
