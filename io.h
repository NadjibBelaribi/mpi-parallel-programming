#ifndef __IO_H__
#define __IO_H__

#include "type.h"

// structure pour un message broadcast
struct bcastParam
{
  float max;
  float no_data;
  int ligne_per_proc;
  int col_per_proc;
  int first_rows ;
};

mnt *mnt_read(char *fname);
void mnt_write(mnt *m, FILE *f);
void mnt_write_lakes(mnt *m, mnt *d, FILE *f);

#endif
