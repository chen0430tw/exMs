#include "runtime.hpp"
#include "pdmf/pdmf.hpp"
#include "pfc/pfc.hpp"

namespace exms::runtime {

bool Runtime::init(bool debug) {
    if (state_.initialized) {
        return true;
    }

    // Configure logging
    state_.log_config.min_level = debug ? log::LogLevel::DEBUG : log::LogLevel::INFO;
    state_.log_config.target = debug ? log::LogTarget::BOTH : log::LogTarget::CONSOLE;
    state_.log_config.log_file = "exms_debug.log";
    state_.log_config.color_output = true;
    state_.log_config.timestamp = true;
    state_.log_config.source_location = debug;

    log::Logger::instance().set_config(state_.log_config);

    LOG_INFO("exMs Runtime initializing...");
    if (debug) {
        LOG_DEBUG("Debug mode enabled");
    }

    state_.debug_mode = debug;

    // Initialize subsystems
    try {
        // Create managers
        state_.mem_mgr = std::make_unique<mem::MemManager>(state_.mem);
        state_.proc_mgr = std::make_unique<proc::ProcManager>(state_.proc);
        state_.fd_mgr = std::make_unique<fd::FdManager>(state_.fd);
        state_.shm_mgr = std::make_unique<shm::ShmManager>(state_.shm);
        state_.event_mgr = std::make_unique<event::EventManager>(state_.event);

        // Initialize PDMF
        pdmf::init(state_.mem, state_.proc, state_.shm);
        DLOG_DEBUG("PDMF initialized");

        // Initialize PFC
        pfc::init(state_.event);
        DLOG_DEBUG("PFC initialized");

        state_.initialized = true;
        LOG_INFO("exMs Runtime initialized successfully");
        return true;

    } catch (const std::exception& e) {
        LOG_FATAL(std::string("Runtime initialization failed: ") + e.what());
        return false;
    }
}

void Runtime::shutdown() {
    if (!state_.initialized) {
        return;
    }

    LOG_INFO("exMs Runtime shutting down...");

    // Shutdown in reverse order
    state_.event_mgr.reset();
    state_.shm_mgr.reset();
    state_.fd_mgr.reset();
    state_.proc_mgr.reset();
    state_.mem_mgr.reset();

    state_.initialized = false;
    LOG_INFO("exMs Runtime shutdown complete");
}

} // namespace exms::runtime
