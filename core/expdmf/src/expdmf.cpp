#include "expdmf.hpp"

namespace exms::expdmf {

std::uint64_t Runtime::create_process() {
    return next_process_++;
}

std::uint64_t Runtime::create_page(const std::string& kind) {
    Page p{};
    p.id = next_page_++;
    p.kind = kind;
    pages_.push_back(p);
    return p.id;
}

void Runtime::map_page(std::uint64_t subject, std::uint64_t page, MapMode mode) {
    mappings_.push_back(Mapping{subject, page, mode});
}

std::uint64_t Runtime::fork_process(std::uint64_t parent) {
    const auto child = create_process();
    for (auto& m : mappings_) {
        if (m.subject == parent) {
            auto mode = m.mode;
            if (mode == MapMode::Private) {
                m.mode = MapMode::Cow;
                mode = MapMode::Cow;
            }
            mappings_.push_back(Mapping{child, m.page, mode});
        }
    }
    return child;
}

std::uint64_t Runtime::create_shared_page(const std::vector<std::uint64_t>& owners) {
    const auto page = create_page("shared");
    for (const auto owner : owners) {
        map_page(owner, page, MapMode::Shared);
    }
    return page;
}

} // namespace exms::expdmf
