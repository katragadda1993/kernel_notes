#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

static long raw_getpid(void)
{
#if defined(__x86_64__)
    long ret;
    __asm__ volatile ("syscall"
                      : "=a" (ret)
                      : "0" ((long)SYS_getpid)
                      : "rcx", "r11", "memory");
    return ret;
#elif defined(__aarch64__)
    register long x0 __asm__("x0");
    register long x8 __asm__("x8") = SYS_getpid;
    __asm__ volatile ("svc 0" : "=r" (x0) : "r" (x8) : "memory");
    return x0;
#else
    return syscall(SYS_getpid);
#endif
}

int main(void)
{
    long libc_pid = syscall(SYS_getpid);
    long raw_pid = raw_getpid();

    printf("libc syscall() getpid: %ld\n", libc_pid);
    printf("raw architecture syscall getpid: %ld\n", raw_pid);

    errno = 0;
    long invalid = syscall(0x7fffffffL);
    int saved_errno = errno;
    printf("invalid syscall: ret=%ld errno=%d (%s)\n",
           invalid, saved_errno, strerror(saved_errno));

    return (libc_pid == raw_pid && invalid == -1 && saved_errno == ENOSYS) ? 0 : 1;
}
