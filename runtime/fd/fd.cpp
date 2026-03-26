#include "fd.hpp"
#include <algorithm>
#include <cstring>

namespace exms::runtime::fd {

std::int64_t File::read(void* buf, std::size_t count) {
    if (!exists || (flags & O_WRONLY)) return -1;
    std::size_t available = data.size() - offset;
    std::size_t to_read = std::min(count, available);
    if (to_read == 0) return 0;
    std::memcpy(buf, data.data() + offset, to_read);
    offset += to_read;
    return to_read;
}

std::int64_t File::write(const void* buf, std::size_t count) {
    if (!exists && (flags & O_CREAT) == 0) return -1;
    if ((flags & O_WRONLY) == 0 && (flags & O_RDWR) == 0) return -1;

    if (!exists) exists = true;
    if (flags & O_APPEND) offset = data.size();

    if (offset + count > data.size()) data.resize(offset + count);
    std::memcpy(data.data() + offset, buf, count);
    offset += count;
    return count;
}

std::uint64_t File::seek(std::int64_t offset_param, std::uint64_t whence) {
    switch (whence) {
        case EXMS_SEEK_SET:
            offset = offset_param < 0 ? 0 : static_cast<std::uint64_t>(offset_param);
            break;
        case EXMS_SEEK_CUR:
            if (offset_param < 0 && static_cast<std::uint64_t>(-offset_param) > offset) {
                offset = 0;
            } else {
                offset += offset_param;
            }
            break;
        case EXMS_SEEK_END:
            if (offset_param < 0 && static_cast<std::uint64_t>(-offset_param) > data.size()) {
                offset = 0;
            } else {
                offset = data.size() + offset_param;
            }
            break;
    }
    return offset;
}

FdManager::FdManager(FdState& state) : state_(state) {
    // Initialize stdin, stdout, stderr
    auto stdin_f = std::make_shared<File>();
    stdin_f->flags = O_RDONLY;
    stdin_f->exists = true;
    state_.fd_table[0] = stdin_f;

    auto stdout_f = std::make_shared<File>();
    stdout_f->flags = O_WRONLY;
    stdout_f->exists = true;
    state_.fd_table[1] = stdout_f;

    auto stderr_f = std::make_shared<File>();
    stderr_f->flags = O_WRONLY;
    stderr_f->exists = true;
    state_.fd_table[2] = stderr_f;
}

std::shared_ptr<File> FdManager::find_or_create_file(const std::string& path) {
    for (auto& f : state_.files) {
        if (f->path == path) return f;
    }
    auto f = std::make_shared<File>();
    f->path = path;
    state_.files.push_back(f);
    return f;
}

int FdManager::open(const std::string& path, std::uint32_t flags, std::uint32_t) {
    auto file = find_or_create_file(path);
    file->flags = flags;

    if ((flags & O_CREAT) || file->exists) {
        file->exists = true;
    }

    int fd = state_.next_fd++;
    state_.fd_table[fd] = file;
    return fd;
}

int FdManager::close(int fd) {
    auto it = state_.fd_table.find(fd);
    if (it == state_.fd_table.end()) return -1;
    state_.fd_table.erase(it);
    return 0;
}

std::int64_t FdManager::read(int fd, void* buf, std::size_t count) {
    auto file = get_file(fd);
    if (!file) return -1;
    return file->read(buf, count);
}

std::int64_t FdManager::write(int fd, const void* buf, std::size_t count) {
    auto file = get_file(fd);
    if (!file) return -1;
    return file->write(buf, count);
}

std::uint64_t FdManager::lseek(int fd, std::int64_t offset, std::uint64_t whence) {
    auto file = get_file(fd);
    if (!file) return static_cast<std::uint64_t>(-1);
    // Convert whence parameter to our constants
    if (whence == 0) return file->seek(offset, EXMS_SEEK_SET);
    if (whence == 1) return file->seek(offset, EXMS_SEEK_CUR);
    if (whence == 2) return file->seek(offset, EXMS_SEEK_END);
    return static_cast<std::uint64_t>(-1);
}

} // namespace exms::runtime::fd
