#include "circular_buffer.h"
#include <iostream>

CircularBuffer::CircularBuffer(size_t cap)
    : buffer(cap), capacity(cap), next(0), count(0)
{
    // Reserve initial capacity for each string to avoid repeated allocations
    for (auto& s : buffer) {
        s.reserve(1024);
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
