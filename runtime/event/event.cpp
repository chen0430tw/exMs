#include "event.hpp"

namespace exms::runtime::event {

constexpr int EPOLL_CTL_ADD = 1;
constexpr int EPOLL_CTL_DEL = 2;
constexpr int EPOLL_CTL_MOD = 3;

std::uint32_t EventManager::epoll_create(int size) {
    (void)size;
    auto set = std::make_unique<EpollSet>();
    return state_.add_epoll(std::move(set))->id;
}

int EventManager::epoll_ctl(std::uint32_t epfd, int op, int fd, std::uint32_t events) {
    EpollSet* set = state_.find_epoll(epfd);
    if (!set) return -1;

    switch (op) {
        case EPOLL_CTL_ADD: {
            EventWatch w{};
            w.fd = fd;
            w.events = events;
            set->watches.push_back(w);
            break;
        }
        case EPOLL_CTL_DEL: {
            auto it = std::remove_if(set->watches.begin(), set->watches.end(),
                                    [fd](const EventWatch& w) { return w.fd == fd; });
            set->watches.erase(it);
            break;
        }
        case EPOLL_CTL_MOD: {
            for (auto& w : set->watches) {
                if (w.fd == fd) {
                    w.events = events;
                    break;
                }
            }
            break;
        }
        default:
            return -1;
    }
    return 0;
}

int EventManager::epoll_wait(std::uint32_t epfd, EventEntry* events,
                              int max_events, int) {
    EpollSet* set = state_.find_epoll(epfd);
    if (!set) return -1;

    int count = 0;
    for (const auto& w : set->watches) {
        if (count >= max_events) break;
        if (w.events != 0) {
            events[count].fd = w.fd;
            events[count].events = w.events;
            count++;
        }
    }
    return count;
}

} // namespace exms::runtime::event
