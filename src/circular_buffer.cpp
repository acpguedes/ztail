#ifndef USE_CHAR_RING_BUFFER
#include "circular_buffer.h"
#include <iostream>

CircularBuffer::CircularBuffer(size_t cap, size_t lineCapacity, size_t bytesBudget)
    : buffer(cap), capacity(cap), next(0), count(0), current_line(),
      bytesBudget(bytesBudget), currentBytes(0)
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

    size_t len = line.size();
    if (bytesBudget > 0) {
        if (len > bytesBudget) {
            return; // line too large to fit
        }
        while (count > 0 && currentBytes + len > bytesBudget) {
            size_t oldest = (next + capacity - count) % capacity;
            currentBytes -= buffer[oldest].size();
            buffer[oldest].clear();
            count--;
        }
    }

    if (count == capacity) {
        size_t oldest = next;
        currentBytes -= buffer[oldest].size();
        buffer[oldest].clear();
        count--;
    }

    currentBytes += len;
    buffer[next] = std::move(line);
    next = (next + 1) % capacity;
    count++;
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

void CircularBuffer::append_line(const char* line, size_t len) {
    std::string tmp(line, len);
    add(std::move(tmp));
}

void CircularBuffer::print(size_t aggregationThreshold) const {
    if (count == 0) {
        return;
    }
    size_t start = (next >= count) ? (next - count) : (capacity + next - count);
    size_t totalLen = 0;
    for (size_t i = 0; i < count; ++i) {
        size_t idx = (start + i) % capacity;
        totalLen += buffer[idx].size() + 1; // include newline
    }
    if (totalLen <= aggregationThreshold) {
        std::string output;
        output.reserve(totalLen);
        for (size_t i = 0; i < count; ++i) {
            size_t idx = (start + i) % capacity;
            output.append(buffer[idx]);
            output.push_back('\n');
        }
        std::cout.write(output.data(), static_cast<std::streamsize>(output.size()));
    } else {
        std::string block;
        block.reserve(aggregationThreshold);
        for (size_t i = 0; i < count; ++i) {
            size_t idx = (start + i) % capacity;
            const std::string& line = buffer[idx];
            if (block.size() + line.size() + 1 > aggregationThreshold && !block.empty()) {
                std::cout.write(block.data(), static_cast<std::streamsize>(block.size()));
                block.clear();
            }
            if (line.size() + 1 > aggregationThreshold) {
                std::cout.write(line.data(), static_cast<std::streamsize>(line.size()));
                std::cout.put('\n');
            } else {
                block.append(line);
                block.push_back('\n');
            }
        }
        if (!block.empty()) {
            std::cout.write(block.data(), static_cast<std::streamsize>(block.size()));
        }
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
