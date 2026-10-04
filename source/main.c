/* PS3 Storage Explorer. Read-only browser; no Cobra-specific calls required. */
#include "scan.h"
#include "paging.h"
#ifndef STORAGE_DIAGNOSTIC
#include "paths.h"
#include <sys/stat.h>
#endif
#include <ppu-lv2.h>
#include <rsx/rsx.h>
#include <rsx/gcm_sys.h>
#include <sysutil/video.h>
#include <sysutil/sysutil.h>
#include <sys/process.h>
#include <io/pad.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>

/* Do not rely on the loader's unspecified default stack size. */
SYS_PROCESS_PARAM(1000, 0x100000);

static gcmContextData *gpu;
static u32 *pixels[2];
static int width,height,buffer,active=1,ascending,selected;
static unsigned previous;
static int flip_pending,scanned;
static int multi_view;
static unsigned buttons(void);
static uint64_t millis(void);
static Catalog catalog;
#ifdef STORAGE_DIAGNOSTIC
#ifdef STORAGE_GAME_TEST
#define TEST_ROOT "/dev_hdd0/game/NPUD21736"
#else
#define TEST_ROOT "/dev_hdd0/STORAGE_TEST"
#endif
static char location[PATH_CAP]=TEST_ROOT;
#else
static char location[PATH_CAP]="/dev_hdd0/game";
#endif
enum { UP=1, DOWN=2, OPEN=4, BACK=8, SORT=16, ROOT=32, EXIT=64, RESCAN=128, PAGE_PREV=256, PAGE_NEXT=512, DEVICE_PREV=1024, DEVICE_NEXT=2048, CLEAR_MARKS=4096 };

/* Original 5x7 bitmap glyphs, row-major. Lowercase is displayed uppercase. */
static const char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .:/_-?[]()+";
static const unsigned char glyphs[][7]={
{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
{17,17,17,17,17,10,4},{17,17,17,21,21,27,17},{17,17,10,4,10,17,17},
{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
{14,17,17,15,1,1,14},{0,0,0,0,0,0,0},{0,0,0,0,0,6,6},
{0,6,6,0,6,6,0},{1,2,2,4,8,8,16},{0,0,0,0,0,0,31},
{0,0,0,31,0,0,0},{14,17,1,2,4,0,4},{14,8,8,8,8,8,14},
{14,2,2,2,2,2,14},{2,4,8,8,8,4,2},{8,4,2,2,2,4,8},{0,4,4,31,4,4,0}};
static void text(int x,int y,const char *s,u32 color) {
    int scale=width>=1000?2:1,origin=x;
    for(;*s;s++,x+=6*scale) {
        const char *g=strchr(alphabet,toupper((unsigned char)*s)); int row,col,dx,dy;
        if(x+5*scale>=width-origin) break;
        if(!g) g=strchr(alphabet,'?');
        for(row=0;row<7;row++) for(col=0;col<5;col++)
            if(glyphs[g-alphabet][row]&(16>>col))
                for(dy=0;dy<scale;dy++) for(dx=0;dx<scale;dx++) {
                    int px=x+col*scale+dx,py=y+row*scale+dy;
                    if(px>=0 && px<width && py>=0 && py<height) pixels[buffer][py*width+px]=color;
                }
    }
}
static int begin(void) {
    int i;
    uint64_t started=millis();
    /* Never wait for a flip before one has actually been submitted. */
    while(flip_pending && gcmGetFlipStatus()!=0) {
        buttons();
        if(!active || millis()-started>=3000) { active=0; return 0; }
        usleep(1000);
    }
    flip_pending=0;
    if(!active) return 0;
    for(i=0;i<width*height;i++) pixels[buffer][i]=0x101925;
    return 1;
}
static void present(void) {
    if(!active) return;
    gcmResetFlipStatus();
    if(gcmSetFlip(gpu,buffer)!=0) { active=0; return; }
    rsxFlushBuffer(gpu);
    gcmSetWaitFlip(gpu);
    flip_pending=1;
    buffer^=1;
}
static void event(u64 status,u64 param,void *userdata) {
    (void)param; (void)userdata;
    if(status==SYSUTIL_EXIT_GAME) active=0;
}
static unsigned buttons(void) {
    padInfo info; padData data; unsigned held=0,result; int i;
    sysUtilCheckCallback();
    memset(&info,0,sizeof(info)); ioPadGetInfo(&info);
    for(i=0;i<MAX_PADS;i++) if(info.status[i]) {
        memset(&data,0,sizeof(data));
        if(ioPadGetData(i,&data)!=0) break;
        if(!data.len) return 0;
        held=(data.BTN_UP?UP:0)|(data.BTN_DOWN?DOWN:0)|(data.BTN_CROSS?OPEN:0)|
             (data.BTN_CIRCLE?BACK:0)|(data.BTN_SQUARE?SORT:0)|(data.BTN_TRIANGLE?ROOT:0)|
             (data.BTN_START?EXIT:0)|(data.BTN_SELECT?RESCAN:0)|
             (data.BTN_L1?PAGE_PREV:0)|(data.BTN_R1?PAGE_NEXT:0)|
             (data.BTN_L2?DEVICE_PREV:0)|(data.BTN_R2?DEVICE_NEXT:0)|(data.BTN_R3?CLEAR_MARKS:0);
        break;
    }
    result=held&~previous; previous=held;
    if(result&EXIT) active=0;
    return result;
}
static uint64_t millis(void) {
    struct timeval now; gettimeofday(&now,NULL);
    return (uint64_t)now.tv_sec*1000+now.tv_usec/1000;
}
static int progress(const char *path,uint64_t bytes,void *ctx) {
    static uint64_t last; uint64_t now=millis(); char line[128]; unsigned key;
    (void)ctx;
    if(now-last<100) return !active;
    last=now; key=buttons();
    if(!active || (key&BACK)) return 1;
    if(!begin()) return 1;
    text(32,32,"PS3 STORAGE EXPLORER - SCANNING",0x5ad9ff);
    text(32,68,path,0xffffff);
    snprintf(line,sizeof(line),"CURRENT ITEM: %.2f GIB   FOUND: %u",(double)bytes/1073741824.,(unsigned)catalog.count);
    text(32,104,line,0xffffff); text(32,140,"CIRCLE: CANCEL / START: EXIT",0xf5c97a); present();
    return 0;
}
static void scan(void) {
    multi_view=0;
    scanned=1; selected=0; scan_directory(&catalog,location,progress,NULL); catalog_sort(&catalog,ascending);
}
#ifndef STORAGE_DIAGNOSTIC
#define CUSTOM_PATH_LIMIT 32
#define MARK_LIMIT 32
static char marked_paths[MARK_LIMIT][PATH_CAP],batch_paths[MARK_LIMIT][PATH_CAP];
static int marked_count,batch_count;
static const char *menu_notice="";
static int mark_index(const char *p) {
    int i; for(i=0;i<marked_count;i++) if(!strcmp(marked_paths[i],p)) return i;
    return -1;
}
static void scan_batch(void) {
    const char *roots[MARK_LIMIT]; int i;
    for(i=0;i<batch_count;i++) roots[i]=batch_paths[i];
    multi_view=1; scanned=1; selected=0;
    scan_directories(&catalog,roots,batch_count,progress,NULL); catalog_sort(&catalog,ascending);
}
static int path_menu,path_choice,device_choice,device_count;
static int devices[STORAGE_DEVICE_COUNT],path_present[STORAGE_PATH_COUNT+CUSTOM_PATH_LIMIT];
static char custom_paths[CUSTOM_PATH_LIMIT][PATH_CAP];
static int custom_count,custom_errors;
static int directory_exists(const char *p) {
    struct stat st; return stat(p,&st)==0 && S_ISDIR(st.st_mode);
}
static void menu_path(int index,char *out) {
    char root[32];
    if(index>=STORAGE_PATH_COUNT) { snprintf(out,PATH_CAP,"%s",custom_paths[index-STORAGE_PATH_COUNT]); return; }
    storage_device(devices[device_choice],root,sizeof(root));
    snprintf(out,PATH_CAP,"%s%s%s",root,storage_paths[index].folder[0]?"/":"",storage_paths[index].folder);
}
static void refresh_paths(void) {
    char p[PATH_CAP]; int i;
    for(i=0;i<STORAGE_PATH_COUNT+custom_count;i++) {
        menu_path(i,p); path_present[i]=directory_exists(p);
    }
}
static void open_paths(void) {
    int i; char p[PATH_CAP]; FILE *f;
    device_count=0; device_choice=0; path_choice=0; custom_count=0; custom_errors=0;
    for(i=0;i<STORAGE_DEVICE_COUNT;i++) {
        storage_device(i,p,sizeof(p));
        if(i==0 || directory_exists(p)) devices[device_count++]=i;
    }
    f=fopen("/dev_hdd0/game/STOR00001/USRDIR/paths.txt","rb");
    if(f) {
        while(fgets(p,sizeof(p),f)) {
            size_t n=strcspn(p,"\r\n");
            if(n==sizeof(p)-1) { int ch; while((ch=fgetc(f))!='\n' && ch!=EOF) {} custom_errors++; continue; }
            p[n]=0;
            if(!n || p[0]=='#') continue;
            if(!storage_local_path(p) || custom_count==CUSTOM_PATH_LIMIT) { custom_errors++; continue; }
            snprintf(custom_paths[custom_count++],PATH_CAP,"%s",p);
        }
        fclose(f);
    }
    refresh_paths(); path_menu=1;
}
static void render_paths(void) {
    char line[256],p[PATH_CAP],root[32]; int row,rows=page_rows(height),step=height>=700?26:18;
    int count=STORAGE_PATH_COUNT+custom_count,page=path_choice/rows;
    if(!begin()) return;
    storage_device(devices[device_choice],root,sizeof(root));
    snprintf(line,sizeof(line),"GAME PATHS - MARKED %d/%d - X: SCAN",marked_count,MARK_LIMIT);
    text(32,24,line,0x5ad9ff);
    snprintf(line,sizeof(line),"DEVICE %d/%d: %s  L2/R2: CHANGE",device_choice+1,device_count,root);
    text(32,52,line,0xffffff);
    snprintf(line,sizeof(line),"PAGE %d/%d  L1/R1: PAGE  CUSTOM PATH ERRORS: %d",page+1,(count+rows-1)/rows,custom_errors);
    text(32,80,line,0xf5c97a);
    text(32,108,"[X] MARKED  [OK] PRESENT  [--] NOT ACCESSIBLE",0xadbccc);
    for(row=0;row<rows;row++) {
        int index=page*rows+row; const char *label;
        if(index>=count) break;
        label=index<STORAGE_PATH_COUNT?storage_paths[index].label:"CUSTOM PATH";
        menu_path(index,p);
        snprintf(line,sizeof(line),"%c %s %s %s - %s",index==path_choice?'+':' ',mark_index(p)>=0?"[X]":"[ ]",path_present[index]?"[OK]":"[--]",
            index<STORAGE_PATH_COUNT?(storage_paths[index].folder[0]?storage_paths[index].folder:"ROOT"):custom_paths[index-STORAGE_PATH_COUNT],label);
        text(32,138+row*step,line,index==path_choice?0x5ad9ff:0xffffff);
    }
    menu_path(path_choice,p); text(32,height-105,p,0xffffff);
    text(32,height-80,menu_notice,0xadbccc);
    text(32,height-55,"SQUARE: MARK/UNMARK  X: SCAN MARKED  CIRCLE: BACK",0xf5c97a);
    text(32,height-32,"R3: CLEAR MARKS  SELECT: REFRESH  START: EXIT",0xf5c97a); present();
}
static void path_keys(unsigned key) {
    int count=STORAGE_PATH_COUNT+custom_count;
    if((key&UP) && path_choice>0) path_choice--;
    if((key&DOWN) && path_choice+1<count) path_choice++;
    if(key&PAGE_PREV) path_choice=page_move(path_choice,count,page_rows(height),-1);
    if(key&PAGE_NEXT) path_choice=page_move(path_choice,count,page_rows(height),1);
    if(key&(DEVICE_PREV|DEVICE_NEXT)) {
        device_choice=(device_choice+((key&DEVICE_PREV)?device_count-1:1))%device_count; refresh_paths();
    }
    if(key&RESCAN) open_paths();
    if(key&CLEAR_MARKS) { marked_count=0; menu_notice="MARKS CLEARED"; }
    if(key&SORT) {
        char p[PATH_CAP]; int i; menu_path(path_choice,p); i=mark_index(p);
        if(i>=0) { memmove(marked_paths[i],marked_paths[i+1],(marked_count-i-1)*PATH_CAP); marked_count--; menu_notice="PATH UNMARKED"; }
        else if(marked_count==MARK_LIMIT) menu_notice="32 PATH LIMIT - UNMARK A PATH FIRST";
        else { snprintf(marked_paths[marked_count++],PATH_CAP,"%s",p); menu_notice="MARKS KEEP THEIR DEVICE WHEN L2/R2 CHANGES DISK"; }
    }
    if(key&(BACK|ROOT)) path_menu=0;
    if(key&OPEN) {
        path_menu=0;
        if(marked_count) { batch_count=marked_count; memcpy(batch_paths,marked_paths,marked_count*PATH_CAP); scan_batch(); }
        else { menu_path(path_choice,location); scan(); }
    }
}
#endif
static void render(void) {
    char line[256]; int row,step=height>=700?26:18,rows=page_rows(height);
    int page=rows>0?selected/rows:0; uint64_t total=0; unsigned errors=catalog.errors; size_t i;
    for(i=0;i<catalog.count;i++) { total+=catalog.items[i].bytes; errors+=catalog.items[i].errors; }
    if(!begin()) return;
#ifdef STORAGE_DIAGNOSTIC
#ifdef STORAGE_GAME_TEST
    text(32,24,"PS3 GAME TEST - NPUD21736",0x5ad9ff);
#else
    text(32,24,"PS3 STORAGE DIAGNOSTIC 0.1.2",0x5ad9ff);
#endif
#else
    text(32,24,"PS3 STORAGE EXPLORER 0.1.7",0x5ad9ff);
#endif
    if(multi_view) {
        snprintf(line,sizeof(line),"COMBINED SCAN: %u PATHS / %u OVERLAPS SKIPPED",catalog.roots_scanned,catalog.roots_skipped);
        text(32,52,line,0xffffff);
    } else text(32,52,location,0xffffff);
    if(!scanned) {
#ifdef STORAGE_DIAGNOSTIC
        text(32,110,"READY - SELECT: SCAN TEST FOLDER / START: EXIT",0x5ad9ff);
#else
        text(32,110,"READY - SELECT: SCAN GAMES / START: EXIT",0x5ad9ff);
#endif
        text(32,148,"NO SCAN RUNNING. TEST START TO RETURN TO XMB FIRST.",0xffffff);
#ifndef STORAGE_DIAGNOSTIC
        text(32,184,"TRIANGLE: CHOOSE PS3 / PS2 / PS1 GAME PATH",0xf5c97a);
#endif
        present(); return;
    }
    snprintf(line,sizeof(line),"%.2f GIB / %u ITEMS / %s / %u ERRORS%s",(double)total/1073741824.,
        (unsigned)catalog.count,ascending?"SMALLEST FIRST":"LARGEST FIRST",errors,catalog.cancelled?" / CANCELLED":"");
    text(32,80,line,0xf5c97a);
    snprintf(line,sizeof(line),"PAGE %u/%u  ITEM %u/%u  L1: PREV / R1: NEXT",
        catalog.count?(unsigned)page+1:0,
        (unsigned)((catalog.count+rows-1)/rows),
        catalog.count?(unsigned)selected+1:0,(unsigned)catalog.count);
    text(32,108,line,0x5ad9ff);
    if(!catalog.count && catalog.errors)
        text(32,160,"FOLDER NOT FOUND / NOT ACCESSIBLE",0xffffff);
#ifdef STORAGE_DIAGNOSTIC
    if(!catalog.count && catalog.errors)
        text(32,138,"CANNOT READ TARGET FOLDER - CHECK PATH VIA FTP",0xffffff);
#endif
    for(row=0;row<rows;row++) {
        int index=page*rows+row; Item *item;
        if(index>=(int)catalog.count) break;
        item=&catalog.items[index];
        snprintf(line,sizeof(line),"%c %8.3f GIB %s %s",index==selected?'+':' ',(double)item->bytes/1073741824.,
            item->complete?(item->directory?"[D]":"[F]"):"[?]",item->title);
        text(32,138+row*step,line,index==selected?0x5ad9ff:0xffffff);
    }
    if(catalog.count) {
        Item *item=&catalog.items[selected];
        text(32,height-105,item->path,0xadbccc);
        snprintf(line,sizeof(line),"%llu BYTES / %llu FILES / ID: %s%s",(unsigned long long)item->bytes,
            (unsigned long long)item->files,item->id[0]?item->id:"UNKNOWN",item->complete?"":" / INCOMPLETE");
        text(32,height-80,line,0xadbccc);
    }
    text(32,height-55,"UP/DOWN: SELECT  X: OPEN  CIRCLE: PARENT  SQUARE: SORT",0xf5c97a);
#ifdef STORAGE_DIAGNOSTIC
    text(32,height-32,"TEST FOLDER ONLY  SELECT: RESCAN  START: EXIT",0xf5c97a);
#else
    text(32,height-32,"TRIANGLE: GAME PATHS  SELECT: RESCAN  START: EXIT",0xf5c97a);
#endif
    present();
}
int main(int argc,char **argv) {
    void *host; videoState state; videoResolution resolution; videoConfiguration config;
    u32 offset; int i; (void)argc; (void)argv;
    host=memalign(1024*1024,32*1024*1024); if(!host) return 1;
#ifdef PSDK3_LEGACY
    gpu=rsxInit(1024*1024,32*1024*1024,host);
#else
    rsxInit(&gpu,1024*1024,32*1024*1024,host);
#endif
    if(!gpu) return 1;
    if(videoGetState(0,0,&state) || videoGetResolution(state.displayMode.resolution,&resolution)) return 1;
    width=resolution.width; height=resolution.height; memset(&config,0,sizeof(config));
    config.resolution=state.displayMode.resolution; config.format=VIDEO_BUFFER_FORMAT_XRGB;
    config.pitch=width*4; config.aspect=state.displayMode.aspect;
    if(videoConfigure(0,&config,NULL,0)) return 1;
    for(i=0;i<2;i++) {
        pixels[i]=rsxMemalign(64,width*height*4); if(!pixels[i]) return 1;
        memset(pixels[i],0,width*height*4);
        if(rsxAddressToOffset(pixels[i],&offset) || gcmSetDisplayBuffer(i,offset,width*4,width,height)) return 1;
    }
    gcmSetFlipMode(GCM_FLIP_VSYNC);
    ioPadInit(7); sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0,event,NULL);
    /* Show a usable frame before any filesystem traversal. */
    while(active) {
        unsigned key=buttons();
#ifndef STORAGE_DIAGNOSTIC
        if(!active) break;
        if(path_menu) { path_keys(key); if(path_menu) render_paths(); else if(active) render(); continue; }
        if(key&ROOT) { open_paths(); render_paths(); continue; }
#endif
        if(key&UP) { if(selected>0) selected--; }
        if(key&DOWN) { if(selected+1<(int)catalog.count) selected++; }
        if(key&PAGE_PREV) selected=page_move(selected,(int)catalog.count,page_rows(height),-1);
        if(key&PAGE_NEXT) selected=page_move(selected,(int)catalog.count,page_rows(height),1);
        if(key&SORT) { ascending=!ascending; catalog_sort(&catalog,ascending); selected=0; }
        if((key&OPEN) && catalog.count && catalog.items[selected].directory) {
            snprintf(location,sizeof(location),"%s",catalog.items[selected].path); scan();
        }
        if(key&BACK) {
#ifndef STORAGE_DIAGNOSTIC
            if(multi_view) { open_paths(); render_paths(); continue; }
#endif
            char *slash=strrchr(location,'/');
#ifdef STORAGE_DIAGNOSTIC
            if(strcmp(location,TEST_ROOT) && slash && slash>=location+strlen(TEST_ROOT)) { *slash=0; scan(); }
#else
            if(slash && slash!=location) { *slash=0; scan(); }
#endif
        }
        if(key&RESCAN) {
#ifndef STORAGE_DIAGNOSTIC
            if(multi_view) scan_batch(); else
#endif
            scan();
        }
        if(active) render();
    }
    catalog_free(&catalog); ioPadEnd(); sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    /* Do not introduce an unbounded GPU wait on the timeout/exit path. */
    /* GameOS reclaims the display buffers and RSX host memory on process exit. */
    return 0;
}
