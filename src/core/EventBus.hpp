#pragma once
// A tiny thread-safe publish/subscribe bus. Detectors publish Contributions,
// the ScoreEngine publishes HighlightEvents, the UI/Clip pipeline subscribe.
//
// Handlers are invoked on the publisher's thread, so subscribers must be cheap
// or marshal to their own thread (the Qt dock marshals onto the UI thread).
#include "core/Types.hpp"
#include <functional>
#include <mutex>
#include <vector>
#include <cstdint>

namespace hypeclip {

class EventBus {
public:
    static EventBus& instance();

    using ContribHandler  = std::function<void(const Contribution&)>;
    using HighlightHandler= std::function<void(const HighlightEvent&)>;
    using ClipHandler     = std::function<void(const ClipRecord&)>;

    uint64_t onContribution(ContribHandler h);
    uint64_t onHighlight(HighlightHandler h);
    uint64_t onClipSaved(ClipHandler h);
    void unsubscribe(uint64_t token);

    void publish(const Contribution& c);
    void publish(const HighlightEvent& e);
    void publishClip(const ClipRecord& r);

private:
    EventBus() = default;
    std::mutex mtx_;
    uint64_t next_ = 1;
    std::vector<std::pair<uint64_t, ContribHandler>> contrib_;
    std::vector<std::pair<uint64_t, HighlightHandler>> highlight_;
    std::vector<std::pair<uint64_t, ClipHandler>> clip_;
};

} // namespace hypeclip
