#include <unistd.h> /* fork, exec, dup2, */
#include <stdio.h> /* fprintf */
/*#include <sys/types.h> */
/*#include <sys/stat.h> */
#include <fcntl.h> /* file modes */

/* see 
http://stackoverflow.com/questions/2605130/redirecting-exec-output-to-a-buffer-or-file
*/

static char *catcmd[] = { "cat", NULL };
static const char *outfile = "/home/chris/devel/editor/OUT";
static const char *infile = "/home/chris/devel/editor/HERE";

/* spawn and capture input and/or output
 * if either our or in (or both) are 0, then that stream will not be captures
 * otherwise out or in should point to a file location, out will create if necessary */
// FIXME need to have truncation and appent support
void spawnc(char **run, const char *in, const char *out);

int main(){
  spawnc(catcmd, infile, outfile);
/*  examples of spawnc
  spawnc(catcmd, in, 0);
  spawnc(catcmd, 0, out); 
*/
  printf("Yes, I got here\n");
}

void spawnc(char **cmd, const char *in, const char *out){
  if( fork() == 0){
    //setsid(); // umm what
    int outfd=0, infd=0;

    if(out){
      outfd = open(out, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );
      /* must close stdout and stderr before the dup2 calls otherwise it will also stop writing to the files */
      close( 1 ); /* stop writing stdout to my parent's stdout */
      close( 2 ); /* stop writing stderr to my parent's stderr */
      dup2( outfd, 1 ); /* make stdout go to out */
      dup2( outfd, 2 ); /* make stderr go to out */
      /* can close file now, must do so after the dup2 calls otherwise we cant write to them */
      close( outfd );
    }

    if(in){
      infd = open(in, O_RDONLY );
      close( 0 ); /* stop reading stdin from my parents stdin */
      dup2( infd, 0 ); /* make stdin come from in */
      /* can close file now, must do so after the dup2 calls otherwise we cant write to them */
      close( infd );
    }

    execvp( cmd[0] , cmd );
    fprintf(stderr, "FAILED TO EXECVP\n");
  }
}
