// fonction de calcul principale : algorithme de Darboux
// (remplissage des cuvettes d'un MNT)
#include <string.h>
#include <mpi.h>
#include <omp.h>

#include "check.h"
#include "type.h"
#include "darboux.h"

// si ce define n'est pas commenté, l'exécution affiche sur stderr la hauteur
// courante en train d'être calculée (doit augmenter) et l'itération du calcul
#define DARBOUX_PPRINT

#define PRECISION_FLOTTANT 1.e-5

// pour accéder à un tableau de flotant linéarisé (ncols doit être défini) :
#define WTERRAIN(w, i, j) (w[(i)*ncols + (j)])
extern int rank, size;

// calcule la valeur max de hauteur sur un terrain
float max_terrain(mnt *restrict m)
{
  float max_val = m->terrain[0];
  //int num=8;

  //Je met 6 threads parce qu'on utilise 6 slot par machine
  #pragma omp parallel for reduction(max : max_val),num_threads(12)
  for (int i = 0; i < m->ncols * m->nrows; i++){
    if (m->terrain[i] > max_val) max_val = m->terrain[i];
    //int tid=omp_get_thread_num();
    //printf("%d\n",tid);
  }
  return (max_val);
}

// initialise le tableau W de départ à partir d'un mnt m
float *init_W(const mnt *restrict m)
{
  const int ncols = m->ncols, nrows = m->nrows + 2;
  float *restrict W;

  CHECK((W = malloc(ncols * nrows * sizeof(float))) != NULL);

  // initialisation W
  const float max = m->max + 10.;

  for (int i = 1; i < nrows - 1; i++)
  {
    for (int j = 0; j < ncols; j++)
    {
      if ((i == 1 && rank == 0) || (i + rank * m->nrows) >= m->first_rows || j == 0 || j == ncols - 1 || TERRAIN(m, i - 1, j) == m->no_data)
        WTERRAIN(W, i, j) = TERRAIN(m, i - 1, j);
      else
        WTERRAIN(W, i, j) = max;
    }
  }

  return (W);
}

// variables globales pour l'affichage de la progression
#ifdef DARBOUX_PPRINT
float min_darboux = 9999.; // ça ira bien, c'est juste de l'affichage
int iter_darboux = 0;
// fonction d'affichage de la progression
void dpprint()
{
  if (min_darboux != 9999.)
  {
    fprintf(stderr, "%.3f %d\r", min_darboux, iter_darboux++);
    fflush(stderr);
    min_darboux = 9999.;
  }
  else
    fprintf(stderr, "\n");
}
#endif

// pour parcourir les 8 voisins :
const int VOISINS[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};

// cette fonction calcule le nouveau W[i,j] en utilisant Wprec[i,j]
// et ses 8 cases voisines : Wprec[i +/- 1 , j +/- 1],
// ainsi que le MNT initial m en position [i,j]
// inutile de modifier cette fonction (elle est sensible...):
int calcul_Wij(float *restrict W, const float *restrict Wprec, const mnt *m, const int i, const int j)
{
  const int nrows = m->nrows + 2, ncols = m->ncols;
  int modif = 0;

  // on prend la valeur précédente...
  WTERRAIN(W, i, j) = WTERRAIN(Wprec, i, j);
  // ... sauf si :
  if (WTERRAIN(Wprec, i, j) > TERRAIN(m, i - 1, j))
  {
    // parcourir les 8 voisins haut/bas + gauche/droite
    for (int v = 0; v < 8; v++)
    {
      const int n1 = i + VOISINS[v][0];
      const int n2 = j + VOISINS[v][1];

      // vérifie qu'on ne sort pas de la grille.
      // ceci est théoriquement impossible, si les bords de la matrice Wprec
      // sont bien initialisés avec les valeurs des bords du mnt
      CHECK(n1 >= 0 && n1 < nrows && n2 >= 0 && n2 < ncols);

      // si le voisin est inconnu, on l'ignore et passe au suivant
      if (WTERRAIN(Wprec, n1, n2) == m->no_data)
        continue;

      CHECK(TERRAIN(m, i - 1, j) > m->no_data);
      CHECK(WTERRAIN(Wprec, i, j) > m->no_data);
      CHECK(WTERRAIN(Wprec, n1, n2) > m->no_data);

      // il est important de mettre cette valeur dans un temporaire, sinon le
      // compilo fait des arrondis flotants divergents dans les tests ci-dessous
      const float Wn = WTERRAIN(Wprec, n1, n2) + EPSILON;
      if (TERRAIN(m, i - 1, j) >= Wn)
      {
        WTERRAIN(W, i, j) = TERRAIN(m, i - 1, j);
        modif = 1;
#ifdef DARBOUX_PPRINT
        if (WTERRAIN(W, i, j) < min_darboux)
          min_darboux = WTERRAIN(W, i, j);
#endif
      }
      else if (WTERRAIN(Wprec, i, j) > Wn)
      {
        WTERRAIN(W, i, j) = Wn;
        modif = 1;
#ifdef DARBOUX_PPRINT
        if (WTERRAIN(W, i, j) < min_darboux)
          min_darboux = WTERRAIN(W, i, j);
#endif
      }
    }
  }
  return (modif);
}

/*****************************************************************************/
/*           Fonction de calcul principale - À PARALLÉLISER                  */
/*****************************************************************************/
// applique l'algorithme de Darboux sur le MNT m, pour calculer un nouveau MNT
mnt *darboux(const mnt *restrict m)
{
  const int ncols = m->ncols, nrows = m->nrows + 2;

  // initialisation
  float *restrict W, *restrict Wprec;

  CHECK((W = malloc(ncols * nrows * sizeof(float))) != NULL);
  Wprec = init_W(m);

  
  
  // calcul : boucle principale
  int end = 1, modif;
  int started=0;
  while (end)
  {
    int i;
    

    //****************Optimization du ReadMe Proposition***************************//
    ///De cette façon ça nous permet de faire le reduce dans la boucle d'après
    //Mais j'aurai tendance a paralleliser tout ça en utilisant OpenMP
    //Faudrait peut étre mettre le Bcast à la fin ???

     ////***************************************************************************///
     
    if(started){
      MPI_Reduce(&modif, &end, 1, MPI_INT, MPI_LOR, 0, MPI_COMM_WORLD);
      MPI_Bcast(&end, 1, MPI_INT, 0, MPI_COMM_WORLD);
      modif = 0;
    }
    else{
      started=1;
    }

     //****************Optimization du ReadMe Proposition***************************//
    //On commence le calcul juste après le le MPI_Send
    //Le temps que ça arrive par le réseau on avance dans le calcul
    //Quand dans le calcul on arrive à la ligne qui nécessite la ligne envoyé par
    //un autre processus on fait le RecV

    ///Je pense que ça peut étre améliorer dans le elif et le else en mettant des tag differents
    ///Et en faisant tous les send avant les recv
    ///J'ai juste fait au plus simple pour le moment

     ////***************************************************************************///
   if(end){
    if (rank == 0)
    {
      // send last to next
      MPI_Send(&Wprec[(nrows - 2) * ncols], ncols, MPI_FLOAT, rank + 1, 200, MPI_COMM_WORLD);

      for (i = 1; i < nrows - 2; i++)
    {
      for (int j = 0; j < ncols; j++)
      {
        // calcule la nouvelle valeur de W[i,j]
        // en utilisant les 8 voisins de la position [i,j] du tableau Wprec
        modif |= calcul_Wij(W, Wprec, m, i, j);
      }
    }

      // recv first of next
      MPI_Recv(&Wprec[(nrows - 1) * ncols], ncols, MPI_FLOAT, rank + 1, 200, MPI_COMM_WORLD, NULL);

      for (int j = 0; j < ncols; j++)
      {
        // calcule la nouvelle valeur de W[i,j]
        // en utilisant les 8 voisins de la position [i,j] du tableau Wprec
        modif |= calcul_Wij(W, Wprec, m, i, j);
      }
    }
    else if (rank == size - 1)
    {

      // recv last of preceden
      MPI_Recv(&Wprec[0], ncols, MPI_FLOAT, rank - 1, 200, MPI_COMM_WORLD, NULL);

       // send first to precedent
      MPI_Send(&Wprec[ncols], ncols, MPI_FLOAT, rank - 1, 200, MPI_COMM_WORLD);

       
      
       for (int i = 1; i < nrows - 1; i++)
    {
      for (int j = 0; j < ncols; j++)
      {
        // calcule la nouvelle valeur de W[i,j]
        // en utilisant les 8 voisins de la position [i,j] du tableau Wprec
        modif |= calcul_Wij(W, Wprec, m, i, j);
      }
    }
     


    }
    else
    {

        for (i = 2; i < nrows - 2; i++)
    {
      for (int j = 0; j < ncols; j++)
      {
        // calcule la nouvelle valeur de W[i,j]
        // en utilisant les 8 voisins de la position [i,j] du tableau Wprec
        modif |= calcul_Wij(W, Wprec, m, i, j);
      }
    }
      //recv last from precedent
      MPI_Recv(&Wprec[0], ncols, MPI_FLOAT, rank - 1, 200, MPI_COMM_WORLD, NULL);

      // send first to precedent
      MPI_Send(&Wprec[ncols], ncols, MPI_FLOAT, rank - 1, 200, MPI_COMM_WORLD);

      //send last to next
      MPI_Send(&Wprec[(nrows - 2) * ncols], ncols, MPI_FLOAT, rank + 1, 200, MPI_COMM_WORLD);
   
      // recv first from next
      MPI_Recv(&Wprec[(nrows - 1) * ncols], ncols, MPI_FLOAT, rank + 1, 200, MPI_COMM_WORLD, NULL);
    

      for (int j = 0; j < ncols; j++)
      {
        // calcule la nouvelle valeur de W[i,j]
        // en utilisant les 8 voisins de la position [i,j] du tableau Wprec
        modif |= calcul_Wij(W, Wprec, m, i, j);
      }
  }
   }


  
    ///**************************Le truc qui avait avant**************************////
     // sera mis à 1 s'il y a une modification

    // calcule le nouveau W fonction de l'ancien (Wprec) en chaque point [i,j]

   /* for (int i = 1; i < nrows - 1; i++)
    {
      for (int j = 0; j < ncols; j++)
      {
        // calcule la nouvelle valeur de W[i,j]
        // en utilisant les 8 voisins de la position [i,j] du tableau Wprec
        modif |= calcul_Wij(W, Wprec, m, i, j);
      }
    }*/

    /*MPI_Reduce(&modif, &end, 1, MPI_INT, MPI_LOR, 0, MPI_COMM_WORLD);
    MPI_Bcast(&end, 1, MPI_INT, 0, MPI_COMM_WORLD);*/

    ////***************************************************************************///

/*#ifdef DARBOUX_PPRINT
    dpprint();
#endif*/

    // échange W et Wprec
    // sans faire de copie mémoire : échange les pointeurs sur les deux tableaux
    float *tmp = W;
    W = Wprec;
    Wprec = tmp;
  }
  // fin du while principal
  // free

  // fin du calcul, le résultat se trouve dans W
  free(Wprec);
  // crée la structure résultat et la renvoie
  mnt *res;
  CHECK((res = malloc(sizeof(*res))) != NULL);
  memcpy(res, m, sizeof(*res));
  res->terrain = W;
  return (res);
}
