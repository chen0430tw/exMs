#include "dispatcher.hpp"
#include <unordered_map>

namespace exms::compat::linux::syscall {

const char* name_of(uint64_t no) {
    switch (no) {
        case static_cast<uint64_t>(SyscallId::read): return "read";
        case static_cast<uint64_t>(SyscallId::write): return "write";
        case static_cast<uint64_t>(SyscallId::open): return "open";
        case static_cast<uint64_t>(SyscallId::close): return "close";
        case static_cast<uint64_t>(SyscallId::mmap): return "mmap";
        case static_cast<uint64_t>(SyscallId::munmap): return "munmap";
        case static_cast<uint64_t>(SyscallId::brk): return "brk";
        case static_cast<uint64_t>(SyscallId::pipe): return "pipe";
        case static_cast<uint64_t>(SyscallId::clone): return "clone";
        case static_cast<uint64_t>(SyscallId::fork): return "fork";
        case static_cast<uint64_t>(SyscallId::execve): return "execve";
        case static_cast<uint64_t>(SyscallId::exit): return "exit";
        case static_cast<uint64_t>(SyscallId::wait4): return "wait4";
        case static_cast<uint64_t>(SyscallId::futex): return "futex";
        case static_cast<uint64_t>(SyscallId::epoll_create): return "epoll_create";
        case static_cast<uint64_t>(SyscallId::epoll_ctl): return "epoll_ctl";
        case static_cast<uint64_t>(SyscallId::epoll_wait): return "epoll_wait";
        case static_cast<uint64_t>(SyscallId::epoll_create1): return "epoll_create1";
        case static_cast<uint64_t>(SyscallId::openat): return "openat";
        case static_cast<uint64_t>(SyscallId::pipe2): return "pipe2";
        default: return "unknown";
    }
}

SyscallResult dispatch(uint64_t no,
                       uint64_t,
                       uint64_t,
                       uint64_t,
                       uint64_t,
                       uint64_t,
                       uint64_t) {
    SyscallResult r{};
    switch (no) {
        case static_cast<uint64_t>(SyscallId::read):
        case static_cast<uint64_t>(SyscallId::write):
        case static_cast<uint64_t>(SyscallId::open):
        case static_cast<uint64_t>(SyscallId::close):
        case static_cast<uint64_t>(SyscallId::openat):
            r.handled = true;
            r.route = "exabi.fs";
            r.value = -38; // ENOSYS placeholder implementation path accepted but not completed
            return r;

        case static_cast<uint64_t>(SyscallId::mmap):
        case static_cast<uint64_t>(SyscallId::munmap):
        case static_cast<uint64_t>(SyscallId::brk):
            r.handled = true;
            r.route = "expdmf.mem";
            r.value = -38;
            return r;

        case static_cast<uint64_t>(SyscallId::clone):
        case static_cast<uint64_t>(SyscallId::fork):
        case static_cast<uint64_t>(SyscallId::execve):
        case static_cast<uint64_t>(SyscallId::wait4):
            r.handled = true;
            r.route = "expdmf.proc";
            r.value = -38;
            return r;

        case static_cast<uint64_t>(SyscallId::pipe):
        case static_cast<uint64_t>(SyscallId::pipe2):
        case static_cast<uint64_t>(SyscallId::futex):
            r.handled = true;
            r.route = "runtime.sync";
            r.value = -38;
            return r;

        case static_cast<uint64_t>(SyscallId::epoll_create):
        case static_cast<uint64_t>(SyscallId::epoll_create1):
        case static_cast<uint64_t>(SyscallId::epoll_ctl):
        case static_cast<uint64_t>(SyscallId::epoll_wait):
            r.handled = true;
            r.route = "expfc.ready";
            r.value = -38;
            return r;

        case static_cast<uint64_t>(SyscallId::exit):
            r.handled = true;
            r.route = "exabi.proc_exit";
            r.value = 0;
            return r;

        default:
            r.handled = false;
            r.route = "unmapped";
            r.value = -38;
            return r;
    }
}

} // namespace exms::compat::linux::syscall
