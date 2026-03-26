#pragma once
#include <cstdint>
#include <vector>
#include <memory>

namespace exms::runtime::mem {

// Protection flags
constexpr std::uint32_t PROT_NONE  = 0x0;
constexpr std::uint32_t PROT_READ  = 0x1;
constexpr std::uint32_t PROT_WRITE = 0x2;
constexpr std::uint32_t PROT_EXEC  = 0x4;

// Mapping flags
constexpr std::uint32_t MAP_PRIVATE    = 0x01;
constexpr std::uint32_t MAP_SHARED     = 0x02;
constexpr std::uint32_t MAP_ANONYMOUS  = 0x04;
constexpr std::uint32_t MAP_FIXED      = 0x08;

// Page size
constexpr std::size_t PAGE_SIZE = 0x1000;

// Memory region
struct MemoryRegion {
    std::uint64_t addr = 0;
    std::uint64_t size = 0;
    std::uint32_t prot = 0;
    std::uint32_t flags = 0;
    std::vector<std::uint8_t> data;

    bool contains(std::uint64_t a) const {
        return a >= addr && a < addr + size;
    }

    std::uint64_t offset(std::uint64_t a) const {
        return a - addr;
    }
};

// Memory manager state
struct MemState {
    std::vector<MemoryRegion> regions;
    std::uint64_t brk = 0;
    std::uint64_t brk_start = 0;

    MemoryRegion* find_region(std::uint64_t addr) {
        for (auto& reg : regions) {
            if (reg.contains(addr)) return &reg;
        }
        return nullptr;
    }
};

// Memory manager
class MemManager {
public:
    explicit MemManager(MemState& state) : state_(state) {}

    std::uint64_t mmap(std::uint64_t addr, std::size_t length, std::uint32_t prot,
                       std::uint32_t flags, int fd, std::uint64_t offset);
    int munmap(std::uint64_t addr, std::size_t length);
    std::uint64_t brk(std::uint64_t new_brk);
    int mprotect(std::uint64_t addr, std::size_t length, std::uint32_t prot);

    bool read(std::uint64_t addr, void* buf, std::size_t size);
    bool write(std::uint64_t addr, const void* buf, std::size_t size);

    void init_brk(std::uint64_t start) {
        state_.brk = start;
        state_.brk_start = start;
    }

    const MemState& state() const noexcept { return state_; }

private:
    MemState& state_;
    std::uint64_t find_free_region(std::size_t size, std::uint64_t hint);
};

} // namespace exms::runtime::mem
