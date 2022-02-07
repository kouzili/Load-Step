#ifndef Memory_H__
#define Memory_H__

#include <linux/slab.h>
#include <linux/vmalloc.h>

int Data_Pool_Init(void);

void Data_Pool_Free(void);

#endif