#include <stdlib.h>
// #include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>

#include <io.h>
#include <sys/types.h>

// AI Told me - Vizonex

static int vdprintf(int fd, const char* format, va_list ap) {
    FILE* f = fdopen(fd, "w"); // Convert file descriptor to FILE* pointer
    int rc = vfprintf(f, format, ap); // Write formatted output
    fclose(f); // Close the file
    return rc;
}

/** Ignore warning:
 * ignoring return value of ‘write’, declared with attribute warn_unused_result
 * Because we write to STDERR_FILENO & don't need to check write() return.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

#define STDERR_FILENO 2
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

/** Print a formatted warning message without leaving the program.
 * The use of printf() inside this function causes it
 * to potentially allocate memory.
 * Unlike error() and die(), this function do not leave the program,
 * making it safe for use in atexit() callbacks.
 */
void            warning(const char *fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    write(STDERR_FILENO, "warning: ", 9);
    vdprintf(STDERR_FILENO, fmt, ap);
    write(STDERR_FILENO, "\n", 1);
    va_end(ap);
}


/** Print a formatted error message and leave the program.
 * The use of printf() inside this function causes it
 * to potentially allocate memory.
 * Memory allocation errors shall then use die() instead.
 */
void            error(const char *fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    write(STDERR_FILENO, "\nerror: ", 8);
    vdprintf(STDERR_FILENO, fmt, ap);
    write(STDERR_FILENO, "\n", 1);
    va_end(ap);
    exit(1);
}


/** Suddenly exit the program with fatal error.
 * Unlike error(), this function does not allocates
 * any additional memory before exiting program.
 */
void            die(const char *msg)
{
    write(STDERR_FILENO, "\nfatal error: ", 14);
    perror(msg);
    exit(1);
}

#pragma GCC diagnostic pop
