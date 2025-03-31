#include <stdlib.h>
// #include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <termios.h>
#include "uinput.h"
#include "status.h"
#include "const.h"
#include "error.h"
#include <io.h>

#include <stdbool.h>
#include "defines.h"

// Disable warnings with the compiler...
#define _CRT_SECURE_NO_WARNINGS 

#define BUF_SIZE    (1024)


// TODO (Vizonex) if termios is ineffective then drop it.

static struct termios   g_old_tio, g_new_tio;


/** thread worker
 * print progression status with print_status() on keypress
 */
static void *watch_user_input_worker(void *arg)
{
    char        input[BUF_SIZE];

    (void)arg;
    while (1)
    {
#if DEBUG_PROGRAM_STATUS == 0
        memset(input, 0, BUF_SIZE);
# pragma GCC diagnostic push // no need to check read()'s return here
# pragma GCC diagnostic ignored "-Wunused-result"
        _read(STDIN_FILENO, input, BUF_SIZE - 1);
# pragma GCC diagnostic pop
#else
        usleep(100000);
#endif
        display_status();
    }
    return NULL;
}

// Another one that AI helped me come up with.
void displayInputs(bool display) {
    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdIn == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "GetStdHandle failed (error %lu)\n", GetLastError());
        return;
    }
    // TODO (Vizonex) Macro wapper for functions like this with line number where program failure occured...

    DWORD mode;
    HANDLE_WINDOWS_FUNCTION_WITH_VOID(GetConsoleMode, hStdIn, &mode)

    if (display) {
        // Set Flag to allow inputs to be echoed
        HANDLE_WINDOWS_FUNCTION_WITH_VOID(SetConsoleMode, hStdIn, mode | ENABLE_ECHO_INPUT)
         
    } else {
        // Remove Flag to allow inputs to be echoed
        HANDLE_WINDOWS_FUNCTION_WITH_VOID(SetConsoleMode, hStdIn, mode & ~(DWORD)(ENABLE_ECHO_INPUT)) 
        
    }
}

static void restore_termios(void)
{
    

    // Can't do the STDIN_FILENO Trick because we set it up was file pointers.
    // tcsetattr(0, TCSANOW, &g_old_tio);

    // Restore inputs...
    displayInputs(true);
}


/** disable terminal's line buffering & echo
 */
static void config_termios(void)
{   
    displayInputs(false);
    // int getattrerr = tcgetattr(0, &g_old_tio); 
    // if (getattrerr < 0)
    //     error("tcgetattr(): %i", getattrerr);

    // g_new_tio = g_old_tio;
    // g_new_tio.c_lflag &= (~ICANON & ~ECHO);
    
    // int setattrerr = tcsetattr(0, TCSANOW, &g_new_tio);  
    // if (setattrerr < 0)
    //     error("tcsetattr(): %i", setattrerr);

    atexit(restore_termios);
}


void        watch_user_input(void)
{
    pthread_t       tid;

    if (!isatty(0))
        return ;

    config_termios();
    if (pthread_create(&tid, NULL, &watch_user_input_worker, NULL) < 0)
        error("cannot create watch_user_input_worker() thread: %s", ERRNO);
}
