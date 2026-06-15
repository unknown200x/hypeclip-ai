#include "core/EventBus.hpp"
#include <algorithm>

namespace hypeclip {

EventBus& EventBus::instance() { static EventBus b; return b; }

uint64_t EventBus::onContribution(ContribHandler h) {
    std::lock_guard<std::mutex> lk(mtx_);
    uint64_t id = next_++;
    contrib_.emplace_back(id, std::move(h));
    return id;
}
uint64_t EventBus::onHighlight(HighlightHandler h) {
    std::lock_guard<std::mutex> lk(mtx_);
    uint64_t id = next_++;
    highlight_.emplace_back(id, std::move(h));
    return id;
}
uint64_t EventBus::onClipSaved(ClipHandler h) {
    std::lock_guard<std::mutex> lk(mtx_);
    uint64_t id = next_++;
    clip_.emplace_back(id, std::move(h));
    return id;
}

void EventBus::unsubscribe(uint64_t token) {
    std::lock_guard<std::mutex> lk(mtx_);
    auto rm = [token](auto& vec){
        vec.erase(std::remove_if(vec.begin(), vec.end(),
                  [token](auto& p){ return p.first == token; }), vec.end());
    };
    rm(contrib_); rm(highlight_); rm(clip_);
}

void EventBus::publish(const Contribution& c) {
    std::vector<std::pair<uint64_t, ContribHandler>> copy;
    { std::lock_guard<std::mutex> lk(mtx_); copy = contrib_; }
    for (auto& [id, h] : copy) h(c);
}
void EventBus::publish(const HighlightEvent& e) {
    std::vector<std::pair<uint64_t, HighlightHandler>> copy;
    { std::lock_guard<std::mutex> lk(mtx_); copy = highlight_; }
    for (auto& [id, h] : copy) h(e);
}
void EventBus::publishClip(const ClipRecord& r) {
    std::vector<std::pair<uint64_t, ClipHandler>> copy;
    { std::lock_guard<std::mutex> lk(mtx_); copy = clip_; }
    for (auto& [id, h] : copy) h(r);
}

} // namespace hypeclip
