"""Exercise the actual frame functions with a mocked GPU, including stalled flips."""
from pathlib import Path
import os
import subprocess
from support import compile_test

root = Path(__file__).resolve().parents[1]
source = (root/'source/main.c').read_text()
functions = source[source.index('static int begin(void)'):source.index('static void event(')]
harness = r'''
#include <assert.h>
#include <stdint.h>
static unsigned storage[2][4], *pixels[2]={storage[0],storage[1]};
static int width=2,height=2,buffer,active=1,flip_pending;
static int status=1,polls,events,resets,submits,flushed,waits,submit_error,exit_requested;
static void *gpu;
static uint64_t clock_ms;
static uint64_t millis(void) { clock_ms+=1000; return clock_ms; }
static unsigned buttons(void) { events++; if(exit_requested) active=0; return 0; }
static int gcmGetFlipStatus(void) { polls++; return status; }
static void usleep(int n) { (void)n; }
static void gcmResetFlipStatus(void) { resets++; status=1; }
static int gcmSetFlip(void *g,int b) { (void)g;(void)b; submits++; return submit_error; }
static void rsxFlushBuffer(void *g) { (void)g; flushed++; }
static void gcmSetWaitFlip(void *g) { (void)g; waits++; }
'''
checks = r'''
int main(void) {
    /* Initial WAITING status must not prevent the first submission. */
    assert(begin()==1 && polls==0);
    present(); assert(submits==1 && flushed==1 && waits==1 && flip_pending && buffer==1);
    status=0; assert(begin()==1 && polls==1 && !flip_pending);
    present(); assert(submits==2 && buffer==0);
    /* A stalled GPU must pump input and time out, never draw/submit again. */
    assert(begin()==0 && !active && events>0 && events<=3);
    present(); assert(submits==2);
    active=1; flip_pending=1; exit_requested=1;
    assert(begin()==0 && !active);
    active=1; flip_pending=0; exit_requested=0; submit_error=1;
    present(); assert(!active && !flip_pending && flushed==2);
    return 0;
}
'''
path = root/'build/test-display.c'
path.write_text(harness+functions+checks)
exe = compile_test('test-display', [path])
subprocess.run([str(exe)], check=True)
print('PASS: first frame, completed flip, GPU timeout, exit input, rejected submission')
