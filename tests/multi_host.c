#include "scan.h"
#include <stdio.h>
#include <stdlib.h>
static int ticks;
static int cancel(const char *p,uint64_t n,void *ctx) {
    (void)p;(void)n;(void)ctx; return ++ticks>4;
}
int main(int argc,char **argv) {
    Catalog c={0}; size_t i;
    scan_directories(&c,(const char *const *)(argv+1),argc-1,getenv("SCAN_CANCEL")?cancel:NULL,NULL);
    printf("%u %u %u %d\n",c.roots_scanned,c.roots_skipped,c.errors,c.cancelled);
    for(i=0;i<c.count;i++) printf("%llu\t%s\n",(unsigned long long)c.items[i].bytes,c.items[i].path);
    catalog_free(&c); return 0;
}
