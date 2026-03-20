#pragma once
#include <cstdint>
#include <string>

namespace exms::compat::linux::syscall {

enum class SyscallId : uint64_t {
    read = 0,
    write = 1,
    open = 2,
    close = 3,
    mmap = 9,
    munmap = 11,
    brk = 12,
    rt_sigaction = 13,
    rt_sigprocmask = 14,
    ioctl = 16,
    pipe = 22,
    yield = 24,
    mremap = 25,
    nanosleep = 35,
    getpid = 39,
    socket = 41,
    connect = 42,
    accept = 43,
    sendto = 44,
    recvfrom = 45,
    clone = 56,
    fork = 57,
    execve = 59,
    exit = 60,
    wait4 = 61,
    kill = 62,
    uname = 63,
    fcntl = 72,
    fsync = 74,
    ftruncate = 77,
    getcwd = 79,
    chdir = 80,
    rename = 82,
    mkdir = 83,
    unlink = 87,
    readlink = 89,
    gettimeofday = 96,
    getuid = 102,
    getgid = 104,
    gettid = 186,
    futex = 202,
    epoll_create = 213,
    epoll_ctl = 233,
    epoll_wait = 232,
    openat = 257,
    pipe2 = 293,
    epoll_create1 = 291,
    eventfd2 = 290,
    accept4 = 288,
};

struct SyscallResult {
    long value = -1;
    bool handled = false;
    std::string route;
};

const char* name_of(uint64_t no);

SyscallResult dispatch(uint64_t no,
                       uint64_t a0,
                       uint64_t a1,
                       uint64_t a2,
                       uint64_t a3,
                       uint64_t a4,
                       uint64_t a5);

} // namespace exms::compat::linux::syscall
