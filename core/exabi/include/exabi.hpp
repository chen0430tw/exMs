#pragma once
#include <cstdint>
#include <string>

// Include runtime manager headers
#include "runtime/mem/mem.hpp"
#include "runtime/proc/proc.hpp"
#include "runtime/fd/fd.hpp"

namespace exms::exabi {

// Syscall handler result
struct SyscallResult {
    long value = -1;
    int error = 0;  // 0 = success, negative = errno
};

// ABI runtime - bridges syscalls to runtime managers
class Runtime {
public:
    Runtime() = default;

    // File operations (route: exabi.fs)
    SyscallResult sys_read(int fd, void* buf, std::uint64_t count);
    SyscallResult sys_write(int fd, const void* buf, std::uint64_t count);
    SyscallResult sys_open(const char* path, int flags, std::uint32_t mode);
    SyscallResult sys_close(int fd);
    SyscallResult sys_openat(int dirfd, const char* path, int flags, std::uint32_t mode);

    // Process exit (route: exabi.proc_exit)
    void sys_exit(int status);

    // Process operations (route: expdmf.proc - delegated here)
    SyscallResult sys_fork();
    SyscallResult sys_clone(std::uint64_t flags, void* stack);
    SyscallResult sys_wait4(int pid, int* status, int options);

    // Set runtime managers
    void set_mem_manager(runtime::mem::MemManager* mgr) { mem_mgr_ = mgr; }
    void set_proc_manager(runtime::proc::ProcManager* mgr) { proc_mgr_ = mgr; }
    void set_fd_manager(runtime::fd::FdManager* mgr) { fd_mgr_ = mgr; }

private:
    runtime::mem::MemManager* mem_mgr_ = nullptr;
    runtime::proc::ProcManager* proc_mgr_ = nullptr;
    runtime::fd::FdManager* fd_mgr_ = nullptr;
};

} // namespace exms::exabi
