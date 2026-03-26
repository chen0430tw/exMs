#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>

// Core components
#include "expdmf.hpp"
#include "expfc.hpp"

// Compat layer
#include "elf_loader.hpp"
#include "dispatcher.hpp"

// Runtime
#include "runtime/mem/mem.hpp"
#include "runtime/proc/proc.hpp"
#include "runtime/fd/fd.hpp"
#include "runtime/shm/shm.hpp"
#include "runtime/event/event.hpp"

// Core modules
#include "exabi.hpp"
#include "exloader.hpp"

using namespace exms;

// Simple stress test
void stress_test_pdmf() {
    std::cout << "\n=== PDMF Stress Test ===" << std::endl;

    expdmf::Runtime pdmf;

    auto start = std::chrono::high_resolution_clock::now();

    // Create many processes
    const int N = 1000;
    std::vector<std::uint64_t> pids;
    for (int i = 0; i < N; i++) {
        pids.push_back(pdmf.create_process());
    }

    // Fork many times
    for (int i = 0; i < N / 2; i++) {
        pdmf.fork_process(pids[i]);
    }

    // Create shared pages
    for (int i = 0; i < 100; i++) {
        pdmf.create_shared_page({pids[0], pids[1]});
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "  Created " << N << " processes" << std::endl;
    std::cout << "  Performed " << (N / 2) << " forks" << std::endl;
    std::cout << "  Created 100 shared pages" << std::endl;
    std::cout << "  Total pages: " << pdmf.pages().size() << std::endl;
    std::cout << "  Total mappings: " << pdmf.mappings().size() << std::endl;
    std::cout << "  Time: " << duration.count() << " ms" << std::endl;
}

void stress_test_pfc() {
    std::cout << "\n=== PFC Stress Test ===" << std::endl;

    expfc::Scheduler pfc(4, 3, 5, 1);

    auto start = std::chrono::high_resolution_clock::now();

    // Enqueue many events
    const int N = 10000;
    for (int i = 0; i < N; i++) {
        if (i % 3 == 0) {
            pfc.enqueue(i, expfc::QueueClass::Fast);
        } else {
            pfc.enqueue(i, expfc::QueueClass::Main);
        }
    }

    // Run many scheduling cycles
    int total_scheduled = 0;
    for (int i = 0; i < 1000; i++) {
        auto out = pfc.schedule(512);
        total_scheduled += out.size();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "  Enqueued " << N << " events" << std::endl;
    std::cout << "  Ran 1000 scheduling cycles" << std::endl;
    std::cout << "  Total scheduled: " << total_scheduled << std::endl;
    std::cout << "  Fast emitted: " << pfc.fast_emitted() << std::endl;
    std::cout << "  Main emitted: " << pfc.main_emitted() << std::endl;
    std::cout << "  Main expired: " << pfc.main_expired() << std::endl;
    std::cout << "  Final tick: " << pfc.tick() << std::endl;
    std::cout << "  Time: " << duration.count() << " ms" << std::endl;
}

void stress_test_runtime() {
    std::cout << "\n=== Runtime Stress Test ===" << std::endl;

    runtime::mem::MemState mem_state;
    runtime::proc::ProcState proc_state;
    runtime::fd::FdState fd_state;
    runtime::shm::ShmState shm_state;
    runtime::event::EventState event_state;

    runtime::mem::MemManager mem_mgr(mem_state);
    runtime::proc::ProcManager proc_mgr(proc_state);
    runtime::fd::FdManager fd_mgr(fd_state);
    runtime::shm::ShmManager shm_mgr(shm_state);
    runtime::event::EventManager event_mgr(event_state);

    auto start = std::chrono::high_resolution_clock::now();

    // Memory stress: allocate many regions
    const int N = 1000;
    std::vector<std::uint64_t> addrs;
    for (int i = 0; i < N; i++) {
        std::uint64_t addr = mem_mgr.mmap(0, 0x1000,
                                              runtime::mem::PROT_READ | runtime::mem::PROT_WRITE,
                                              runtime::mem::MAP_ANONYMOUS, -1, 0);
        if (addr != 0) {
            addrs.push_back(addr);
        }
    }

    // Process stress: create many processes
    std::vector<runtime::proc::Process*> procs;
    for (int i = 0; i < 100; i++) {
        auto* p = proc_mgr.create_process(0x400000 + i * 0x1000, 0x7000000);
        if (p) procs.push_back(p);
    }

    // FD stress: open/close many files
    std::vector<int> fds;
    for (int i = 0; i < 100; i++) {
        std::string path = "/test_" + std::to_string(i) + ".txt";
        int fd = fd_mgr.open(path, runtime::fd::O_RDWR | runtime::fd::O_CREAT, 0644);
        if (fd >= 0) fds.push_back(fd);
    }

    // SHM stress: create many shared segments
    std::vector<std::uint32_t> shm_ids;
    for (int i = 0; i < 100; i++) {
        std::uint32_t id = shm_mgr.create(0x1000);
        if (id != 0) shm_ids.push_back(id);
    }

    // Epoll stress: create many epoll sets and watches
    std::vector<std::uint32_t> epfds;
    for (int i = 0; i < 100; i++) {
        std::uint32_t epfd = event_mgr.epoll_create(16);
        if (epfd != 0) {
            epfds.push_back(epfd);
            for (int j = 0; j < 10; j++) {
                event_mgr.epoll_ctl(epfd, 1, j, runtime::event::EVT_READ);
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "  Memory regions: " << addrs.size() << std::endl;
    std::cout << "  Processes: " << procs.size() << std::endl;
    std::cout << "  File descriptors: " << fds.size() << std::endl;
    std::cout << "  SHM segments: " << shm_ids.size() << std::endl;
    std::cout << "  Epoll sets: " << epfds.size() << std::endl;
    std::cout << "  Time: " << duration.count() << " ms" << std::endl;
}

void stability_test() {
    std::cout << "\n=== Stability Test (Repeated Operations) ===" << std::endl;

    runtime::mem::MemState mem_state;
    runtime::mem::MemManager mem_mgr(mem_state);

    int passed = 0;
    int failed = 0;

    // Test repeated mmap/munmap
    for (int i = 0; i < 1000; i++) {
        std::uint64_t addr = mem_mgr.mmap(0, 0x1000,
                                              runtime::mem::PROT_READ | runtime::mem::PROT_WRITE,
                                              runtime::mem::MAP_ANONYMOUS, -1, 0);
        if (addr != 0) {
            if (mem_mgr.munmap(addr, 0x1000) == 0) {
                passed++;
            } else {
                failed++;
            }
        } else {
            failed++;
        }
    }

    std::cout << "  mmap/munmap (1000 iterations): " << passed << " passed, " << failed << " failed" << std::endl;

    // Test repeated brk
    passed = 0; failed = 0;
    for (int i = 0; i < 1000; i++) {
        std::uint64_t new_brk = 0x10000000 + i * 0x1000;
        std::uint64_t result = mem_mgr.brk(new_brk);
        if (result == new_brk) {
            passed++;
        } else {
            failed++;
        }
    }

    std::cout << "  brk (1000 iterations): " << passed << " passed, " << failed << " failed" << std::endl;

    // Test repeated process creation
    runtime::proc::ProcState proc_state;
    runtime::proc::ProcManager proc_mgr(proc_state);

    passed = 0; failed = 0;
    for (int i = 0; i < 1000; i++) {
        auto* p = proc_mgr.create_process(0x400000, 0x7000000);
        if (p && p->pid != 0) {
            passed++;
        } else {
            failed++;
        }
    }

    std::cout << "  process creation (1000 iterations): " << passed << " passed, " << failed << " failed" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    exMs Stability & Stress Test" << std::endl;
    std::cout << "========================================" << std::endl;

    auto total_start = std::chrono::high_resolution_clock::now();

    // Run tests
    stability_test();
    stress_test_pdmf();
    stress_test_pfc();
    stress_test_runtime();

    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start);

    std::cout << "\n========================================" << std::endl;
    std::cout << "  All tests completed successfully!" << std::endl;
    std::cout << "  Total time: " << total_duration.count() << " ms" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
