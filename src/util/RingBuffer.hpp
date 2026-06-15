#pragma once
// Single-producer / single-consumer lock-free ring buffer.
//
// The audio render thread (producer) must never block, so we hand samples to
// the analysis thread (consumer) through this. Power-of-two capacity lets us
// mask instead of modulo.
#include <atomic>
#include <vector>
#include <cstddef>
#include <cstring>

namespace hypeclip {

template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacityPow2 = 1u << 16)
        : capacity_(capacityPow2), mask_(capacityPow2 - 1), data_(capacityPow2) {
        // capacityPow2 must be a power of two; caller responsibility.
    }

    // Producer side. Returns number of items actually written (drops overflow).
    size_t push(const T* src, size_t n) {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t tail = tail_.load(std::memory_order_acquire);
        const size_t free = capacity_ - (head - tail);
        const size_t toWrite = n < free ? n : free;
        for (size_t i = 0; i < toWrite; ++i)
            data_[(head + i) & mask_] = src[i];
        head_.store(head + toWrite, std::memory_order_release);
        return toWrite;
    }

    // Consumer side. Returns number of items read.
    size_t pop(T* dst, size_t n) {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        const size_t head = head_.load(std::memory_order_acquire);
        const size_t avail = head - tail;
        const size_t toRead = n < avail ? n : avail;
        for (size_t i = 0; i < toRead; ++i)
            dst[i] = data_[(tail + i) & mask_];
        tail_.store(tail + toRead, std::memory_order_release);
        return toRead;
    }

    size_t size() const {
        return head_.load(std::memory_order_acquire) - tail_.load(std::memory_order_acquire);
    }

private:
    const size_t capacity_;
    const size_t mask_;
    std::vector<T> data_;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
};

} // namespace hypeclip
