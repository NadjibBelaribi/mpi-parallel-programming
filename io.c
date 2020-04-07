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

 
extern int rank, size;

mnt *mnt_read(char *fname)
{
  mnt *m;
  FILE *f;
  float max ;
  int rows ;

   

  CHECK((m = malloc(sizeof(*m))) != NULL);
  CHECK((f = fopen(fname, "r")) != NULL);

  CHECK(fscanf(f, "%d", &m->ncols) == 1);
  CHECK(fscanf(f, "%d", &m->nrows) == 1);
  CHECK(fscanf(f, "%f", &m->xllcorner) == 1);
  CHECK(fscanf(f, "%f", &m->yllcorner) == 1);
  CHECK(fscanf(f, "%f", &m->cellsize) == 1);
  CHECK(fscanf(f, "%f", &m->no_data) == 1);

  if(m->nrows % size != 0 )
    rows =  (m->nrows +size - (m->nrows % size))/size ; 
  else 
    rows = m->nrows / size ; 
  float taille = (m->ncols * rows * size) ;

  CHECK((m->terrain = malloc(taille * sizeof(float))) != NULL);

  max = FLT_MIN_10_EXP - 37;

  for (int i = 0; i < m->ncols * m->nrows; i++)
  {
    CHECK(fscanf(f, "%f", &m->terrain[i]) == 1);
    if (m->terrain[i] > max)
      max = m->terrain[i];
  }
   m->max = max ;
   m->nrows = rows ;
  

CHECK(fclose(f) == 0);
  return(m);
}

void mnt_write(mnt *m, FILE *f)
{
  CHECK(f != NULL);

  fprintf(f, "%d\n", m->ncols);
  fprintf(f, "%d\n", m->nrows);
  fprintf(f, "%.2f\n", m->xllcorner);
  fprintf(f, "%.2f\n", m->yllcorner);
  fprintf(f, "%.2f\n", m->cellsize);
  fprintf(f, "%.2f\n", m->no_data);

  for(int i = 0 ; i < m->nrows ; i++)
  {
    for(int j = 0 ; j < m->ncols ; j++)
    {
      fprintf(f, "%.2f ", TERRAIN(m,i,j));
    }
    fprintf(f, "\n");
  }
}

void mnt_write_lakes(mnt *m, mnt *d, FILE *f)
{
  CHECK(f != NULL);

  for(int i = 0 ; i < m->nrows ; i++)
  {
    for(int j = 0 ; j < m->ncols ; j++)
    {
      const float dif = TERRAIN(d,i,j)-TERRAIN(m,i,j);
      fprintf(f, "%c", (dif>1.)?'#':(dif>0.)?'+':'.' );
    }
    fprintf(f, "\n");
  }
}