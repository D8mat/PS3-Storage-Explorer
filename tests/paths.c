#include "paths.h"
#include <assert.h>
int main(void) {
    char p[80],q[80]; int i,j;
    for(i=0;i<STORAGE_DEVICE_COUNT;i++) {
        storage_device(i,p,sizeof(p)); assert(storage_local_path(p));
        for(j=0;j<i;j++) { storage_device(j,q,sizeof(q)); assert(strcmp(p,q)); }
    }
    storage_device(1,p,sizeof(p)); assert(!strcmp(p,"/dev_usb000"));
    storage_device(128,p,sizeof(p)); assert(!strcmp(p,"/dev_usb127"));
    for(i=0;i<STORAGE_PATH_COUNT;i++) {
        snprintf(p,sizeof(p),"/dev_hdd0%s%s",storage_paths[i].folder[0]?"/":"",storage_paths[i].folder);
        assert(storage_local_path(p));
        for(j=0;j<i;j++) assert(strcmp(storage_paths[i].folder,storage_paths[j].folder));
    }
    assert(storage_local_path("/dev_hdd0/My Games/PS1"));
    assert(storage_local_path("/dev_usb006/PS2ISO"));
    assert(!storage_local_path("/dev_hdd0/../dev_flash"));
    assert(!storage_local_path("/dev_hdd0evil/PS2ISO"));
    assert(!storage_local_path("/dev_hdd0//PS2ISO"));
    assert(!storage_local_path("/dev_hdd0/PS2ISO/"));
    assert(!storage_local_path("/net0/PSXISO"));
    assert(!storage_local_path("/ntfs0/PS2ISO"));
    assert(!storage_local_path("/dev_usb"));
    assert(!storage_local_path("/dev_flash"));
    return 0;
}
