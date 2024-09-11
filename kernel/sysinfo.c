#include "sysinfo.h"
#include "memlayout.h"
#include "types.h"
#include "param.h"


extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct sysinfo info;

struct sysinfo *get_sys_info(){
    return &info;
}
void sys_info_init(){
    info.freemem = PHYSTOP - (uint64)end - 134057968;   // just the initial memory size, i dont know why the value
    info.nproc = NPROC;
}

void info_mem_change(uint64 change){
    info.freemem += change;
}

void info_proc_change(uint64 change){
    info.nproc += change;
}