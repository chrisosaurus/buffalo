#include <stdlib.h> /* for calloc */
#include "cbtree.h"

static cbt_node*
cbt_merge( struct cbt_node* n1, struct cbt_node* n1 ){
  // FIXME
}

static void
cbt_insert( struct cbt_node* node, char median, struct cbt_node* left, struct cbt_node* right ){
  // FIXME
}

struct cbt_node*
cbt_new( int order, struct cbt_node* parent ){
  struct cbt_node* n = (struct cbt_node*) calloc(sizeof(struct cbt_node));
  if( n ){
    n->parent = parent;
    n->order = order;
    n->elems = (char*) calloc(sizeof(char) * (order-1)); /* an array[order-1] of chars */
    n->children = (struct cbt_node**) calloc(sizeof(struct cbt_node*) * order); /* an array of points to cbt_nodes */
  }
  return n;
}

char
cbt_get( struct cbt_node* node, int index ){
  struct cbt_node* n = cbt_findnode( node, index ); // updates index to the correct position
  if( !n ) return 0;
  if( index >= n->n_elems ) return 0; // something went wrong
  return n->elems[index];
}

void
cbt_add( struct cbt_node* node, int index, char elem ){
  //FIXME
}

void
cbt_remove( struct cbt_node* node, int index ){
  //FIXME
}

struct cbt_node*
cbt_findnode( struct cbt_node* node, int* index ){
  int tally=0; /* the tally so far of the counts of the children nodes, needed to translate index for sub tree */
  if( index > node->count )
    return 0; // it is outside the range in this subtree
  for( int i=0; i<node->order; ++i ){
    if( ! node->children[i] )
      return 0; // a null child means all other children to the right are null

    if( node->children[i]->count >= *index ){ // in this subtree
      *index = *index - tally +1;
      return cbt_findnode( node->children[i], index ); // translate index for this subtree
    } else if( node->children[i]->count+1 == *index ){ // it is the element directly after child i
      *index = i; // this is where I found it! FIXME
      return node;
    } else // it is somewhere to the right
      tally += node->children[i]->count;
  }
  return 0;
}


