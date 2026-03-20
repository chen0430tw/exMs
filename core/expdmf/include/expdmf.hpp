#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace exms::expdmf {

enum class MapMode {
    Private,
    Shared,
    Cow,
    Observe
};

struct Page {
    std::uint64_t id = 0;
    std::string kind;
};

struct Mapping {
    std::uint64_t subject = 0;
    std::uint64_t page = 0;
    MapMode mode = MapMode::Private;
};

class Runtime {
public:
    std::uint64_t create_process();
    std::uint64_t create_page(const std::string& kind);
    void map_page(std::uint64_t subject, std::uint64_t page, MapMode mode);
    std::uint64_t fork_process(std::uint64_t parent);
    std::uint64_t create_shared_page(const std::vector<std::uint64_t>& owners);

    const std::vector<Page>& pages() const noexcept { return pages_; }
    const std::vector<Mapping>& mappings() const noexcept { return mappings_; }

private:
    std::uint64_t next_process_ = 1000;
    std::uint64_t next_page_ = 1;
    std::vector<Page> pages_;
    std::vector<Mapping> mappings_;
};

} // namespace exms::expdmf
