#include "exloader.hpp"
#include <fstream>

namespace exms::exloader {

LoadResult Loader::load_executable(const std::string& path) {
    LoadResult r{};

    // Parse ELF
    compat::linux::elf::ElfLoadResult parsed = compat::linux::elf::load_static_elf(path);
    if (!parsed.ok) {
        r.message = "Failed to parse ELF: " + parsed.message;
        return r;
    }

    // Construct image plan
    compat::linux::elf::ElfImagePlan plan = compat::linux::elf::construct_image(parsed);
    if (!plan.ok) {
        r.message = "Failed to construct image: " + plan.message;
        return r;
    }

    return load_from_plan(path, plan);
}

LoadResult Loader::load_from_plan(const std::string& path, const compat::linux::elf::ElfImagePlan& plan) {
    LoadResult r{};

    if (!plan.ok) {
        r.message = "Invalid plan";
        return r;
    }

    // Map segments into memory
    if (!map_segments(plan, path)) {
        r.message = "Failed to map segments";
        return r;
    }

    // Create process
    if (!proc_mgr_) {
        r.message = "No proc manager";
        return r;
    }

    auto* proc = proc_mgr_->create_process(plan.entry_point, 0);
    if (!proc) {
        r.message = "Failed to create process";
        return r;
    }

    // Initialize brk after max_vaddr
    if (mem_mgr_) {
        mem_mgr_->init_brk(plan.max_vaddr);
        proc->brk = plan.max_vaddr;
    }

    r.ok = true;
    r.process_id = proc->pid;
    r.entry_point = plan.entry_point;
    r.message = "Loaded successfully";
    return r;
}

bool Loader::map_segments(const compat::linux::elf::ElfImagePlan& plan, const std::string& path) {
    if (!mem_mgr_) {
        return false;
    }

    // Open file for reading segment data
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }

    // Map each segment
    for (const auto& seg : plan.segments) {
        if (seg.size == 0) continue;

        // Determine protection flags
        std::uint32_t prot = 0;
        if (seg.prot & 0x1) prot |= runtime::mem::PROT_READ;    // PF_X
        if (seg.prot & 0x2) prot |= runtime::mem::PROT_WRITE;   // PF_W
        if (seg.prot & 0x4) prot |= runtime::mem::PROT_EXEC;    // PF_R

        // Map segment
        std::uint64_t addr = mem_mgr_->mmap(seg.vaddr, seg.size, prot,
                                            runtime::mem::MAP_PRIVATE | runtime::mem::MAP_ANONYMOUS,
                                            -1, 0);
        if (addr == 0 && seg.vaddr != 0) {
            return false;
        }

        // Read file data into segment
        if (seg.file_size > 0) {
            in.seekg(seg.file_offset);
            if (!in) {
                return false;
            }

            std::vector<std::uint8_t> buf(seg.file_size);
            in.read(reinterpret_cast<char*>(buf.data()), seg.file_size);

            // Write to memory
            if (!mem_mgr_->write(seg.vaddr, buf.data(), seg.file_size)) {
                return false;
            }
        }

        // Zero-fill BSS (memsz > filesz)
        if (seg.size > seg.file_size) {
            std::vector<std::uint8_t> zero(seg.size - seg.file_size, 0);
            if (!mem_mgr_->write(seg.vaddr + seg.file_size, zero.data(), zero.size())) {
                return false;
            }
        }
    }

    return true;
}

} // namespace exms::exloader
