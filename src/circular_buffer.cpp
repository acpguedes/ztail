#ifndef USE_CHAR_RING_BUFFER
#include "circular_buffer.h"
#include <iostream>

CircularBuffer::CircularBuffer(size_t cap, size_t lineCapacity)
    : buffer(cap), capacity(cap), next(0), count(0), current_line()
{
    // Reserve initial capacity for each string when requested
    if (lineCapacity > 0) {
        for (auto& s : buffer) {
            s.reserve(lineCapacity);
        }
        current_line.reserve(lineCapacity);
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

void CircularBuffer::append_segment(const char* segment, size_t len) {
    if (len == 0) {
        return;
    }
    current_line.append(segment, len);
}

void CircularBuffer::end_line() {
    add(std::move(current_line));
    current_line.clear();
}

void CircularBuffer::print() const {
    if (count == 0) {
        return;
    }
    size_t start = (next >= count) ? (next - count) : (capacity + next - count);
    size_t totalLen = 0;
    for (size_t i = 0; i < count; ++i) {
        size_t idx = (start + i) % capacity;
        totalLen += buffer[idx].size() + 1; // include newline
    }
    std::string output;
    output.reserve(totalLen);
    for (size_t i = 0; i < count; ++i) {
        size_t idx = (start + i) % capacity;
        output.append(buffer[idx]);
        output.push_back('\n');
    }
    std::cout.write(output.data(), static_cast<std::streamsize>(output.size()));
}

size_t CircularBuffer::memoryUsage() const {
    size_t total = sizeof(CircularBuffer) + buffer.capacity() * sizeof(std::string);
    for (const auto& s : buffer) {
        total += s.capacity();
    }
    return total;
}

#endif // USE_CHAR_RING_BUFFER
