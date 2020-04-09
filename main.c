 // programme principal
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "type.h"
#include "io.h"
#include "darboux.h"

int rank, size;

int main(int argc, char **argv)
{

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s <input filename> [<output filename>]\n", argv[0]);
    exit(1);
  }

  mnt *m, *d;
  mnt *part_m = NULL;
  MPI_Datatype Mpi_bcastParam;
  float *matrix = NULL;
  struct bcastParam recvParam;

  if (MPI_Init(&argc, &argv))
  {
    fprintf(stderr, "erreur MPI_Init!\n");
    return (1);
  }

  /* Creation type pour le broadcast */

  MPI_Datatype type[4] = {MPI_INT, MPI_INT, MPI_FLOAT, MPI_FLOAT};
  int blocklen[4] = {1, 1, 1, 1};
  MPI_Aint disp[4];

  disp[0] = (char *)&recvParam.ligne_per_proc - (char *)&recvParam;
  disp[1] = (char *)&recvParam.col_per_proc - (char *)&recvParam;
  disp[2] = (char *)&recvParam.max - (char *)&recvParam;
  disp[3] = (char *)&recvParam.no_data - (char *)&recvParam;

  MPI_Type_create_struct(4, blocklen, disp, type, &Mpi_bcastParam);
  MPI_Type_commit(&Mpi_bcastParam);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  /******************************************/

  // READ INPUT
  if (rank == 0)
  {
    m = mnt_read(argv[1]);
    
     recvParam.ligne_per_proc = m->nrows;
     recvParam.col_per_proc = m->ncols;
     recvParam.max = m->max;
     recvParam.no_data = m->no_data;

     matrix = m->terrain;


     d = malloc(sizeof(*d));
     memcpy(d, m, sizeof(*d));
     d->terrain = malloc(m->nrows*m->ncols*size* sizeof(float)) ;
     d->ncols = m->nrows;
     d->nrows = m->ncols;

   }

  part_m = (mnt *)malloc(sizeof(mnt));

  MPI_Bcast(&recvParam, 1, Mpi_bcastParam, 0, MPI_COMM_WORLD);

  part_m->nrows = recvParam.ligne_per_proc;
  part_m->ncols = recvParam.col_per_proc;
  part_m->max = recvParam.max;
  part_m->no_data = recvParam.no_data;

  // allouer 2 lignes additionneles pour l'echange

  float taille = part_m->ncols * part_m->nrows;
  part_m->terrain = malloc(sizeof(float) * taille);

  MPI_Scatter(matrix, part_m->ncols * part_m->nrows, MPI_FLOAT,
              part_m->terrain, part_m->ncols * part_m->nrows, MPI_FLOAT, 0, MPI_COMM_WORLD);

  // COMPUTE
  part_m = darboux(part_m);

  MPI_Gather(&part_m->terrain[part_m->ncols], part_m->ncols * part_m->nrows, MPI_FLOAT,
             &d->terrain[0], part_m->ncols * part_m->nrows, MPI_FLOAT, 0, MPI_COMM_WORLD);

  /*if (rank == 0)
  {

    // WRITE OUTPUT
    FILE *out;
    if (argc == 3)
      out = fopen(argv[2], "w");
    else
      out = stdout;
    mnt_write(d, out);
    if (argc == 3)
      fclose(out);
    else
      mnt_write_lakes(m, d, stdout);

    // free
    */
   if(rank == 0){

    free(m->terrain);
    free(m);
    free(matrix);
  }
printf("\n");


  free(part_m->terrain);
  free(part_m);
  free(d->terrain);
  free(d);

  MPI_Finalize();

  return (0);
}
