#include "elf_loader.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <vector>

namespace exms::compat::linux::elf {

namespace {
constexpr unsigned char EI_MAG0 = 0;
constexpr unsigned char EI_MAG1 = 1;
constexpr unsigned char EI_MAG2 = 2;
constexpr unsigned char EI_MAG3 = 3;
constexpr unsigned char EI_CLASS = 4;
constexpr unsigned char EI_DATA  = 5;

constexpr unsigned char ELFCLASS64 = 2;
constexpr unsigned char ELFDATA2LSB = 1;

constexpr uint32_t PT_LOAD = 1;

template <typename T>
bool read_object(std::ifstream& in, T& out) {
    return static_cast<bool>(in.read(reinterpret_cast<char*>(&out), sizeof(T)));
}

#pragma pack(push, 1)
struct Elf64_Ehdr {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct Elf64_Phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};
#pragma pack(pop)

bool valid_magic(const unsigned char ident[16]) {
    return ident[EI_MAG0] == 0x7f &&
           ident[EI_MAG1] == 'E' &&
           ident[EI_MAG2] == 'L' &&
           ident[EI_MAG3] == 'F';
}
} // namespace

ElfLoadResult load_static_elf(const std::string& path) {
    ElfLoadResult result{};
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        result.message = "cannot open file: " + path;
        return result;
    }

    Elf64_Ehdr eh{};
    if (!read_object(in, eh)) {
        result.message = "failed to read ELF header";
        return result;
    }

    if (!valid_magic(eh.e_ident)) {
        result.message = "not an ELF file";
        return result;
    }

    result.is_64 = (eh.e_ident[EI_CLASS] == ELFCLASS64);
    result.little_endian = (eh.e_ident[EI_DATA] == ELFDATA2LSB);
    result.machine = eh.e_machine;
    result.type = eh.e_type;
    result.entry = eh.e_entry;
    result.program_header_count = eh.e_phnum;

    if (!result.is_64) {
        result.message = "only ELF64 is supported in this skeleton";
        return result;
    }
    if (!result.little_endian) {
        result.message = "only little-endian ELF is supported in this skeleton";
        return result;
    }

    if (eh.e_phoff == 0 || eh.e_phnum == 0) {
        result.ok = true;
        result.message = "ELF parsed, but has no program headers";
        return result;
    }

    in.seekg(static_cast<std::streamoff>(eh.e_phoff), std::ios::beg);
    if (!in) {
        result.message = "failed to seek to program header table";
        return result;
    }

    for (uint16_t i = 0; i < eh.e_phnum; ++i) {
        Elf64_Phdr ph{};
        if (!read_object(in, ph)) {
            result.message = "failed to read program header #" + std::to_string(i);
            return result;
        }

        if (ph.p_type == PT_LOAD) {
            ElfProgramHeaderInfo info{};
            info.type = ph.p_type;
            info.flags = ph.p_flags;
            info.offset = ph.p_offset;
            info.vaddr = ph.p_vaddr;
            info.filesz = ph.p_filesz;
            info.memsz = ph.p_memsz;
            info.align = ph.p_align;
            result.program_headers.push_back(info);
        }
    }

    result.ok = true;
    result.message = "ELF64 parsed successfully";
    return result;
}

void ElfImagePlan::calculate_layout() {
    if (segments.empty()) {
        return;
    }

    // Find minimum and maximum virtual addresses
    min_vaddr = UINT64_MAX;
    max_vaddr = 0;

    for (const auto& seg : segments) {
        if (seg.vaddr < min_vaddr) {
            min_vaddr = seg.vaddr;
        }
        uint64_t seg_end = seg.vaddr + seg.size;
        if (seg_end > max_vaddr) {
            max_vaddr = seg_end;
        }
    }

    // Align min_vaddr down to page boundary (4KB)
    constexpr uint64_t PAGE_SIZE = 0x1000;
    min_vaddr = min_vaddr & ~(PAGE_SIZE - 1);

    // Align max_vaddr up to page boundary
    max_vaddr = (max_vaddr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    // Calculate base address (we'll use the ELF's vaddr directly)
    base_addr = min_vaddr;
    total_size = max_vaddr - min_vaddr;
}

ElfImagePlan construct_image(const ElfLoadResult& parsed) {
    ElfImagePlan plan{};

    if (!parsed.ok) {
        plan.message = "Cannot construct image from failed parse: " + parsed.message;
        return plan;
    }

    if (parsed.program_headers.empty()) {
        plan.message = "No PT_LOAD segments found";
        return plan;
    }

    // Convert program headers to image segments
    for (const auto& ph : parsed.program_headers) {
        ImageSegment seg{};
        seg.vaddr = ph.vaddr;
        seg.size = ph.memsz;
        seg.file_offset = ph.offset;
        seg.file_size = ph.filesz;
        seg.prot = ph.flags;
        seg.align = ph.align;
        plan.segments.push_back(seg);
    }

    // Calculate layout
    plan.calculate_layout();

    // Set entry point
    plan.entry_point = parsed.entry;

    plan.ok = true;
    plan.message = "Image plan constructed successfully";
    return plan;
}

uint64_t load_image_to_memory(const std::string& path, const ElfImagePlan& plan) {
    if (!plan.ok) {
        return 0;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return 0;
    }

    // For now, return the base address (actual memory mapping will be done by PDMF)
    return plan.base_addr;
}

} // namespace exms::compat::linux::elf
