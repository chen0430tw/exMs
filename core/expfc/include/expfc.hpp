#pragma once
#include <cstdint>
#include <deque>
#include <unordered_set>
#include <vector>

namespace exms::expfc {

enum class QueueClass {
    Fast,
    Main
};

struct ReadyItem {
    std::uint64_t event_id = 0;
    QueueClass qclass = QueueClass::Main;
    std::uint64_t enqueue_tick = 0;
};

class Scheduler {
public:
    Scheduler(std::uint64_t rescue_interval = 4,
              std::uint64_t rescue_quota = 3,
              std::uint64_t main_ttl = 5,
              std::uint64_t cooldown = 1);

    void enqueue(std::uint64_t event_id, QueueClass qclass);
    std::vector<std::uint64_t> schedule(std::size_t max_events = 512);

    std::uint64_t tick() const noexcept { return tick_; }
    std::uint64_t fast_emitted() const noexcept { return fast_emitted_; }
    std::uint64_t main_emitted() const noexcept { return main_emitted_; }
    std::uint64_t main_expired() const noexcept { return main_expired_; }

private:
    void expire_main();

    std::uint64_t rescue_interval_;
    std::uint64_t rescue_quota_;
    std::uint64_t main_ttl_;
    std::uint64_t cooldown_;

    std::uint64_t tick_ = 0;
    std::uint64_t fast_streak_ = 0;
    std::uint64_t fast_emitted_ = 0;
    std::uint64_t main_emitted_ = 0;
    std::uint64_t main_expired_ = 0;

    std::deque<ReadyItem> fast_q_;
    std::deque<ReadyItem> main_q_;
    std::unordered_set<std::uint64_t> queued_;
    std::unordered_map<std::uint64_t, std::uint64_t> cooldown_until_;
};

} // namespace exms::expfc
