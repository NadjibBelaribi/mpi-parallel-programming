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
  mnt *part_m;
  MPI_Datatype Mpi_bcastParam;
  float *matrix,max;
  struct bcastParam recvParam;

  if (MPI_Init(&argc, &argv))
  {
    fprintf(stderr, "erreur MPI_Init!\n");
    return (1);
  }

  /* tout ce qui suit permet de créer le type MPI qui contient toutes les infos*/

  MPI_Datatype type[3] = {MPI_INT, MPI_INT, MPI_FLOAT};
  int blocklen[3] = {1, 1, 1};
  MPI_Aint disp[3];

  disp[0] = (char *)&recvParam.ligne_per_proc - (char *)&recvParam;
  disp[1] = (char *)&recvParam.col_per_proc - (char *)&recvParam;
  disp[2] = (char *)&recvParam.max - (char *)&recvParam;

  MPI_Type_create_struct(3, blocklen, disp, type, &Mpi_bcastParam);
  MPI_Type_commit(&Mpi_bcastParam);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  /******************************************/

  printf("je suis %d / %d \n", rank, size);

  // READ INPUT
  if (rank == 0)
  {
    m = mnt_read(argv[1]);
    recvParam.ligne_per_proc = m->nrows;
    recvParam.col_per_proc = m->ncols;
    recvParam.max = m->max;
    matrix = m->terrain;
  }

  /***Tout le monde doit effectuer le MPI_Bcast sinon ils reçoivent pas****/

  MPI_Bcast(&recvParam, 1, Mpi_bcastParam, 0, MPI_COMM_WORLD);

  part_m = (mnt *)malloc(sizeof(mnt));
  part_m->nrows = recvParam.ligne_per_proc;
  part_m->ncols = recvParam.col_per_proc;
  part_m->max = recvParam.max;

  part_m->terrain = malloc(sizeof(float) * part_m->ncols * part_m->nrows);

  MPI_Scatter(matrix, part_m->ncols * part_m->nrows, MPI_FLOAT,
              part_m->terrain, part_m->ncols * part_m->nrows, MPI_FLOAT, 0, MPI_COMM_WORLD);

  printf("proc N %d / %d avec rows ; %d cols ; %d \n", rank, size, part_m->nrows, part_m->ncols);

  // COMPUTE
  //d = darboux(part_m);

  MPI_Gather(part_m->terrain, part_m->ncols * part_m->nrows, MPI_FLOAT, m->terrain, part_m->ncols * part_m->nrows, MPI_FLOAT, 0, MPI_COMM_WORLD);

  if (rank == 0)
  {
    for (int i = 0; i < part_m->ncols * part_m->nrows * size; i++)
      printf(" %d %f", rank, m->terrain[i]);
    printf("\n");
  }

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
    mnt_write_lakes(part_m, d, stdout);

  // free
  free(m->terrain);
  free(m);
  free(d->terrain);
  free(d);*/

  MPI_Finalize();

  return (0);
}