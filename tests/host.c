#include "scan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static int ticks;
static int cancel(const char *p,uint64_t b,void *c) {
    (void)p;(void)b;(void)c; return ++ticks>3;
}
int main(int argc,char **argv) {
    Catalog c={0}; size_t i; unsigned char bad[64]={0}; char value[32];
    assert(argc>1);
    assert(!sfo_value(bad,sizeof(bad),"TITLE",value,sizeof(value)));
    memcpy(bad,"\0PSF",4); memset(bad+8,255,12);
    assert(!sfo_value(bad,sizeof(bad),"TITLE",value,sizeof(value)));
    scan_directory(&c,argv[1],argc>2 && !strcmp(argv[2],"cancel")?cancel:NULL,NULL);
    if(argc>2 && !strcmp(argv[2],"asc")) catalog_sort(&c,1);
    printf("STATUS\t%u\t%d\n",c.errors,c.cancelled);
    for(i=0;i<c.count;i++) printf("%s\t%llu\t%llu\t%d\t%u\t%s\n",c.items[i].title,
        (unsigned long long)c.items[i].bytes,(unsigned long long)c.items[i].files,
        c.items[i].complete,c.items[i].errors,c.items[i].id);
    catalog_free(&c); return 0;
}
