#include "pars_Method.h"
#include "string.h"

void *memmem(void *start, unsigned char s_len, void *find, unsigned char f_len){

	unsigned char len	= 0;
			char *p		= start, 
				 *q		= find;
	
	while((p - (char *)start + f_len) <= s_len){
	
		while(*p ++ == *q ++){
		
			len ++;
			if(len == f_len)return (p - f_len);
		}
		
		q 	= find;
		len = 0;
	}
	
	return NULL;
}

int memloc(u8 mem2[],u8 num_s2,u8 mem1[],u8 num_s1)
{
	int la = num_s1;
	int i, j;
	int lb = num_s2;
	for(i = 0; i < lb; i ++)
	{
		for(j = 0; j < la && i + j < lb && mem1[j] == mem2[i + j]; j ++);
		if(j == la)return i;
	}
	return -1;
}