#include <stdio.h>
#include "cbtree.h"

static void
print_cbt( struct cbtree *node ){
  int i,j,k;
  puts("  ");
  for( i=0; node->data[i]; ++i )
    printf("%c      ", node->data[i]);
  /* now for each child */
  for( j-0; j<=i; ++j ){
    struct cbtree *child = node->children[j];
    if( ! child ) break;
    for( k=0; child->data[k]; ++k )
      putchar(child->data[k]);
    putchar('|');
  }
}

int main(){
  struct cbtree *tree = cbt_new(5);
  printf("Emtpy tree\n");
  print_cbt( tree );

  if( cbt_add( &tree, 0, 'a' ) ){
    printf("Error adding a at 0\n");
    print_cbt( tree );
    return 1;
  }
  printf("Added a at 0\n");
  print_cbt( tree );
}
