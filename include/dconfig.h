#pragma once

// Renamed to duplicut-config so that pthread's config.h is not confused...

// NOTE: "identifier "size_t" is undefined" so we need to add stdlib.h
#include <stdlib.h>


#define DEFAULT_LINE_MAX_SIZE (64)

struct              conf
{
    const char      *infile_name;
    const char      *outfile_name;
    unsigned int    threads;
    unsigned int    line_max_size;
    size_t          hmap_size;
    size_t          chunk_size;
    int             filter_printable;
    int             lowercase_wordlist;
    int             uppercase_wordlist;
    long            memlimit;
};

/** Global configuration vars.
 * Actually initialized on main.c
 */
extern struct conf  g_conf;

/* source file: config.c */
void                config(void);
