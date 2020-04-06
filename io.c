// fonctions d'entrée/sortie

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "check.h"
#include "type.h"
#include "io.h"
#include <string.h>
 #include<float.h>
#include <mpi.h>

int ligne_per_proc, col_per_proc;
float max ;
extern int rank, size;

mnt *mnt_read(char *fname)
{
  mnt *m;
  FILE *f;
  CHECK((m = malloc(sizeof(*m))) != NULL);
  CHECK((f = fopen(fname, "r")) != NULL);

  CHECK(fscanf(f, "%d", &m->ncols) == 1);
  CHECK(fscanf(f, "%d", &m->nrows) == 1);
  CHECK(fscanf(f, "%f", &m->xllcorner) == 1);
  CHECK(fscanf(f, "%f", &m->yllcorner) == 1);
  CHECK(fscanf(f, "%f", &m->cellsize) == 1);
  CHECK(fscanf(f, "%f", &m->no_data) == 1);

  CHECK((m->terrain = malloc(m->ncols * m->nrows * sizeof(float))) != NULL);

  max = FLT_MIN_10_EXP - 37;

  for (int i = 0; i < m->ncols * m->nrows; i++)
  {
    CHECK(fscanf(f, "%f", &m->terrain[i]) == 1);
    if (m->terrain[i] > max)
      max = m->terrain[i];
  }
  CHECK(fclose(f) == 0);

  float taille = (m->ncols * m->nrows) + (m->ncols * m->nrows) % size;
  float marge = (m->ncols * m->nrows) % size;

  realloc(m->terrain, marge * sizeof(float));

  ligne_per_proc = (taille / m->ncols) / size;
  col_per_proc = m->ncols ; 
  int element_per_proc = ligne_per_proc * m->ncols;

  MPI_Scatter(&m->terrain[0], element_per_proc, MPI_FLOAT,
              &m->terrain[0], element_per_proc, MPI_FLOAT, 0, MPI_COMM_WORLD);

  MPI_Bcast(&ligne_per_proc, 1, MPI_INT, 0, MPI_COMM_WORLD);
  m->nrows = ligne_per_proc;
 
  MPI_Bcast(&col_per_proc, 1, MPI_INT, 0, MPI_COMM_WORLD);
  m->nrows = ligne_per_proc;
 
  MPI_Bcast(&max, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);

  return (m);
}

void mnt_write(mnt *m, FILE *f)
{

  float *recv;
  CHECK((recv = malloc(100 * sizeof(float))) != NULL);

  MPI_Gather(&m->terrain[0], 50, MPI_FLOAT, recv, 50, MPI_FLOAT, 0, MPI_COMM_WORLD);

  CHECK(f != NULL);

  if (rank == 0)
  {
    memcpy(m->terrain, recv, 100);
    for (int i = 0; i < m->nrows; i++)
    {
      for (int j = 0; j < m->ncols; j++)
      {
        fprintf(f, "%.2f ", TERRAIN(m, i, j));
      }
      fprintf(f, "\n");
    }
  }
}

void mnt_write_lakes(mnt *m, mnt *d, FILE *f)
{

  float *recv;
  CHECK((recv = malloc(100 * sizeof(float))) != NULL);

  MPI_Gather(&m->terrain[0], 50, MPI_FLOAT, recv, 50, MPI_FLOAT, 0, MPI_COMM_WORLD);

  CHECK(f != NULL);

  memcpy(m->terrain, recv, 100);

  if (rank == 0)
  {
    for (int i = 0; i < m->nrows * m->nrows; i++)
    {
      printf(" %f \n", m->terrain[i]);
    }
  }
}