#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace exms::compat::linux::elf {

struct ElfProgramHeaderInfo {
    uint32_t type = 0;
    uint32_t flags = 0;
    uint64_t offset = 0;
    uint64_t vaddr = 0;
    uint64_t filesz = 0;
    uint64_t memsz = 0;
    uint64_t align = 0;
};

// Memory segment for image construction
struct ImageSegment {
    uint64_t vaddr = 0;           // Virtual address
    uint64_t size = 0;            // Size in memory
    uint64_t file_offset = 0;     // Offset in file
    uint64_t file_size = 0;       // Size to load from file
    uint32_t prot = 0;            // Protection flags (R/W/X)
    uint64_t align = 0;           // Alignment requirement
};

// Complete image plan for ELF loading
struct ElfImagePlan {
    bool ok = false;
    std::string message;

    uint64_t base_addr = 0;       // Base load address
    uint64_t entry_point = 0;     // Entry point (absolute)
    uint64_t min_vaddr = 0;       // Minimum virtual address
    uint64_t max_vaddr = 0;       // Maximum virtual address
    uint64_t total_size = 0;      // Total image size

    std::vector<ImageSegment> segments;  // Loadable segments

    // Calculate load address and layout
    void calculate_layout();
};

struct ElfLoadResult {
    bool ok = false;
    std::string message;
    bool is_64 = false;
    bool little_endian = true;
    uint16_t machine = 0;
    uint16_t type = 0;
    uint64_t entry = 0;
    uint16_t program_header_count = 0;
    std::vector<ElfProgramHeaderInfo> program_headers;
};

// Parse ELF file and extract headers
ElfLoadResult load_static_elf(const std::string& path);

// Construct image plan from parsed ELF
ElfImagePlan construct_image(const ElfLoadResult& parsed);

// Load ELF into memory (returns actual load address)
uint64_t load_image_to_memory(const std::string& path, const ElfImagePlan& plan);

} // namespace exms::compat::linux::elf
