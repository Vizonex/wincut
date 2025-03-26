#ifndef __DEFINES_H__
#define __DEFINES_H__


#include <stdlib.h>
// #include <io.h>
#include <stdio.h>

// We don't have ssize_t as far as I'm aware...

typedef long long ssize_t;

/* Used to define Macros windows may not originally support */
#ifndef __SIZEOF_POINTER__
    // sizeof(void*) can't be utilized according to this answer 
    // 
    #ifdef _WIN64
        #define __SIZEOF_POINTER__ 8    
    #else 
        #define __SIZEOF_POINTER__ 4
    #endif
#endif


#define isatty _isatty 

#define STDIN_FILENO _fileno(stdin) 
// #define STDERR_FILENO _fileno(stderr)






#endif // __DEFINES_H__