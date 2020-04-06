// programme principal
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "type.h"
#include "io.h"
#include "darboux.h"


int rank,size ; 
extern int ligne_per_proc,col_per_proc,elem_per_proc ; 
extern float max ;  
extern struct bcastParam recvParam;
MPI_Datatype Mpi_bcastParam;

int main(int argc, char **argv)
{
  mnt *m, *d;
  


  if(argc < 2)
  {
    fprintf(stderr, "Usage: %s <input filename> [<output filename>]\n", argv[0]);
    exit(1);
  }

  if(MPI_Init(&argc, &argv))
  {
    fprintf(stderr, "erreur MPI_Init!\n");
    return(1);
  }


  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
 

 /* tout ce qui suit permet de créer le type MPI qui contient toutes les infos
 ** Les colonnes, les lignes et le max
 ** la structure est dans io.h
 ***/
  MPI_Datatype type[3]={MPI_INT,MPI_INT,MPI_FLOAT};
  int blocklen[3]={1,1,1};
  MPI_Aint disp[3];

  
  
  disp[0] = (char*)&recvParam.ligne_per_proc - (char*)&recvParam;
  disp[1] = (char*)&recvParam.col_per_proc - (char*)&recvParam;
  disp[2] = (char*)&recvParam.max - (char*)&recvParam;
  

  MPI_Type_create_struct(3, blocklen, disp, type, &Mpi_bcastParam);
  MPI_Type_commit(&Mpi_bcastParam);
  /******************************************/

 printf("je suis %d / %d \n",rank,size) ;
 
  // READ INPUT
     if(rank == 0 ) 
      m = mnt_read(argv[1]);

  MPI_Barrier(MPI_COMM_WORLD) ;

  /***Tout le monde doit effectuer le MPI_Bcast sinon ils reçoivent pas****/
  
  MPI_Bcast(&recvParam, 1, Mpi_bcastParam, 0, MPI_COMM_WORLD);

  printf("Je suis %d: %d, %d, %f\n", rank,recvParam.ligne_per_proc,recvParam.col_per_proc,recvParam.max);

  if (rank != 0 ) 
  {
    //MPI_Bcast(&recvParam, 1, Mpi_bcastParam, 0, MPI_COMM_WORLD);
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