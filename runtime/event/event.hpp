#pragma once
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>
#include <algorithm>

namespace exms::runtime::event {

constexpr std::uint32_t EVT_READ  = 0x001;
constexpr std::uint32_t EVT_WRITE = 0x002;

struct EventWatch {
    int fd = -1;
    std::uint32_t events = 0;
    std::function<void(std::uint32_t)> callback;
};

struct EpollSet {
    std::uint32_t id = 0;
    std::vector<EventWatch> watches;
};

struct EventEntry {
    int fd = -1;
    std::uint32_t events = 0;
};

struct EventState {
    std::vector<std::unique_ptr<EpollSet>> epoll_sets;
    std::uint32_t next_epoll_id = 1;

    EpollSet* find_epoll(std::uint32_t id) {
        for (auto& set : epoll_sets) {
            if (set->id == id) return set.get();
        }
        return nullptr;
    }

    EpollSet* add_epoll(std::unique_ptr<EpollSet> set) {
        set->id = next_epoll_id++;
        epoll_sets.push_back(std::move(set));
        return epoll_sets.back().get();
    }
};

class EventManager {
public:
    explicit EventManager(EventState& state) : state_(state) {}

    std::uint32_t epoll_create(int size);
    int epoll_ctl(std::uint32_t epfd, int op, int fd, std::uint32_t events);
    int epoll_wait(std::uint32_t epfd, EventEntry* events, int max_events, int timeout);

private:
    EventState& state_;
};

} // namespace exms::runtime::event
