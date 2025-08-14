#ifndef USE_CHAR_RING_BUFFER
#include "circular_buffer.h"
#include <iostream>

CircularBuffer::CircularBuffer(size_t cap, size_t lineCapacity)
    : buffer(cap), capacity(cap), next(0), count(0)
{
    // Reserve initial capacity for each string when requested
    if (lineCapacity > 0) {
        for (auto& s : buffer) {
            s.reserve(lineCapacity);
        }
    }
}

void CircularBuffer::add(std::string&& line) {
    if (capacity == 0) {
        return;
    }
    // Move the line into the circular buffer
    buffer[next].assign(std::move(line));
    next = (next + 1) % capacity;

    if (count < capacity) {
        count++;
    }
}

void CircularBuffer::print() const {
    size_t start = (next >= count) ? (next - count) : (capacity + next - count);
    for (size_t i = 0; i < count; ++i) {
        size_t idx = (start + i) % capacity;
        std::cout << buffer[idx] << "\n";
    }
}

size_t CircularBuffer::memoryUsage() const {
    size_t total = sizeof(CircularBuffer) + buffer.capacity() * sizeof(std::string);
    for (const auto& s : buffer) {
        total += s.capacity();
    }
    return total;
}

#endif // USE_CHAR_RING_BUFFER
