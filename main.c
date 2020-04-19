// programme principal
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>
#include <string.h>
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

  mnt *m = NULL;
  mnt *d = (mnt *)malloc(sizeof(*d));
  mnt *part_m;
  MPI_Datatype Mpi_bcastParam;
  float *matrix = NULL;
  struct bcastParam recvParam;
  double time_kernel = 0;

  if (MPI_Init(&argc, &argv))
  {
    fprintf(stderr, "erreur MPI_Init!\n");
    return (1);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // rank du processus
  MPI_Comm_size(MPI_COMM_WORLD, &size); // nombre de processus

  /* Creation type MPI pour le broadcast */

  MPI_Datatype type[5] = {MPI_FLOAT, MPI_FLOAT, MPI_INT, MPI_INT, MPI_INT};
  int blocklen[5] = {1, 1, 1, 1, 1};
  MPI_Aint disp[5];

  disp[0] = (char *)&recvParam.max - (char *)&recvParam;
  disp[1] = (char *)&recvParam.no_data - (char *)&recvParam;
  disp[2] = (char *)&recvParam.ligne_per_proc - (char *)&recvParam;
  disp[3] = (char *)&recvParam.col_per_proc - (char *)&recvParam;
  disp[4] = (char *)&recvParam.first_rows - (char *)&recvParam;

  MPI_Type_create_struct(5, blocklen, disp, type, &Mpi_bcastParam);
  MPI_Type_commit(&Mpi_bcastParam);

  //-----------------------------------------------------------------------//

  // READ INPUT on process 0
  if (rank == 0)
  {

    m = mnt_read(argv[1]);
    matrix = m->terrain;

    m->max = max_terrain(m);
    recvParam.ligne_per_proc = m->nrows / size;
    recvParam.col_per_proc = m->ncols;
    recvParam.max = m->max;
    recvParam.no_data = m->no_data;
    recvParam.first_rows = m->first_rows;

    memcpy(d, m, sizeof(*d));
    d->ncols = m->ncols;
    d->nrows = m->nrows;
    d->terrain = (float *)malloc(d->nrows * d->ncols * sizeof(float));
  }

  // envoyer les données pour chaque processus
  MPI_Bcast(&recvParam, 1, Mpi_bcastParam, 0, MPI_COMM_WORLD);

  // structre mnt pour chaque processus
  part_m = (mnt *)malloc(sizeof(mnt));

  // reception des données
  part_m->nrows = recvParam.ligne_per_proc;
  part_m->ncols = recvParam.col_per_proc;
  part_m->max = recvParam.max;
  part_m->no_data = recvParam.no_data;
  part_m->first_rows = recvParam.first_rows;

  float taille = part_m->ncols * part_m->nrows;
  part_m->terrain = malloc(sizeof(float) * taille);

  if (rank == 0)
    time_kernel = omp_get_wtime();

  // Partager le terrain entre les processus 
  MPI_Scatter(matrix, part_m->ncols * part_m->nrows, MPI_FLOAT,
              part_m->terrain, part_m->ncols * part_m->nrows, MPI_FLOAT, 0, MPI_COMM_WORLD);

  // COMPUTE
  part_m = darboux(part_m);

  /// Regroupement des résultats par le processus 0 
  MPI_Gather(&part_m->terrain[part_m->ncols], part_m->ncols * part_m->nrows, MPI_FLOAT,
             d->terrain, part_m->ncols * part_m->nrows, MPI_FLOAT, 0, MPI_COMM_WORLD);

  if (rank == 0)
  {

    d->nrows = m->first_rows;
    time_kernel = omp_get_wtime() - time_kernel;
    printf("Kernel time -- : %3.5lf s\n", time_kernel);

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

    free(m->terrain);
    free(m);
    free(d->terrain);
    free(d);
  }

  free(part_m->terrain);
  free(part_m);

  MPI_Finalize();

  return (0);
}
