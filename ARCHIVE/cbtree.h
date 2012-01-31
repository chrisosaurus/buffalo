#ifndef CBTREE_H
#define CBTREE_H

struct cbt_node{
  int order; /* order according to Knuth, the maximum number of children per node */
  int count; /* the number of characters within this subtree FIXME count may interfere with n_elems in the mindspace*/
  int n_elems; /* the number of elements in this node, add 1 to find number of children (if non leaf) */
  char *elems; /* pointer to array of up to order-1 elements */
  cbt_node **children; /* pointer to array of up to order children */
  cbt_node *parent;
};

/* construct a new counted btree or return 0, if this is the root set parent to null */
struct cbt_node* cbt_new(int order, struct cbt_node* parent);

/* get the char stored at index */
char cbt_get(struct cbt_node* node, int index);

/* add elem below this node at index, and then update up the tree */
void cbt_add(struct cbt_node* node, int index, char elem);

/* remove the elem at index, and update the tree */
void cbt_remove(struct cbt_node* node, int index);

/* find the node with index as on of its elems, and return it
 * update index to refer to the location within the node
 */
struct cbt_node* cbt_findnode(struct cbt_node* node, int* index);

#endif
