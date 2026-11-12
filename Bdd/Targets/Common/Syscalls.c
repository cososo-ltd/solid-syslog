/* Newlib system-call retargeting for the cross BDD targets.
 *
 * printf and friends route through _write to the board's console
 * (BddTargetBoard.h). _read pulls one byte at a time from the same console,
 * blocking until one arrives, so fgets in
 * Bdd/Targets/Common/BddTargetInteractive can drive `send N` / `quit` over the
 * same serial channel. CR (0x0D) is translated to LF (0x0A) so terminals
 * that send carriage-return on Enter still terminate fgets, and each
 * received byte is echoed back over TX so the user sees what they type.
 *
 * Two threads write the one console, and the board may sleep between bytes, so
 * each _write and each echoed byte holds the console lock from
 * BddTargetOsPrimitives.h. The lock keeps one write whole; it does not make
 * stdio shareable, which is why the pipeline keeps the Service thread's
 * output out of stdio altogether.
 *
 * Not host-TDD'd - this file exists only in the cross build (gated by
 * CMAKE_CROSSCOMPILING + arm in the top-level CMakeLists.txt). The BDD run
 * is the integration check that proves the path end-to-end. */

#include "BddTargetBoard.h"
#include "BddTargetOsPrimitives.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>

/* Small heap reserved for newlib re-entrancy buffers (printf's _impure_ptr).
 * FreeRTOS heap_4 manages the kernel/task heap separately. */
#define SYSCALL_HEAP_SIZE 4096U

static char syscallHeap[SYSCALL_HEAP_SIZE];
static char* syscallHeapBreak = syscallHeap;

static inline bool IsWithinSyscallHeap(const char* candidateBreak);

void* _sbrk(int increment)
{
    char* previousBreak = syscallHeapBreak;
    char* nextBreak = syscallHeapBreak + increment;
    void* result = (void*) -1;
    if (IsWithinSyscallHeap(nextBreak))
    {
        syscallHeapBreak = nextBreak;
        result = previousBreak;
    }
    else
    {
        errno = ENOMEM;
    }
    return result;
}

static inline bool IsWithinSyscallHeap(const char* candidateBreak)
{
    return candidateBreak >= syscallHeap && (size_t) (candidateBreak - syscallHeap) <= sizeof(syscallHeap);
}

int _write(int file, char* buffer, int length)
{
    (void) file;
    if (length > 0)
    {
        BddTargetOsPrimitives_LockConsole();
        BddTargetBoard_ConsoleWrite(buffer, (size_t) length);
        BddTargetOsPrimitives_UnlockConsole();
    }
    return length;
}

int _read(int file, char* buffer, int length)
{
    (void) file;
    int bytesRead = 0;
    if (length > 0)
    {
        char byte = BddTargetBoard_ConsoleRead();
        if (byte == '\r')
        {
            byte = '\n';
        }
        BddTargetOsPrimitives_LockConsole();
        if (byte == '\n')
        {
            BddTargetBoard_ConsoleWrite("\r\n", 2U);
        }
        else
        {
            BddTargetBoard_ConsoleWrite(&byte, 1U);
        }
        BddTargetOsPrimitives_UnlockConsole();
        buffer[0] = byte;
        bytesRead = 1;
    }
    return bytesRead;
}

int _close(int file)
{
    (void) file;
    return -1;
}

int _lseek(int file, int offset, int whence)
{
    (void) file;
    (void) offset;
    (void) whence;
    return 0;
}

int _fstat(int file, struct stat* status)
{
    (void) file;
    status->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void) file;
    return 1;
}

int _kill(int pid, int signal)
{
    (void) pid;
    (void) signal;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}

void _exit(int status)
{
    (void) status;
    for (;;)
    {
    }
}
