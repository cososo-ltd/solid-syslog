/* Newlib system-call retargeting for the QEMU mps2-an385 FreeRTOS image.
 *
 * Replaces the rdimon (semihosting) syscalls. printf and friends route
 * through _write -> CmsdkUart_Write -> the CMSDK UART0 data register, which
 * QEMU surfaces over `-serial stdio`. Reads return EOF for now; slice 3
 * wires _read to the UART receive path so Example/Common/ExampleInteractive
 * can drive `send N` / `quit` over the same serial channel.
 *
 * Not host-TDD'd — this file exists only in the cross build (gated by
 * CMAKE_CROSSCOMPILING + arm in the top-level CMakeLists.txt). The QEMU
 * banner smoke is the integration check that proves the path end-to-end. */

#include "CmsdkUart.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>

/* Small heap reserved for newlib re-entrancy buffers (printf's _impure_ptr).
 * FreeRTOS heap_4 manages the kernel/task heap separately. */
#define SYSCALL_HEAP_SIZE 4096U

static char  syscallHeap[SYSCALL_HEAP_SIZE];
static char* syscallHeapBreak = syscallHeap;

static inline bool IsWithinSyscallHeap(const char* candidateBreak);

void* _sbrk(int increment)
{
    char* previousBreak = syscallHeapBreak;
    char* nextBreak     = syscallHeapBreak + increment;
    void* result        = (void*) -1;
    if (IsWithinSyscallHeap(nextBreak))
    {
        syscallHeapBreak = nextBreak;
        result           = previousBreak;
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
        CmsdkUart_Write(buffer, (size_t) length);
    }
    return length;
}

int _read(int file, char* buffer, int length)
{
    (void) file;
    (void) buffer;
    (void) length;
    return 0;
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
