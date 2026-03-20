#include "expfc.hpp"
#include <algorithm>
#include <unordered_map>

namespace exms::expfc {

Scheduler::Scheduler(std::uint64_t rescue_interval,
                     std::uint64_t rescue_quota,
                     std::uint64_t main_ttl,
                     std::uint64_t cooldown)
    : rescue_interval_(rescue_interval),
      rescue_quota_(rescue_quota),
      main_ttl_(main_ttl),
      cooldown_(cooldown) {}

void Scheduler::enqueue(std::uint64_t event_id, QueueClass qclass) {
    if (queued_.count(event_id)) return;

    if (qclass == QueueClass::Main) {
        auto it = cooldown_until_.find(event_id);
        if (it != cooldown_until_.end() && it->second > tick_) {
            return;
        }
        main_q_.push_back(ReadyItem{event_id, qclass, tick_});
    } else {
        fast_q_.push_back(ReadyItem{event_id, qclass, tick_});
    }
    queued_.insert(event_id);
}

void Scheduler::expire_main() {
    std::deque<ReadyItem> kept;
    while (!main_q_.empty()) {
        const auto item = main_q_.front();
        main_q_.pop_front();
        if (!queued_.count(item.event_id)) continue;

        const auto age = tick_ - item.enqueue_tick;
        if (age > main_ttl_) {
            queued_.erase(item.event_id);
            cooldown_until_[item.event_id] = tick_ + cooldown_;
            ++main_expired_;
        } else {
            kept.push_back(item);
        }
    }
    main_q_ = std::move(kept);
}

std::vector<std::uint64_t> Scheduler::schedule(std::size_t max_events) {
    expire_main();

    std::vector<std::uint64_t> out;
    out.reserve(max_events);

    if (!fast_q_.empty()) {
        while (!fast_q_.empty() && out.size() < max_events) {
            const auto item = fast_q_.front();
            fast_q_.pop_front();
            queued_.erase(item.event_id);
            out.push_back(item.event_id);
            ++fast_emitted_;
        }
        ++fast_streak_;

        if (fast_streak_ >= rescue_interval_ && !main_q_.empty()) {
            std::size_t rescued = 0;
            while (!main_q_.empty() && rescued < rescue_quota_ && out.size() < max_events) {
                const auto item = main_q_.front();
                main_q_.pop_front();
                queued_.erase(item.event_id);
                out.push_back(item.event_id);
                ++main_emitted_;
                ++rescued;
            }
            if (rescued > 0) fast_streak_ = 0;
        }
    } else {
        fast_streak_ = 0;
        while (!main_q_.empty() && out.size() < max_events) {
            const auto item = main_q_.front();
            main_q_.pop_front();
            queued_.erase(item.event_id);
            out.push_back(item.event_id);
            ++main_emitted_;
        }
    }

    ++tick_;
    return out;
}

} // namespace exms::expfc
