#include "mem.hpp"
#include <algorithm>
#include <cstring>

namespace exms::runtime::mem {

constexpr std::uint64_t BASE_ALLOC_ADDR = 0x10000000ULL;
constexpr std::uint64_t MAX_ALLOC_ADDR = 0x80000000ULL;

std::uint64_t MemManager::find_free_region(std::size_t size, std::uint64_t hint) {
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (hint != 0) {
        hint = (hint + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        bool hint_free = true;
        for (const auto& reg : state_.regions) {
            if (hint < reg.addr + reg.size && hint + size > reg.addr) {
                hint_free = false;
                break;
            }
        }
        if (hint_free && hint + size <= MAX_ALLOC_ADDR) return hint;
    }

    std::uint64_t addr = BASE_ALLOC_ADDR;
    while (addr < MAX_ALLOC_ADDR) {
        bool collision = false;
        for (const auto& reg : state_.regions) {
            if (addr < reg.addr + reg.size && addr + size > reg.addr) {
                collision = true;
                addr = reg.addr + reg.size;
                addr = (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
                break;
            }
        }
        if (!collision) return addr;
    }
    return 0;
}

std::uint64_t MemManager::mmap(std::uint64_t addr, std::size_t length, std::uint32_t prot,
                               std::uint32_t flags, int fd, std::uint64_t) {
    if (length == 0) return 0;

    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if ((flags & MAP_FIXED) == 0) {
        addr = find_free_region(length, addr);
        if (addr == 0) return 0;
    } else {
        addr = (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    }

    MemoryRegion reg{};
    reg.addr = addr;
    reg.size = length;
    reg.prot = prot;
    reg.flags = flags;
    reg.data.resize(length, 0);

    state_.regions.push_back(reg);
    return addr;
}

int MemManager::munmap(std::uint64_t addr, std::size_t length) {
    if (addr == 0 || length == 0) return -1;

    addr = (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    for (auto it = state_.regions.begin(); it != state_.regions.end(); ) {
        if (it->addr <= addr && it->addr + it->size > addr) {
            if (it->addr == addr && it->size == length) {
                it = state_.regions.erase(it);
                return 0;
            }
            // Partial unmap not fully implemented
            return 0;
        } else {
            ++it;
        }
    }
    return -1;
}

std::uint64_t MemManager::brk(std::uint64_t new_brk) {
    if (new_brk == 0) return state_.brk;
    if (new_brk < state_.brk_start) return state_.brk;
    state_.brk = new_brk;
    return state_.brk;
}

int MemManager::mprotect(std::uint64_t addr, std::size_t length, std::uint32_t prot) {
    if (addr == 0 || length == 0) return -1;

    for (auto& reg : state_.regions) {
        if (reg.addr <= addr && reg.addr + reg.size >= addr + length) {
            reg.prot = prot;
            return 0;
        }
    }
    return -1;
}

bool MemManager::read(std::uint64_t addr, void* buf, std::size_t size) {
    if (buf == nullptr || size == 0) return false;

    auto* reg = state_.find_region(addr);
    if (!reg) return false;

    std::uint64_t off = reg->offset(addr);
    if (off + size > reg->data.size()) return false;
    if ((reg->prot & PROT_READ) == 0) return false;

    std::memcpy(buf, reg->data.data() + off, size);
    return true;
}

bool MemManager::write(std::uint64_t addr, const void* buf, std::size_t size) {
    if (buf == nullptr || size == 0) return false;

    auto* reg = state_.find_region(addr);
    if (!reg) return false;

    std::uint64_t off = reg->offset(addr);
    if (off + size > reg->data.size()) return false;
    if ((reg->prot & PROT_WRITE) == 0) return false;

    std::memcpy(reg->data.data() + off, buf, size);
    return true;
}

} // namespace exms::runtime::mem
