#include <iostream>
#include "elf_loader.hpp"
#include "dispatcher.hpp"
#include "expdmf.hpp"
#include "expfc.hpp"

int main() {
    using namespace exms;

    expdmf::Runtime pdmf;
    const auto p0 = pdmf.create_process();
    const auto p1 = pdmf.fork_process(p0);
    const auto shared = pdmf.create_shared_page({p0, p1});

    expfc::Scheduler pfc(4, 3, 5, 1);
    pfc.enqueue(100, expfc::QueueClass::Fast);
    pfc.enqueue(200, expfc::QueueClass::Main);
    auto out = pfc.schedule(16);

    auto sys = compat::linux::syscall::dispatch(
        static_cast<std::uint64_t>(compat::linux::syscall::SyscallId::epoll_wait),
        0,0,0,0,0,0
    );

    std::cout << "demo_echo placeholder\\n";
    std::cout << "pdmf.pages=" << pdmf.pages().size() << " mappings=" << pdmf.mappings().size() << "\\n";
    std::cout << "fork child pid=" << p1 << " shared_page=" << shared << "\\n";
    std::cout << "pfc.out=" << out.size() << " fast_emitted=" << pfc.fast_emitted() << " main_emitted=" << pfc.main_emitted() << "\\n";
    std::cout << "syscall route=" << sys.route << " handled=" << sys.handled << "\\n";
    return 0;
}
