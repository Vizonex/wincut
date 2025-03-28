#include <windows.h>
#include <stdlib.h>
// Obviously can't do that...
// #include <unistd.h> 

#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

// Renamed to not confuse with the pthread folder.
// #include "config.h"


#include <mmiscapi.h>
#include "file.h"
#include "const.h"
#include "status.h"
#include "error.h"
#include "debug.h"
#include "bytesize.h"
#include "defines.h"
#include "mman.h"
#include "mkstemp.h"
#include "dconfig.h"


// SEE: https://stackoverflow.com/a/62371749
// Windows does not define the S_ISREG and S_ISDIR macros in stat.h, so we do.
// We have to define _CRT_INTERNAL_NONSTDC_NAMES 1 before #including sys/stat.h
// in order for Microsoft's stat.h to define names like S_IFMT, S_IFREG, and S_IFDIR,
// rather than just defining  _S_IFMT, _S_IFREG, and _S_IFDIR as it normally does.
#define _CRT_INTERNAL_NONSTDC_NAMES 1
#if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
  #define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISDIR) && defined(S_IFMT) && defined(S_IFDIR)
  #define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif



// According to stack overflow...
// SEE: https://stackoverflow.com/a/76196125
#include <io.h>
#define ftruncate _chsize_s


#define FILE_ISSET(_f) ((_f)->fd >= 0)
#define BUF_SIZE       (0x20000) /* 128kb */


static struct file  g_infile;
static struct file  g_outfile;
static struct file  g_tmpfile;

// TODO: (Vizonex) Allow very large temporary files to be redirected to 
// other places especially 100GB ones like rockyou2021.txt


/** Ensure that infile and outfile don't point to the same
 * file on filesystem, by comparing device & inode.
 */
static void check_files(const char *infile_name, const char *outfile_name)
{
    struct stat     infile_info;
    struct stat     outfile_info;

    if (stat(infile_name, &infile_info) < 0)
        return ;
    if (stat(outfile_name, &outfile_info) < 0)
        return ;

    if (infile_info.st_dev == outfile_info.st_dev
            && infile_info.st_ino == outfile_info.st_ino)
        error("infile and outfile must be different");
}


/** Safely close a `file`.
 */
static void close_file(struct file *file)
{
    if (FILE_ISSET(file))
    {
        if (close(file->fd) < 0)
            warning("cannot close %s: %s", file->name, ERRNO);
        file->fd = -1;
    }
}


/** Destructor callback for potentially open files
 * Close infile, outfile and tmpfile (if exists)
 */
static void close_all(void)
{
    close_file(&g_infile);
    close_file(&g_outfile);

    if (FILE_ISSET(&g_tmpfile))
    {
        close_file(&g_tmpfile);
        if (unlink(g_tmpfile.name) < 0)
            warning("cannot unlink %s: %s", g_tmpfile.name, ERRNO);
    }
}


/** Open (`pathname`, `flags`) -> `file`
 */
static void open_file(struct file *file, const char *pathname, int flags)
{
    if ((file->fd = open(pathname, flags, 0666)) < 0)
        error("couldn't open %s: %s", pathname, ERRNO);

    if (fstat(file->fd, &(file->info)) < 0)
        error("couldn't stat %s: %s", pathname, ERRNO);

    file->name = pathname;
}


/** Specific opener for g_tmpfile, which also uses mkstemp.
 * This function is a singleton (should be called once).
 */
static void create_tmpfile(void)
{
    struct file *file = &g_tmpfile;
    static char template[] = PROGNAME "_tmpfile.XXXXXX";

    if ((file->fd = mkstemp(template)) < 0)
        error("couldn't create tmpfile '%s': %s", template, ERRNO);

    if (fstat(file->fd, &(file->info)) < 0)
        error("couldn't stat tmpfile '%s': %s", template, ERRNO);

    file->name = template;
}


static void buf_tolower(char *str, ssize_t size)
{
    while (size--)
    {
        str[size] = tolower(str[size]);
    }
}


static void buf_toupper(char *str, ssize_t size)
{
    while (size--)
    {
        str[size] = toupper(str[size]);
    }
}


/** Copy file from src_fd to dst_fd
 */
static void copy_file(int dst_fd, int src_fd, bool send_status)
{
    char        buffer[BUF_SIZE];
    ssize_t     nread;
    unsigned int read_bytes;

// #if _POSIX_C_SOURCE >= 200112L
//     posix_fadvise(src_fd, 0, 0, POSIX_FADV_SEQUENTIAL);
// #endif
    read_bytes = 0;
    while ((nread = (ssize_t)read(src_fd, buffer, BUF_SIZE)) > 0)
    {
        char    *dst_ptr = buffer;
        ssize_t nwrite;

        if (g_conf.lowercase_wordlist)
            buf_tolower(buffer, nread);
        else if (g_conf.uppercase_wordlist)
            buf_toupper(buffer, nread);

        read_bytes += nread;

        do
        {
            if ((nwrite = write(dst_fd, dst_ptr, (unsigned int) nread)) >= 0)
            {
                nread -= nwrite;
                dst_ptr += nwrite;
            }
            else if (errno != EINTR)
            {
                error("copy_file() -> write(): %s", ERRNO);
            }
        } while (nread > 0);

        if (send_status && read_bytes >= BUF_SIZE * 0x100) {
            set_status(FCOPY_BYTES, read_bytes);
            read_bytes = 0;
        }
    }
    if (nread != 0)
        error("copy_file() -> read(): %s", ERRNO);
}


/** file constructor.
 * - Handle src/dst files, and return a struct file* for use by duplicut.
 * - Can deal with non-regular files
 * - Registers cleanup functions with atexit()
 * - returned file has `addr` attribute mapped in memory
 */
void        init_file(const char *infile_name, const char *outfile_name)
{
    struct file  *file;

    check_files(infile_name, outfile_name);

    memset(&g_infile, -1, sizeof(g_infile));
    memset(&g_outfile, -1, sizeof(g_infile));
    memset(&g_tmpfile, -1, sizeof(g_infile));

    atexit(close_all);
    open_file(&g_infile, infile_name, O_RDONLY);
    open_file(&g_outfile, outfile_name, O_TRUNC | O_CREAT | O_RDWR);

    if (S_ISREG(g_outfile.info.st_mode))
        file = &g_outfile;
    else
    {
        create_tmpfile();
        file = &g_tmpfile;
    }

    set_status(FILE_SIZE, g_infile.info.st_size);
    copy_file(file->fd, g_infile.fd, 1);
    close_file(&g_infile);
    
    if (fstat(file->fd, &(file->info)) < 0)
        error("couldn't stat %s: %s", file->name, ERRNO);
    if (file->info.st_size == 0)
        exit(0);

    file->addr = mmap(NULL, (size_t) file->info.st_size,
            (PROT_READ | PROT_WRITE), MAP_SHARED, file->fd, 0);
    if (file->addr == MAP_FAILED)
        error("couldn't mmap %s: %s", file->name, ERRNO);

    file->orig_size = file->info.st_size;
    g_file = file;

    DLOG1("");
    DLOG1("---------- g_file ------------");
    DLOG1("g_file->fd:           %d", g_file->fd);
    DLOG1("g_file->name:         %s", g_file->name);
    DLOG1("g_file->addr:         %p", g_file->addr);
    DLOG1("g_file->info.st_size: %s (%ld)",
            sizerepr(g_file->info.st_size), g_file->info.st_size);
    DLOG1("------------------------------");
}


/** file destructor
 * - If tmpfile, copy it info outfile
 * - truncate file with new size.
 * - Call close_all() for cleanout.
 */
void        destroy_file(void)
{
    struct file     *file;

    if (FILE_ISSET(&g_tmpfile))
        file = &g_tmpfile;
    else
        file = &g_outfile;

    if (munmap(file->addr, file->orig_size) < 0)
        error("cannot munmap() %s: %s", file->name, ERRNO);
    if (ftruncate(file->fd, file->info.st_size) < 0)
        error("cannot ftruncate() %s: %s", file->name, ERRNO);

    if (FILE_ISSET(&g_tmpfile))
    {
        lseek(g_tmpfile.fd, 0, SEEK_SET);
        copy_file(g_outfile.fd, g_tmpfile.fd, 0);
    }

    close_all();
}
