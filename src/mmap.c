/* mmap() replacement for Windows
 *
 * Author: Mike Frysinger <vapier@gentoo.org>
 * Placed into the public domain
 */

/* References:
 * CreateFileMapping: http://msdn.microsoft.com/en-us/library/aa366537(VS.85).aspx
 * CloseHandle:       http://msdn.microsoft.com/en-us/library/ms724211(VS.85).aspx
 * MapViewOfFile:     http://msdn.microsoft.com/en-us/library/aa366761(VS.85).aspx
 * UnmapViewOfFile:   http://msdn.microsoft.com/en-us/library/aa366882(VS.85).aspx
 */

#include <io.h>
#include <windows.h>
#include <sys/types.h>
#include "mmap.h"

void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
 {
     if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC))
         return MAP_FAILED;
     if (fd == -1) {
         if (!(flags & MAP_ANON) || offset)
             return MAP_FAILED;
     } else if (flags & MAP_ANON)
         return MAP_FAILED;
 
     DWORD flProtect;
     if (prot & PROT_WRITE) {
         if (prot & PROT_EXEC)
             flProtect = PAGE_EXECUTE_READWRITE;
         else
             flProtect = PAGE_READWRITE;
     } else if (prot & PROT_EXEC) {
         if (prot & PROT_READ)
             flProtect = PAGE_EXECUTE_READ;
         else if (prot & PROT_EXEC)
             flProtect = PAGE_EXECUTE;
     } else
         flProtect = PAGE_READONLY;
 
     off_t end = length + offset;
     HANDLE mmap_fd, h;
     if (fd == -1)
         mmap_fd = INVALID_HANDLE_VALUE;
     else
         mmap_fd = (HANDLE)_get_osfhandle(fd);
     h = CreateFileMapping(mmap_fd, NULL, flProtect, DWORD_HI(end), DWORD_LO(end), NULL);
     if (h == NULL)
         return MAP_FAILED;
 
     DWORD dwDesiredAccess;
     if (prot & PROT_WRITE)
         dwDesiredAccess = FILE_MAP_WRITE;
     else
         dwDesiredAccess = FILE_MAP_READ;
     if (prot & PROT_EXEC)
         dwDesiredAccess |= FILE_MAP_EXECUTE;
     if (flags & MAP_PRIVATE)
         dwDesiredAccess |= FILE_MAP_COPY;
     void *ret = MapViewOfFile(h, dwDesiredAccess, DWORD_HI(offset), DWORD_LO(offset), length);
     if (ret == NULL) {
         CloseHandle(h);
         ret = MAP_FAILED;
     }
     return ret;
 }
 
int munmap(void *addr, size_t length)
 {  
    // NOTE: (Vizonex made 1 change which is that -1 is returned if it fails...)
    return UnmapViewOfFile(addr) ? 0: -1;

    /* ruh-ro, we leaked handle from CreateFileMapping() ... */
    // Luckily we close that later...
}
 
 #undef DWORD_HI
 #undef DWORD_LO

