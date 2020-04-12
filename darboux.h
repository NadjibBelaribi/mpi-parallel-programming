#ifndef __DARBOUX_H__
#define __DARBOUX_H__

#include "type.h"

#define EPSILON .01
float max_terrain(const mnt *restrict m);
 mnt *darboux(const mnt *restrict m);

#endif
