// #include <unistd.h>
#include "memstate.h"
#include "meminfo.h"
#include "dconfig.h"
#include "error.h"
#include "const.h"
#include "bytesize.h"

#include <windows.h>
#include <sysinfoapi.h>

#define _CRT_SECURE_NO_DEPRECATE

static int          get_page_size(void)
{
    // TODO: (Vizonex) Fix GetSysteminfo...; 
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    // page_size = sysconf(_SC_PAGESIZE);
    
    if (sys_info.dwPageSize < 0)
        error("sysconf(_SC_PAGESIZE): %s", ERRNO);

    return (sys_info.dwPageSize);
}


static long long    get_mem_available(void)
{
    long long   mem_available;

    mem_available = meminfo(MEMAVAILABLE);
    if (mem_available < 0)
        error("meminfo(MEMAVAILABLE): Cannot determine available memory");
    if (mem_available > g_conf.memlimit)
        mem_available = g_conf.memlimit;

    if (mem_available < MIN_MEMORY)
        error("not enough memory (at least %s required)", sizerepr(MIN_MEMORY));
    return (mem_available);
}


/** Update current memory state.
 * Unlike `init_memstate()`, this function only refreshes
 * their values that may have changed.
 * For example, it does not changes `page_size`, which
 * is a static value.
 */
void        update_memstate(struct memstate *m)
{
    m->mem_available = get_mem_available();
}


/** Initialize current memory state.
 */
void        init_memstate(struct memstate *m)
{
    m->page_size = get_page_size();
    m->mem_available = get_mem_available();
}
