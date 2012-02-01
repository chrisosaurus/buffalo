#include "cbtree.h"

#include <stdio.h>

int main(){
  struct cbtree *root = cbt_new(4);
  cbt_add(&root, 0, 'a');

  char ret = cbt_remove(&root, 0);
  if( ret != 'a' )
    fprintf(stderr, "Remove failed, expected a, got: %c", ret);
}
