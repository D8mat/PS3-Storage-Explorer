#include "paging.h"
#include <assert.h>
int main(void) {
    int heights[]={480,576,720,1080};
    unsigned h;
    for(h=0;h<sizeof(heights)/sizeof(heights[0]);h++) {
        int rows=page_rows(heights[h]), count;
        for(count=0;count<=151;count++) {
            int i;
            for(i=0;i<(count?count:1);i++) {
                int prev=page_move(i,count,rows,-1),next=page_move(i,count,rows,1);
                assert(prev>=0 && next>=0);
                if(!count) { assert(prev==0 && next==0); continue; }
                assert(prev<count && next<count);
                assert(prev/rows==(i/rows?i/rows-1:0));
                assert(next/rows==(i/rows==(count-1)/rows?i/rows:i/rows+1));
            }
        }
        /* List stays above selected-item details, even at SD resolutions. */
        assert(138+(rows-1)*(heights[h]>=700?26:18)+14<heights[h]-105);
    }
    return 0;
}
