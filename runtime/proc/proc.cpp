#include "proc.hpp"
#include <algorithm>

namespace exms::runtime::proc {

Process* ProcManager::create_process(std::uint64_t entry_point, std::uint64_t stack_ptr) {
    auto proc = std::make_unique<Process>();
    proc->entry_point = entry_point;
    proc->stack_ptr = stack_ptr;
    proc->state = PROC_STATE_CREATED;
    proc->ppid = state_.current ? state_.current->pid : 0;

    if (state_.current) {
        state_.current->children.push_back(proc.get());
        proc->parent = state_.current;
    }

    return state_.add_process(std::move(proc));
}

std::unique_ptr<Process> ProcManager::copy_process(Process* src) {
    if (!src) return nullptr;

    auto proc = std::make_unique<Process>();
    proc->entry_point = src->entry_point;
    proc->stack_ptr = src->stack_ptr;
    proc->brk = src->brk;
    proc->state = PROC_STATE_CREATED;
    proc->ppid = src->pid;
    proc->parent = src;
    src->children.push_back(proc.get());
    return proc;
}

std::uint32_t ProcManager::fork(std::uint32_t) {
    if (!state_.current) return static_cast<std::uint32_t>(-1);

    auto new_proc = copy_process(state_.current);
    if (!new_proc) return static_cast<std::uint32_t>(-1);

    Process* proc_ptr = state_.add_process(std::move(new_proc));
    return proc_ptr->pid;
}

std::uint32_t ProcManager::clone(std::uint32_t flags, std::uint64_t stack_ptr) {
    if (!state_.current) return static_cast<std::uint32_t>(-1);

    auto new_proc = copy_process(state_.current);
    if (!new_proc) return static_cast<std::uint32_t>(-1);

    if (stack_ptr != 0 && (flags & CLONE_VM)) {
        new_proc->stack_ptr = stack_ptr;
    }

    Process* proc_ptr = state_.add_process(std::move(new_proc));
    return proc_ptr->pid;
}

void ProcManager::exit(int status) {
    if (!state_.current) return;

    state_.current->exit_status = status;
    state_.current->exited = true;
    state_.current->state = PROC_STATE_ZOMBIE;
}

int ProcManager::wait(int* status, std::uint32_t options) {
    if (!state_.current) return -1;

    for (auto* child : state_.current->children) {
        if (child->state == PROC_STATE_ZOMBIE) {
            if (status) *status = child->exit_status;
            std::uint32_t pid = child->pid;

            auto it = std::find(state_.current->children.begin(),
                               state_.current->children.end(), child);
            if (it != state_.current->children.end()) {
                state_.current->children.erase(it);
            }

            state_.remove_process(pid);
            return pid;
        }
    }

    if (options & WNOHANG) return 0;
    return -1;
}

} // namespace exms::runtime::proc
