#include <windows.h>
#include <stdlib.h>
// #include <unistd.h>
#include <string.h>
#include <limits.h>
#include "optparse.h"
#include "dconfig.h"
#include "file.h"
#include "hmap.h"
#include "dedupe.h"
#include "uinput.h"
#include "status.h"
#include "error.h"


struct conf g_conf = {
    .infile_name = NULL,
    .outfile_name = NULL,
    .threads = 0,
    .line_max_size = DEFAULT_LINE_MAX_SIZE,
    .hmap_size = 0,
    .chunk_size = 0,
    .filter_printable = 0,
    .memlimit = LONG_MAX,
};

struct file *g_file;

struct hmap g_hmap;


/** Rewrite file, ignoring lines tagged with 'DISABLED_LINE'
 */
static void     remove_duplicates(void)
{
    t_chunk     file_chunk;
    t_line      line;
    size_t      line_size;
    char        *dst;
    char        *base_ptr;
    int         i;

    file_chunk.ptr = g_file->addr;
    file_chunk.endptr = g_file->addr + g_file->info.st_size;

    i = 0;
    base_ptr = file_chunk.ptr;

    dst = file_chunk.ptr;
    size_t junk_lines = 0;
    while (get_next_line(&line, &file_chunk, &junk_lines))
    {
        line_size = LINE_SIZE(line);
        memmove(dst, LINE_ADDR(line), line_size);
        dst += line_size;
        if (dst != file_chunk.endptr)
            *dst++ = '\n';
        i++;
        if (i == 500000) {
            set_status(FCLEAN_BYTES, (size_t)(file_chunk.ptr - base_ptr));
            base_ptr = file_chunk.ptr;
            i = 0;
        }
    }

    /* update file size */
    g_file->info.st_size = dst - g_file->addr;
    set_status(FCLEAN_BYTES, (size_t)(file_chunk.ptr - base_ptr));
}

// Fixes windows problems with printing colored text...
// This was implemented based off the python colorama libaray...

int init_color(bool enable){
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdOut == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "GetStdHandle failed (error %lu)\n", GetLastError());
        return -1;
    }
    DWORD mode;    
    if (!GetConsoleMode(hStdOut, &mode)){
        fprintf(stderr, "GetConsoleMode failed (error %lu)\n", GetLastError());
        return -1;
    };
    
    if (enable){
        if (!SetConsoleMode(hStdOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)){
            fprintf(stderr, "SetConsoleMode failed (error %lu)\n", GetLastError());
            return -1;
        }
        if (!SetConsoleMode(hStdOut, mode &~(DWORD)(ENABLE_VIRTUAL_TERMINAL_PROCESSING))){
            fprintf(stderr, "SetConsoleMode failed (error %lu)\n", GetLastError());
            return -1;
        }
    }
    return 0;
}

void exit_color(){
    init_color(false);
}


int main(int argc, char **argv)
{
    optparse(argc, argv); /* setup g_conf options */
    if (init_color(true) < 0){
        return;
    }
    // Turn off coloring when we exit...
    atexit(exit_color);

    if (isatty(STDIN_FILENO))
        watch_user_input();

    update_status(FCOPY_START);
    init_file(g_conf.infile_name, g_conf.outfile_name);
    // printf("config();");
    config(); /* configure g_conf options */
    // printf("set_status();");
    set_status(CHUNK_SIZE, g_conf.chunk_size);
    // printf("init_hmap();");
    init_hmap(g_conf.hmap_size);
    // printf("update_status();");
    update_status(TAGDUP_START);
    // printf("tag_duplicates();");
    tag_duplicates();
    // printf("destory_hmap();");
    destroy_hmap();

    update_status(FCLEAN_START);
    remove_duplicates();
    destroy_file();
    
    display_report();
    
    return (0);
}
