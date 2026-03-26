#include "shm.hpp"

namespace exms::runtime::shm {

std::uint32_t ShmManager::create(std::uint64_t size) {
    auto seg = std::make_unique<ShmSegment>();
    seg->size = size;
    seg->data.resize(size, 0);
    return state_.add(std::move(seg))->id;
}

std::uint64_t ShmManager::attach(std::uint32_t id) {
    ShmSegment* seg = state_.find(id);
    if (!seg) return 0;
    seg->ref_count++;
    return reinterpret_cast<std::uint64_t>(seg->data.data());
}

int ShmManager::detach(std::uint32_t id) {
    ShmSegment* seg = state_.find(id);
    if (!seg) return -1;
    seg->ref_count--;
    if (seg->ref_count <= 0) {
        // Remove would need iterator, simplified here
    }
    return 0;
}

int ShmManager::remove(std::uint32_t id) {
    // Simplified - would need proper implementation
    (void)id;
    return 0;
}

} // namespace exms::runtime::shm
