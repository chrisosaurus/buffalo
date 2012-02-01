struct cbtree;
/* allocate the memory for a new struct cbtree node */
struct cbtree* cbt_new(int n);

/* add data into the tree at pos from root, returns 0 for success, 1 for failure */
int cbt_add(struct cbtree **root, int pos, char data);
/* remove data at pos, returns old itme or null */
char cbt_remove(struct cbtree **root, int pos);

/* replace item at pos relative to from with data, returns old data or null */
char cbt_replace(struct cbtree *from, int pos, char data);
/* retreive data at pos relative to from */
char cbt_get(struct cbtree *from, int pos);

/* find the place in the tree to insert, given a from ndoe and a pos (relative to from) it will set node to the appropriate node and return the offset into node's data
 * will return -1 on error */
int cbt_findpos(struct cbtree *from, int pos, struct cbtree **node);
/* find the place in the tree that corresponds to the start of line number line, like findpos it will return the offset into data and set node to the appropraite node
 * will return -1 on error */
int cbt_findline(struct cbtree *from, int line, struct cbtree **node);


static char cbt_split( struct cbtree *left, struct cbtree **right, int pos, char insertee, struct cbtree *leftchild, struct cbtree *rightchild );

static int cbt_promote( struct cbtree *node, struct cbtree *lanchor, char sep, struct cbtree *leftchild, struct cbtree *rightchild );

static int cbt_shuffle( struct cbtree *node, int from, int to );

