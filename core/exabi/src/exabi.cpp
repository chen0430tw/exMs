#include "exabi.hpp"
#include <cstring>

namespace exms::exabi {

SyscallResult Runtime::sys_read(int fd, void* buf, std::uint64_t count) {
    SyscallResult r{};
    if (!fd_mgr_) {
        r.error = -38;  // ENOSYS
        return r;
    }

    auto file = fd_mgr_->get_file(fd);
    if (!file) {
        r.error = -9;  // EBADF
        return r;
    }

    std::int64_t n = file->read(buf, count);
    if (n < 0) {
        r.error = -22;  // EINVAL
        return r;
    }

    r.value = n;
    r.error = 0;
    return r;
}

SyscallResult Runtime::sys_write(int fd, const void* buf, std::uint64_t count) {
    SyscallResult r{};
    if (!fd_mgr_) {
        r.error = -38;
        return r;
    }

    auto file = fd_mgr_->get_file(fd);
    if (!file) {
        r.error = -9;
        return r;
    }

    std::int64_t n = file->write(buf, count);
    if (n < 0) {
        r.error = -22;
        return r;
    }

    r.value = n;
    r.error = 0;
    return r;
}

SyscallResult Runtime::sys_open(const char* path, int flags, std::uint32_t mode) {
    SyscallResult r{};
    if (!fd_mgr_) {
        r.error = -38;
        return r;
    }

    if (!path) {
        r.error = -2;  // ENOENT
        return r;
    }

    int fd = fd_mgr_->open(path, flags, mode);
    if (fd < 0) {
        r.error = -2;
        return r;
    }

    r.value = fd;
    r.error = 0;
    return r;
}

SyscallResult Runtime::sys_close(int fd) {
    SyscallResult r{};
    if (!fd_mgr_) {
        r.error = -38;
        return r;
    }

    int result = fd_mgr_->close(fd);
    if (result < 0) {
        r.error = -9;
        return r;
    }

    r.value = 0;
    r.error = 0;
    return r;
}

SyscallResult Runtime::sys_openat(int dirfd, const char* path, int flags, std::uint32_t mode) {
    // For now, ignore dirfd and treat as regular open
    (void)dirfd;
    return sys_open(path, flags, mode);
}

void Runtime::sys_exit(int status) {
    if (proc_mgr_) {
        proc_mgr_->exit(status);
    }
}

SyscallResult Runtime::sys_fork() {
    SyscallResult r{};
    if (!proc_mgr_) {
        r.error = -38;
        return r;
    }

    std::uint32_t child_pid = proc_mgr_->fork(0);
    r.value = child_pid;
    r.error = 0;
    return r;
}

SyscallResult Runtime::sys_clone(std::uint64_t flags, void* stack) {
    SyscallResult r{};
    if (!proc_mgr_) {
        r.error = -38;
        return r;
    }

    std::uint64_t stack_ptr = reinterpret_cast<std::uint64_t>(stack);
    std::uint32_t result = proc_mgr_->clone(static_cast<std::uint32_t>(flags), stack_ptr);
    r.value = result;
    r.error = 0;
    return r;
}

SyscallResult Runtime::sys_wait4(int pid, int* status, int options) {
    SyscallResult r{};
    if (!proc_mgr_) {
        r.error = -38;
        return r;
    }

    int result = proc_mgr_->wait(status, static_cast<std::uint32_t>(options));
    if (result < 0) {
        r.error = -10;  // ECHILD
        return r;
    }

    r.value = result;
    r.error = 0;
    return r;
}

} // namespace exms::exabi
