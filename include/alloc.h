#ifndef __ALLOC_H__
#define __ALLOC_H__


#if USE_MIMALLOC >= 1
    #include "mimalloc.h"
    #define nmalloc mi_malloc
    #define nrealloc mi_realloc
    #define nfree mi_free
    #define ncalloc mi_calloc

#else /* STDLIB it is... */

    #include <stdlib.h>
    #define nmalloc malloc
    #define nrealloc realloc
    #define nfree free
    #define ncalloc calloc
#endif


#endif // __ALLOC_H__