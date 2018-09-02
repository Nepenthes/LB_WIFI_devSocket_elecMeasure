#ifndef __PARSMETHOD_H_
#define __PARSMETHOD_H_

#include "STC15Fxxxx.H"

void *memmem(void *start, unsigned char s_len, void *find, unsigned char f_len);
int memloc(u8 mem2[],u8 num_s2,u8 mem1[],u8 num_s1);

#endif