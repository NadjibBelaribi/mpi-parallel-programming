// programme principal
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "type.h"
#include "io.h"
#include "darboux.h"

int rank,size ; 

int main(int argc, char **argv)
{
  mnt *m, *d;
  if(argc < 2)
  {
    fprintf(stderr, "Usage: %s <input filename> [<output filename>]\n", argv[0]);
    exit(1);
  }
  int ligne_per_proc,col_per_proc,elem_per_proc ; 
float max ; 

  if(MPI_Init(&argc, &argv))
  {
    fprintf(stderr, "erreur MPI_Init!\n");
    return(1);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
 
 printf("je suis %d / %d \n",rank,size) ;
 
  // READ INPUT
     if(rank == 0 ) 
      m = mnt_read(argv[1]);

  MPI_Barrier(MPI_COMM_WORLD) ;

  if (rank != 0 ) 
  {
      m = (mnt*)malloc(sizeof(mnt)) ; 
     m->terrain = malloc(sizeof(float)*elem_per_proc) ;

  }
  m->nrows = ligne_per_proc;
  m->ncols = col_per_proc;
  m->max = max;
  
  printf("proc N %d / %d avec rows ; %d cols ; %d  max ; %f \n",rank,size,m->nrows ,m->ncols,max)  ;

  // COMPUTE
  //d = darboux(m);

  // WRITE OUTPUT
  /*FILE *out;
  if(argc == 3)
    out = fopen(argv[2], "w");
  else
    out = stdout;
  mnt_write(d, out);
  if(argc == 3)
    fclose(out);
  else
    mnt_write_lakes(m, d, stdout);

  // free
  free(m->terrain);
  free(m);
  free(d->terrain);
  free(d);*/

    MPI_Finalize();

  return(0);
}