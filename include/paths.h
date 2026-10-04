#ifndef STORAGE_PATHS_H
#define STORAGE_PATHS_H
#include <stdio.h>
#include <string.h>
#define STORAGE_DEVICE_COUNT 132
typedef struct { const char *folder,*label; } StoragePath;
#define PROFILE_PATHS(n) \
    {"GAMES_" #n,"PS3 PROFILE " #n},{"GAMEZ_" #n,"PS3 PROFILE " #n}, \
    {"PS3ISO_" #n,"PS3 PROFILE " #n},{"PSXISO_" #n,"PS1 PROFILE " #n}, \
    {"PS2ISO_" #n,"PS2 PROFILE " #n},{"PSPISO_" #n,"PSP PROFILE " #n}, \
    {"ROMS_" #n,"ROMS PROFILE " #n},{"BDISO_" #n,"BD PROFILE " #n},{"DVDISO_" #n,"DVD PROFILE " #n}
static const StoragePath storage_paths[]={
    {"game","INSTALLED PS3 / PS1 / PS2 CLASSICS"},
    {"GAMES","PS3 FOLDER GAMES"},{"GAMEZ","PS3 FOLDER GAMES"},
    {"PS3ISO","PS3 DISC IMAGES"},{"PSXISO","PS1 DISC IMAGES"},
    {"PSXGAMES","PS1 GAME FOLDERS"},{"PS2ISO","PS2 ISO / BIN.ENC CLASSICS"},
    {"PS2DISC","PS2 EXTRACTED DISCS"},{"CD","PS2 OPL CD IMAGES"},
    {"DVD","PS2 OPL DVD IMAGES"},{"GAMEI","EXTERNAL INSTALLED GAMES"},
    {"PSPISO","PSP DISC IMAGES"},{"ISO","PSP ISO FOLDER"},
    {"ROMS/PSXISO","RETRO PS1 IMAGES"},{"ROMS/PS2ISO","RETRO PS2 IMAGES"},
    {"ROMS","OTHER EMULATOR GAMES"},
    {"GAMES_DUP","MANAGER DUPLICATES"},{"GAMES_BAD","MANAGER BAD COPIES"},
    {"GAMES [auto]","PS3 AUTO FOLDER"},{"GAMEZ [auto]","PS3 AUTO FOLDER"},
    {"PS3ISO [auto]","PS3 AUTO IMAGES"},{"PSXISO [auto]","PS1 AUTO IMAGES"},
    {"PS2ISO [auto]","PS2 AUTO IMAGES"},{"PSPISO [auto]","PSP AUTO IMAGES"},
    {"packages","PKG INSTALLERS"},{"Packages","PKG INSTALLERS"},{"PKG","PKG INSTALLERS"},
    {"video","VIDEO / ALTERNATIVE ISO STORAGE"},
    {"BDISO","BLURAY / DATA IMAGES"},{"DVDISO","DVD / DATA IMAGES"},
    {"game/LAUN12345/GAMEZ","LEGACY BACKUP MANAGER"},
    {"game/OMAN46756/GAMEZ","LEGACY OPEN MANAGER"},
    {"game/BLES80608/USRDIR/GAMES","MULTIMAN CUSTOM STORAGE"},
    PROFILE_PATHS(1),PROFILE_PATHS(2),PROFILE_PATHS(3),PROFILE_PATHS(4),
    {"","DEVICE ROOT / OTHER FOLDERS"}
};
#undef PROFILE_PATHS
#define STORAGE_PATH_COUNT ((int)(sizeof(storage_paths)/sizeof(storage_paths[0])))
static inline void storage_device(int index,char *out,size_t cap) {
    if(index==0) snprintf(out,cap,"/dev_hdd0");
    else if(index<=128) snprintf(out,cap,"/dev_usb%03d",index-1);
    else snprintf(out,cap,"%s",index==129?"/dev_sd":index==130?"/dev_ms":"/dev_cf");
}
/* Presets can be supplemented by any local path in the user's config. */
static inline int storage_local_path(const char *p) {
    const char *tail=NULL;
    if(!strncmp(p,"/dev_hdd0",9)) tail=p+9;
    else if(!strncmp(p,"/dev_usb",8) && strlen(p)>=11 &&
        p[8]>='0' && p[8]<='9' && p[9]>='0' && p[9]<='9' && p[10]>='0' && p[10]<='9') tail=p+11;
    else if(!strncmp(p,"/dev_sd",7)||!strncmp(p,"/dev_ms",7)||!strncmp(p,"/dev_cf",7)) tail=p+7;
    if(!tail || (*tail && *tail!='/')) return 0;
    while(*tail) {
        const char *end; size_t len;
        tail++; end=strchr(tail,'/'); len=end?(size_t)(end-tail):strlen(tail);
        if(!len || (len==1 && tail[0]=='.') || (len==2 && tail[0]=='.' && tail[1]=='.')) return 0;
        tail+=len;
    }
    return 1;
}
#endif
