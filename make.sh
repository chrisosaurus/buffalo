if [ ! -e bin ] ; then
  mkdir bin
fi

gcc editor.c codes.c -o bin/buffalo
gcc cbtree.c cbtree_test.c -o bin/cbtree_test
gcc forking.c -o bin/forking
