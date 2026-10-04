#ifndef STORAGE_SCAN_H
#define STORAGE_SCAN_H
#include <stdint.h>
#include <stddef.h>
#define PATH_CAP 1024
#define ITEM_LIMIT 8192
typedef struct {
    char path[PATH_CAP], title[128], id[20];
    uint64_t bytes, files;
    unsigned errors;
    int directory, complete;
} Item;
typedef struct {
    Item *items;
    size_t count;
    unsigned errors;
    int cancelled;
    unsigned roots_scanned, roots_skipped;
} Catalog;
/* Return nonzero to cancel. Called regularly, including during enumeration. */
typedef int (*Progress)(const char *path, uint64_t bytes, void *context);
int scan_directory(Catalog *out, const char *root, Progress progress, void *context);
int scan_directories(Catalog *out, const char *const *roots, size_t count, Progress progress, void *context);
void catalog_free(Catalog *catalog);
void catalog_sort(Catalog *catalog, int ascending);
int sfo_value(const unsigned char *data, size_t size, const char *key, char *out, size_t cap);
#endif
