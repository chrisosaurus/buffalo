#include <stdlib.h> /* malloc, calloc */
#include "cbtree.h"

struct cbtree{
  struct cbtree *parent;
  struct cbtree **children; /* pointer to array of pointers to children */
  char *data; /* pointer to array of data */
  int cap; /* number of elems per node (length of data) */
  int count; /* number of data items in this subtree */
  int ncount; /* number of newlines in this subtree */
  int eon; /* is the last data of this subtree a newline */
};

struct cbtree *cbt_new(int n){
  struct cbtree *node = (struct cbtree *) malloc(sizeof(struct cbtree));
  node->parent = 0;
  node->children =0;
  node->data = (char *) calloc(n, sizeof(char));
  node->cap = n;
  node->count = 0;
  node->ncount = 0;
  node->eon = 0;
  return node;
}


int
cbt_add(struct cbtree **root, int pos, char data){
  struct cbtree *node = 0;
  int place = cbt_findpos(*root, pos, &node);
  if( place < 0 ){
    // TODO right descend, as we are appending the data to the 'end' of the tree
  } else {
  /* handle splitting case */
  if( node->count == node ->cap ){ /* works fine as inserts are always in leaves */
    struct cbtree *right = 0;
    char sep = cbt_split( node, &right, place, data, 0, 0 );
    if( !right ) return -1; /* FIXME error case */
    if( cbt_promote( node->parent, node, sep, 0, 0) ) return -1; /* FIXME error case */
  } else { 
    // TODO handle simple (non splitting) insert case, requires a shuffle
  }
  }
  return 0; /* TODO */
}

char
cbt_remove(struct cbtree **root, int pos){
  return 0; /* TODO */
}


char
cbt_replace(struct cbtree *from, int pos, char data){
  struct cbtree *node = 0;
  int place = cbt_findpos(from, pos, &node);
  if( place < 0 ) return 0; /* FIXME error case */
  char old = node->data[place]; /* FIXME what if there is nothing there?*/
  node->data[place] = data;
  return old;
}

char
cbt_get(struct cbtree *from, int pos){
  struct cbtree *node = 0;
  int place = cbt_findpos(from, pos, &node);
  if( place < 0 ) return 0; /* FIXME error case */
  return node->data[place];
}

int
cbt_findpos(struct cbtree *from, int pos, struct cbtree **node){
  if( pos > from->count )
   return -1;
  if( ! from->children ){
    if( pos < from->cap ){ /* a winner is me */
      *node = from;
      return pos;
    }
    return -1;
  }
  
  /* try everything except for the far right child */
  int i;
  for( i=0; i < from->cap; ++i ){
    if( ! from->children[i] )
      return -1;
    int c = from->children[i]->count;
    if( c < pos )
      return cbt_findpos(from->children[i], pos-c, node);
    else if( c == pos ){ /* found it */
      *node = from;
      return i;
    }
  }

  /* not found within node or any of the left children, decent rightwards */
  if( ! from->children[from->cap] )
    return -1;
  return cbt_findpos( from->children[from->cap], pos, node);
}

int
cbt_findline(struct cbtree *from, int line, struct cbtree **node){
  /* same as findpos, just replace use of count with ncount */
  return 0; /* TODO */
}

/* split the node left into two, keeping the lower half in left and moving the upper half into right (after allocating memory for right)
 * put insertee into pos (logically) before split, preserve its children (if supplied)
*/
static char
cbt_split( struct cbtree *left, struct cbtree **right, int pos, char insertee, struct cbtree *leftchild, struct cbtree *rightchild ){
  char sep; /* new seperator post split */
  struct cbtree *insideright; /* inside right child post split */
  
  if( pos > left->cap ) return 0; /* FIXME error case, can tell as right is still not allocated */

  (*right) = cbt_new( left->cap );
  if( ! *right ) return 0; /* FIXME error case, see previous error case */
  
  /* first pass, pre arrange tree and find seperator to make copying phase trivial */
  int half = left->cap / 2;
  if( pos == half ){
    sep = insertee;
    insideright = rightchild;
    left->children[half] = leftchild; /* TODO check this, origionally had ->data ... */
  } else {
    if( pos < half ){
      sep = left->data[half-1];
      insideright = left->children[half];
      if( cbt_shuffle( left, pos, half ) ) return -1; /* FIXME error case */
      /*int i;
      for( i=half; i>pos; --i ){ this loop may have been incorrect
        left->children[i] = left->children[i-1];
        left->data[i-1] = left->data[i-2];
      }*/
    } else {
      sep = left->data[half];
      insideright = left->children[half+1];
      if( cbt_shuffle( left, half, pos ) ) return -1; /* FIXME error case */
      /*int i;
      for( i=half; i < pos; ++i ){ this loop may have been incorrect
        left->data[i] = left->data[i+1];
        left->children[i+1] = left->children[i+2];
      } */
    }
    /* either way, do these */
    left->data[pos] = insertee;
    left->children[pos] = leftchild;
    left->children[pos+1] = rightchild;
  }

  /* copy over latter half */
  int i;
  for( i=half; ++i; i < left->cap ){
    (*right)->data[i-half] = left->data[i];
    (*right)->children[i-half+1] = left->children[i+1];
  }

  /* set right inside child */
  (*right)->children[0] = insideright;
}

/* promote sep from below, given its new left and right children and the address of the old left child in order to find pos
*/
static int
cbt_promote( struct cbtree *node, struct cbtree *lanchor, char sep, struct cbtree *leftchild, struct cbtree *rightchild ){
  /* search through children until we find lanchor, this is where to insert (to the right of)
   * if this node is already full, split is our friend
   * otherwise shuffle down from lanchor (both data and children)
   * replace right of lanchor with sep, replace lanchor with leftchild and replace the next child with rightchild
  */
  // TODO handle split case (note, cannot simply compare count to cap as this cant be a leaf node)
  // TODO correct order of args so that promote and split have a similar interface
  // TODO who handles new root promotion? I can, if node is null
  int i;
  for( i=0; i <= node->cap; ++i ){
    if( ! node->children[i] ) return -1; /* FIXME error case */
    if( node->children[i] == lanchor ){
      /* found it, shuffle everything up */
      if( cbt_shuffle(node, i, node->cap) ) return -1; /* FIXME error case */
      /*int j;
      for( j=node->cap; j>i; --j ){
        node->data[j] = node->data[j-1];
        node->children[j+1] = node->children[j];
      } */
      /* insert and overwrite (old) left child */
      node->children[i] = leftchild;
      node->children[i+1] = rightchild;
      node->data[i] = sep;
      return 0;
    }
  }
  return -1; /* FIXME error case */
}

/* shuffle the data starting from from and going until to
 * also shuffle the children starting from from and going until to+1
 * NB: the child at from will be in two places at the end of this operation
 */
static int
cbt_shuffle( struct cbtree *node, int from, int to ){
  int i=to;
  node->children[to+1] = node->children[to];
  for( ; i>from; --i ){
    node->data[i] = node->data[i-1];
    node->children[i] = node->children[i-1];
  }
  return 0;
}

