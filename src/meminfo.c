#include <stdlib.h>
#include <stdio.h>

#include "meminfo.h"
#include "error.h"
#include "bytesize.h"

#include "alloc.h"

// Because windows does not have a proper getline implementation
// we have to get creative...
// Source: https://github.com/digilus/getline/blob/master/getline.c
// Changes:
//      - mimalloc instead of stdlib's versions...

#define GETLINE_MINSIZE 16
long long getline(char **lineptr, size_t *n, FILE *stream) {
    char *buffer = NULL;
    size_t capacity = 0;
    size_t size = 0;
    int c;

    while ((c = getc(stream)) != EOF) {
        if (size >= capacity) {
            capacity = (capacity == 0) ? 32 : capacity * 2;
            buffer = nrealloc(buffer, capacity);
            if (buffer == NULL) {
                return -1;
            }
        }
        buffer[size++] = c;
        if (c == '\n') {
            break;
        }
    }
    if (c == EOF && size == 0) {
        return -1;
    }
    buffer[size] = '\0';
    *lineptr = buffer;
    *n = size;
    return size;
}

#ifdef __APPLE__ /* Mac OS X case (host_statistics64()) */

#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

/** Apple Mac OS X specific wrapper for meminfo_memavailable()
 *
 * Specific code needed because Mac OS X doesn't have "/proc" vfs.
 */
static long long meminfo_memavailable(void)
{
    vm_size_t page_size;
    mach_port_t mach_port;
    mach_msg_type_number_t count;
    vm_statistics64_data_t vm_stats;

    mach_port = mach_host_self();
    count = sizeof(vm_stats) / sizeof(natural_t);
    if (host_page_size(mach_port, &page_size) != KERN_SUCCESS)
    {
        error("couldn't retrieve 'page size' from host_page_size()");
    }
    if (host_statistics64(mach_port, HOST_VM_INFO,
                          (host_info64_t)&vm_stats, &count) != KERN_SUCCESS)
    {
        error("couldn't retrieve 'vm info' from host_statistics64()");
    }
    return ((long long)vm_stats.free_count * (long long)page_size);
}

#else /* Unix default case (through /proc/meminfo) */
#include <windows.h>
#define PROC_MEMINFO_FILE ("/proc/meminfo")
#define BUF_SIZE (1024)

/** Retrieve value from given /proc/meminfo line.
 */
static long long get_value(char *ptr, const char *str, size_t str_len)
{
    if (strncmp(ptr, str, str_len) == 0)
    {
        ptr += str_len;
        if (*ptr == ':')
        {
            ptr += 1;
            return (bytesize(ptr));
        }
    }
    return (-1L);
}


/** NOTE: (Vizonex) Windows is a little bit different but it got so optimzied that I could 
 * Forget about proc_meminfo alltogether...
 */

/** Get identifier value from /proc/meminfo
 *
 * The proc_meminfo() function returns the value in bytes of
 * an identifier from /proc/meminfo file.
 *
 * If identifier could not be found, the function returns -1.
 */
// static long long proc_meminfo(_proc_identifier identifier)
// {
    // char *buf;
    // size_t size;
    // FILE *fp;
    // long long result;
    // size_t identifier_len;



    // fp = fopen(PROC_MEMINFO_FILE, "r");
    // if (fp == NULL)
    //     error("cannot open %s: %s", PROC_MEMINFO_FILE, ERRNO);

    // size = BUF_SIZE * sizeof(*buf);
    // buf = (char *)malloc(size);
    // if (buf == NULL)
    //     die("cannot malloc() proc_meminfo buffer");

    // result = -1LL;
    // identifier_len = strlen(identifier);
    // while (getline(&buf, &size, fp) >= 0)
    // {
    //     printf("BUFFER: %s", buf);
    //     result = get_value(buf, identifier, identifier_len);
    //     if (result >= 0)
    //         break;
    // }
    // fclose(fp);
    // free((void *)buf);
    // return (result);
// }

/** Default meminfo_memavailable() function.
 *
 * This wrapper calls /proc/meminfo through proc_meminfo()
 * static function.
 *
 * "MemAvailable" represents memory which can really be
 * used without swapping, taking care of important things
 * such as shared memory segments, tmpfs, ramfs and
 * reclaimable slab memory.
 *
 * Therefore, it is only available on recent linux kernels,
 * so we fallback to "MemFree" if "MemAvailable" is not
 * provided.
 */
static long long meminfo_memavailable(void)
{
    // It was so optimzied that we could literally forget the other function alltogether...

    MEMORYSTATUSEX meminfo;
    meminfo.dwLength = sizeof(MEMORYSTATUSEX);
    
    if (!GlobalMemoryStatusEx(&meminfo)) {
        fprintf(stderr, "GlobalMemoryStatusEx failed (error %lu)\n", GetLastError());
        return -1LL;
    }
    // ullAvalilPhys otherwise try ullAvailPageFile
    return ( meminfo.ullAvailPhys < 0) ? meminfo.ullAvailPageFile : meminfo.ullAvailPhys;
}

#endif

/** Return requested meminfo parameter.
 *
 * If it fails to retrieve information, the function
 * returns -1.
 */
long long meminfo(enum e_meminfo_param info)
{
    switch (info)
    {
    case MEMAVAILABLE:
        return (meminfo_memavailable());
    default:
        error("meminfo(): Invalid argument: %d", info);
    }
    return (-1LL);
}
