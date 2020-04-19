// programme principal
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

#include "type.h"
#include "io.h"
#include "darboux.h"

int main(int argc, char **argv)
{
  mnt *m, *d;
double time_reference ; 
  if(argc < 2)
  {
    fprintf(stderr, "Usage: %s <input filename> [<output filename>]\n", argv[0]);
    exit(1);
  }

  // READ INPUT
  m = mnt_read(argv[1]);

  // COMPUTE
  time_reference = omp_get_wtime();
  d = darboux(m);
  time_reference = omp_get_wtime() - time_reference;
  printf("Reference time : %3.5lf s\n", time_reference);


  // WRITE OUTPUT
  FILE *out;
  if(argc == 3)
    out = fopen(argv[2], "w");
  else
    out = stdout;
     mnt_write(d, out);
  if(argc == 3)
    fclose(out);
  else
    mnt_write_lakes(m, d, stdout);

  // free*/
  free(m->terrain);
  free(m);
  free(d->terrain);
  free(d);

  return(0);
}
