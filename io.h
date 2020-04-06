#ifndef __IO_H__
#define __IO_H__

#include "type.h"

struct bcastParam{
  int ligne_per_proc;
  int col_per_proc;
  float max;
};

mnt *mnt_read(char *fname);
void mnt_write(mnt *m, FILE *f);
void mnt_write_lakes(mnt *m, mnt *d, FILE *f);

#endif
