#ifndef STORAGE_PAGING_H
#define STORAGE_PAGING_H
static inline int page_rows(int height) {
    int rows=(height-254)/(height>=700?26:18);
    return rows>0?rows:1;
}
static inline int page_move(int selected,int count,int rows,int direction) {
    int page,target;
    if(count<=0) return 0;
    page=selected/rows;
    if((direction<0 && page==0) || (direction>0 && page==(count-1)/rows)) return selected;
    target=selected+(direction<0?-rows:rows);
    return target>=count?count-1:target;
}
#endif
