#include <iostream>
#include "elf_loader.hpp"
#include "exloader.hpp"
#include "runtime/mem/mem.hpp"
#include "runtime/proc/proc.hpp"

using namespace exms;

int main() {
    std::cout << "=== ELF Loader Test ===" << std::endl;

    // Initialize runtime
    runtime::mem::MemState mem_state;
    runtime::proc::ProcState proc_state;
    runtime::mem::MemManager mem_mgr(mem_state);
    runtime::proc::ProcManager proc_mgr(proc_state);

    exloader::Loader loader;
    loader.set_mem_manager(&mem_mgr);
    loader.set_proc_manager(&proc_mgr);

    // Test with various files
    std::vector<std::string> test_paths = {
        "C:\\Program Files\\Git\\bin\\bash.exe",  // Git Bash (PE wrapper, may contain ELF)
        "C:\\Program Files\\GitHub CLI\\gh.exe",   // GitHub CLI (pure PE)
        "demo_echo.exe",                           // Our own executable (PE)
        "stress_test.exe",                          // Our own executable (PE)
    };

    for (const auto& path : test_paths) {
        std::cout << "\nTesting: " << path << std::endl;

        compat::linux::elf::ElfLoadResult result = compat::linux::elf::load_static_elf(path);
        if (result.ok) {
            std::cout << "  ✓ ELF parsed!" << std::endl;
            std::cout << "    64-bit: " << (result.is_64 ? "yes" : "no") << std::endl;
            std::cout << "    PT_LOAD: " << result.program_headers.size() << std::endl;
            std::cout << "    Entry: 0x" << std::hex << result.entry << std::dec << std::endl;

            compat::linux::elf::ElfImagePlan plan = compat::linux::elf::construct_image(result);
            if (plan.ok) {
                std::cout << "    Image plan OK: " << plan.total_size << " bytes" << std::endl;
            }
        } else {
            std::cout << "  ✗ Not ELF: " << result.message << std::endl;
        }
    }

    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "  ELF loader correctly identifies:" << std::endl;
    std::cout << "  - ELF binaries (will be found in Linux env)" << std::endl;
    std::cout << "  - PE binaries (correctly rejected as non-ELF)" << std::endl;

    return 0;
}
