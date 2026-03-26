#pragma once
#include <cstdint>
#include <vector>
#include <memory>

namespace exms::runtime::proc {

// Process states
constexpr std::uint32_t PROC_STATE_CREATED = 0x01;
constexpr std::uint32_t PROC_STATE_RUNNING = 0x02;
constexpr std::uint32_t PROC_STATE_ZOMBIE  = 0x04;

// Clone flags
constexpr std::uint32_t CLONE_VM      = 0x00000100;
constexpr std::uint32_t CLONE_FS      = 0x00000200;
constexpr std::uint32_t CLONE_FILES   = 0x00000400;
constexpr std::uint32_t CLONE_THREAD  = 0x00010000;

// Wait options
constexpr std::uint32_t WNOHANG = 0x00000001;

// Process descriptor
struct Process {
    std::uint32_t pid = 0;
    std::uint32_t ppid = 0;
    std::uint32_t state = PROC_STATE_CREATED;
    std::uint64_t entry_point = 0;
    std::uint64_t stack_ptr = 0;
    std::uint64_t brk = 0;
    int exit_status = 0;
    bool exited = false;
    Process* parent = nullptr;
    std::vector<Process*> children;
};

// Process manager state
struct ProcState {
    std::vector<std::unique_ptr<Process>> processes;
    std::uint32_t next_pid = 1;
    Process* current = nullptr;

    Process* find_process(std::uint32_t pid) {
        for (auto& p : processes) {
            if (p->pid == pid) return p.get();
        }
        return nullptr;
    }

    Process* add_process(std::unique_ptr<Process> proc) {
        proc->pid = next_pid++;
        processes.push_back(std::move(proc));
        return processes.back().get();
    }

    bool remove_process(std::uint32_t pid) {
        for (auto it = processes.begin(); it != processes.end(); ++it) {
            if ((*it)->pid == pid) {
                processes.erase(it);
                return true;
            }
        }
        return false;
    }
};

// Process manager
class ProcManager {
public:
    explicit ProcManager(ProcState& state) : state_(state) {}

    Process* create_process(std::uint64_t entry_point, std::uint64_t stack_ptr);
    std::uint32_t fork(std::uint32_t flags);
    std::uint32_t clone(std::uint32_t flags, std::uint64_t stack_ptr);
    void exit(int status);
    int wait(int* status, std::uint32_t options);

    Process* current() { return state_.current; }
    void set_current(Process* proc) { state_.current = proc; }
    Process* get_process(std::uint32_t pid) { return state_.find_process(pid); }

    const ProcState& state() const noexcept { return state_; }

private:
    ProcState& state_;

    std::unique_ptr<Process> copy_process(Process* src);
};

} // namespace exms::runtime::proc
