#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Include dependencies
#include "compat/linux/elf/elf_loader.hpp"
#include "runtime/mem/mem.hpp"
#include "runtime/proc/proc.hpp"

namespace exms::exloader {

// Load result
struct LoadResult {
    bool ok = false;
    std::uint64_t process_id = 0;
    std::uint64_t entry_point = 0;
    std::string message;
};

// ELF loader - loads ELF binaries into memory and prepares for execution
class Loader {
public:
    Loader() = default;

    // Load ELF executable
    LoadResult load_executable(const std::string& path);

    // Load with pre-parsed plan
    LoadResult load_from_plan(const std::string& path, const compat::linux::elf::ElfImagePlan& plan);

    // Set runtime managers
    void set_mem_manager(runtime::mem::MemManager* mgr) { mem_mgr_ = mgr; }
    void set_proc_manager(runtime::proc::ProcManager* mgr) { proc_mgr_ = mgr; }

private:
    runtime::mem::MemManager* mem_mgr_ = nullptr;
    runtime::proc::ProcManager* proc_mgr_ = nullptr;

    // Map ELF segments into memory
    bool map_segments(const compat::linux::elf::ElfImagePlan& plan, const std::string& path);
};

} // namespace exms::exloader
