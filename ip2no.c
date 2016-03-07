#include <stdio.h>
#include "../include/global.h"

uint32_t conv(char ipadr[])
{
    uint32_t num=0,val;
    char *tok,*ptr;
    tok=strtok(ipadr,".");
    while( tok != NULL)
    {
        val=strtoul(tok,&ptr,0);
        num=(num << 8) + val;
        tok=strtok(NULL,".");
    }
    return(num);
}
