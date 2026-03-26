#pragma once
#include "mem/mem.hpp"
#include "proc/proc.hpp"
#include "fd/fd.hpp"
#include "log/log.hpp"
#include "shm/shm.hpp"
#include "event/event.hpp"
#include <memory>

namespace exms::runtime {

// Global runtime state
struct RuntimeState {
    // Subsystem states
    mem::MemState mem;
    proc::ProcState proc;
    fd::FdState fd;
    shm::ShmState shm;
    event::EventState event;

    // Managers (created on initialization)
    std::unique_ptr<mem::MemManager> mem_mgr;
    std::unique_ptr<proc::ProcManager> proc_mgr;
    std::unique_ptr<fd::FdManager> fd_mgr;
    std::unique_ptr<shm::ShmManager> shm_mgr;
    std::unique_ptr<event::EventManager> event_mgr;

    // Runtime flags
    bool initialized = false;
    bool debug_mode = false;

    // Configuration
    log::LogConfig log_config;
};

// Global runtime accessor
class Runtime {
public:
    static Runtime& instance() {
        static Runtime inst;
        return inst;
    }

    // Initialize runtime
    bool init(bool debug = false);

    // Shutdown runtime
    void shutdown();

    // Get runtime state
    RuntimeState& state() { return state_; }

    // Check if initialized
    bool is_initialized() const { return state_.initialized; }

    // Debug mode
    bool is_debug_mode() const { return state_.debug_mode; }
    void set_debug_mode(bool enable) { state_.debug_mode = enable; }

    // Convenience accessors
    mem::MemManager* mem() { return state_.mem_mgr.get(); }
    proc::ProcManager* proc() { return state_.proc_mgr.get(); }
    fd::FdManager* fd() { return state_.fd_mgr.get(); }
    shm::ShmManager* shm() { return state_.shm_mgr.get(); }
    event::EventManager* event() { return state_.event_mgr.get(); }

    // Module loggers
    log::ModuleLogger& log_loader() { return log_loader_; }
    log::ModuleLogger& log_elf() { return log_elf_; }
    log::ModuleLogger& log_syscall() { return log_syscall_; }
    log::ModuleLogger&_log_abi() { return log_abi_; }
    log::ModuleLogger& log_pdmf() { return log_pdmf_; }
    log::ModuleLogger& log_pfc() { return log_pfc_; }

private:
    Runtime() = default;
    ~Runtime() { shutdown(); }

    RuntimeState state_;

    // Module loggers
    log::ModuleLogger log_loader_{"loader"};
    log::ModuleLogger log_elf_{"elf"};
    log::ModuleLogger log_syscall_{"syscall"};
    log::ModuleLogger log_abi_{"abi"};
    log::ModuleLogger log_pdmf_{"pdmf"};
    log::ModuleLogger log_pfc_{"pfc"};
};

// RAII runtime guard
class RuntimeGuard {
public:
    explicit RuntimeGuard(bool debug = false) {
        if (!Runtime::instance().is_initialized()) {
            Runtime::instance().init(debug);
        }
    }

    ~RuntimeGuard() {
        // Don't shutdown - let runtime persist
    }
};

} // namespace exms::runtime
