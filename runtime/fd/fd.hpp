#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace exms::runtime::fd {

// File access modes
constexpr std::uint32_t O_RDONLY = 0x00000000;
constexpr std::uint32_t O_WRONLY = 0x00000001;
constexpr std::uint32_t O_RDWR   = 0x00000002;
constexpr std::uint32_t O_CREAT  = 0x00000040;
constexpr std::uint32_t O_TRUNC  = 0x00000100;
constexpr std::uint32_t O_APPEND = 0x00000400;

// Seek constants (use different names to avoid Windows macro conflicts)
constexpr int EXMS_SEEK_SET = 0;
constexpr int EXMS_SEEK_CUR = 1;
constexpr int EXMS_SEEK_END = 2;

// File object
struct File {
    std::uint32_t flags = 0;
    std::uint64_t offset = 0;
    std::string path;
    std::vector<std::uint8_t> data;
    bool exists = false;

    std::int64_t read(void* buf, std::size_t count);
    std::int64_t write(const void* buf, std::size_t count);
    std::uint64_t seek(std::int64_t offset, std::uint64_t whence);
};

// File descriptor manager state
struct FdState {
    std::vector<std::shared_ptr<File>> files;
    std::unordered_map<int, std::shared_ptr<File>> fd_table;
    int next_fd = 3;

    std::shared_ptr<File> get_file(int fd) {
        auto it = fd_table.find(fd);
        return it != fd_table.end() ? it->second : nullptr;
    }
};

// File descriptor manager
class FdManager {
public:
    explicit FdManager(FdState& state);

    int open(const std::string& path, std::uint32_t flags, std::uint32_t mode);
    int close(int fd);
    std::int64_t read(int fd, void* buf, std::size_t count);
    std::int64_t write(int fd, const void* buf, std::size_t count);
    std::uint64_t lseek(int fd, std::int64_t offset, std::uint64_t whence);

    std::shared_ptr<File> get_file(int fd) { return state_.get_file(fd); }

private:
    FdState& state_;

    std::shared_ptr<File> find_or_create_file(const std::string& path);
};

} // namespace exms::runtime::fd
