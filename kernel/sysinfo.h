#include "types.h"

struct sysinfo {
  uint64 freemem;   // amount of free memory (bytes)
  uint64 nproc;     // number of process
};

struct sysinfo *get_sys_info();
void sys_info_init();
void info_mem_change(uint64 change);
void info_proc_change(uint64 change);