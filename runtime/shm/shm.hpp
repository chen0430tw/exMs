#pragma once
#include <cstdint>
#include <vector>
#include <memory>

namespace exms::runtime::shm {

struct ShmSegment {
    std::uint32_t id = 0;
    std::uint64_t addr = 0;
    std::uint64_t size = 0;
    std::vector<std::uint8_t> data;
    int ref_count = 0;
};

struct ShmState {
    std::vector<std::unique_ptr<ShmSegment>> segments;
    std::uint32_t next_id = 1;

    ShmSegment* find(std::uint32_t id) {
        for (auto& seg : segments) {
            if (seg->id == id) return seg.get();
        }
        return nullptr;
    }

    ShmSegment* add(std::unique_ptr<ShmSegment> seg) {
        seg->id = next_id++;
        segments.push_back(std::move(seg));
        return segments.back().get();
    }
};

class ShmManager {
public:
    explicit ShmManager(ShmState& state) : state_(state) {}

    std::uint32_t create(std::uint64_t size);
    std::uint64_t attach(std::uint32_t id);
    int detach(std::uint32_t id);
    int remove(std::uint32_t id);

    ShmSegment* get_segment(std::uint32_t id) { return state_.find(id); }

private:
    ShmState& state_;
};

} // namespace exms::runtime::shm
