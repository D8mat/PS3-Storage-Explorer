#include "scan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __PPU__
#include <sys/file.h>
#endif

static uint32_t le32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static unsigned le16(const unsigned char *p) { return p[0] | (unsigned)p[1]<<8; }
int sfo_value(const unsigned char *d, size_t n, const char *key, char *out, size_t cap) {
    uint32_t keys, values, count, i;
    if (!cap) return 0;
    out[0]=0;
    if (n<20 || memcmp(d,"\0PSF",4)) return 0;
    keys=le32(d+8); values=le32(d+12); count=le32(d+16);
    if (count>(n-20)/16 || keys>=n || values>=n) return 0;
    for(i=0;i<count;i++) {
        const unsigned char *e=d+20+i*16;
        size_t k=le16(e), v=le32(e+12), len=le32(e+4), actual;
        if(k>=n-keys || v>=n-values || len>n-values-v) continue;
        if(!memchr(d+keys+k,0,n-keys-k)) continue;
        if(strcmp((const char *)d+keys+k,key) || le16(e+2)!=0x0204) continue;
        if(!len || !memchr(d+values+v,0,len)) return 0;
        actual=strlen((const char *)d+values+v);
        if(actual>=cap) actual=cap-1;
        memcpy(out,d+values+v,actual); out[actual]=0; return 1;
    }
    return 0;
}
static int join(char *out, const char *a, const char *b) {
    int n=snprintf(out,PATH_CAP,"%s/%s",a,b);
    return n>=0 && n<PATH_CAP;
}
static void metadata(Item *item) {
    /* Scanner is single-threaded. Keep the 64 KiB buffer off the PPU stack. */
    static unsigned char data[65536];
    const char *suffix[]={"PARAM.SFO","PS3_GAME/PARAM.SFO"};
    unsigned i;
    for(i=0;i<2;i++) {
        char p[PATH_CAP]; FILE *f; size_t n;
        if(!join(p,item->path,suffix[i])) continue;
        f=fopen(p,"rb"); if(!f) continue;
        n=fread(data,1,sizeof(data),f); fclose(f);
        sfo_value(data,n,"TITLE",item->title,sizeof(item->title));
        sfo_value(data,n,"TITLE_ID",item->id,sizeof(item->id));
        if(item->title[0]) return;
    }
}
static int info(const char *p, uint64_t *bytes, int *dir) {
#ifdef __PPU__
    sysFSStat st;
    if(sysLv2FsStat(p,&st)) return 0;
#elif defined(_WIN32)
    struct _stat64 st;
    DWORD attributes=GetFileAttributesA(p);
    if(attributes==INVALID_FILE_ATTRIBUTES || (attributes&FILE_ATTRIBUTE_REPARSE_POINT)) return 0;
    if(_stat64(p,&st)) return 0;
#else
    struct stat st;
    if(lstat(p,&st)) return 0;
#endif
    *dir=S_ISDIR(st.st_mode);
    if(!*dir && !S_ISREG(st.st_mode)) return 0;
    *bytes=*dir?0:(uint64_t)st.st_size;
    return 1;
}
static int walk(Item *item, const char *path, unsigned depth, Progress cb, void *ctx) {
    DIR *d; struct dirent *e;
    if(cb && cb(path,item->bytes,ctx)) return 0;
    if(depth>=64) { item->errors++; return 1; }
    d=opendir(path); if(!d) { item->errors++; return 1; }
    for(;;) {
        char p[PATH_CAP]; uint64_t bytes; int dir;
        errno=0; e=readdir(d);
        if(!e) { if(errno) item->errors++; break; }
        if(!strcmp(e->d_name,".") || !strcmp(e->d_name,"..")) continue;
        if(cb && cb(path,item->bytes,ctx)) { closedir(d); return 0; }
        if(!join(p,path,e->d_name) || !info(p,&bytes,&dir)) { item->errors++; continue; }
        if(dir) { if(!walk(item,p,depth+1,cb,ctx)) { closedir(d); return 0; } }
        else { item->bytes+=bytes; item->files++; }
    }
    closedir(d); return 1;
}
void catalog_free(Catalog *c) { free(c->items); memset(c,0,sizeof(*c)); }
static int append_directory(Catalog *out, const char *root, Progress cb, void *ctx) {
    DIR *d; struct dirent *e; size_t capacity=out->count;
    if(cb && cb(root,0,ctx)) { out->cancelled=1; return 0; }
    d=opendir(root); if(!d) { out->errors++; return 0; }
    for(;;) {
        Item *item; uint64_t bytes; int dir;
        errno=0; e=readdir(d);
        if(!e) { if(errno) out->errors++; break; }
        if(!strcmp(e->d_name,".") || !strcmp(e->d_name,"..")) continue;
        if(cb && cb(root,0,ctx)) { out->cancelled=1; break; }
        if(out->count==ITEM_LIMIT) { out->errors++; break; }
        if(out->count==capacity) {
            Item *next; capacity=capacity?capacity*2:64;
            if(capacity>ITEM_LIMIT) capacity=ITEM_LIMIT;
            next=realloc(out->items,capacity*sizeof(Item));
            if(!next) { out->errors++; break; } out->items=next;
        }
        item=&out->items[out->count++]; memset(item,0,sizeof(*item));
        snprintf(item->title,sizeof(item->title),"%s",e->d_name);
        if(!join(item->path,root,e->d_name) || !info(item->path,&bytes,&dir)) { item->errors++; continue; }
        item->directory=dir;
        if(dir) {
            char fallback[128]; memcpy(fallback,item->title,sizeof(fallback));
            if(cb && cb(item->path,0,ctx)) { out->cancelled=1; break; }
            metadata(item); if(!item->title[0]) memcpy(item->title,fallback,sizeof(fallback));
            if(!walk(item,item->path,0,cb,ctx)) { out->cancelled=1; break; }
        } else { item->bytes=bytes; item->files=1; }
        item->complete=!item->errors;
    }
    closedir(d); return 1;
}
int scan_directory(Catalog *out,const char *root,Progress cb,void *ctx) {
    int result; catalog_free(out); out->roots_scanned=1;
    result=append_directory(out,root,cb,ctx); catalog_sort(out,0); return result;
}
static size_t root_length(const char *p) {
    size_t n=strlen(p); while(n>1 && p[n-1]=='/') n--; return n;
}
int scan_directories(Catalog *out,const char *const *roots,size_t count,Progress cb,void *ctx) {
    size_t i,j; int ok=1;
    catalog_free(out);
    for(i=0;i<count;i++) {
        size_t n=root_length(roots[i]); int covered=0; char root[PATH_CAP];
        if(!n || n>=PATH_CAP) { out->errors++; ok=0; continue; }
        for(j=0;j<count;j++) if(i!=j) {
            size_t m=root_length(roots[j]);
            if(m && m<=n && !strncmp(roots[i],roots[j],m) &&
               ((m==n && j<i) || (m<n && (roots[i][m]=='/' || (m==1 && roots[j][0]=='/'))))) {
                covered=1; break;
            }
        }
        if(covered) { out->roots_skipped++; continue; }
        memcpy(root,roots[i],n); root[n]=0;
        out->roots_scanned++;
        if(!append_directory(out,root,cb,ctx)) ok=0;
        if(out->cancelled) break;
        if(out->count==ITEM_LIMIT) { if(i+1<count) out->errors++; break; }
    }
    catalog_sort(out,0); return ok;
}
static int desc(const void *a, const void *b) {
    const Item *x=a,*y=b;
    if(x->bytes!=y->bytes) return x->bytes<y->bytes?1:-1;
    return strcmp(x->path,y->path);
}
static int asc(const void *a,const void *b) { return desc(b,a); }
void catalog_sort(Catalog *c,int ascending) {
    if(c->count>1) qsort(c->items,c->count,sizeof(Item),ascending?asc:desc);
}
