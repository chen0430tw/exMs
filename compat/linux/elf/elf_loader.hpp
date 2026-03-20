#pragma once
#include <cstdint>
#include <string>
#include <vector>

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

ElfLoadResult load_static_elf(const std::string& path);

} // namespace exms::compat::linux::elf
