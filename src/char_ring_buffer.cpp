#include "char_ring_buffer.h"
#include <iostream>

CharRingBuffer::CharRingBuffer(size_t cap, size_t lineCapacity)
    : data(), offsets(), capacity(cap), count(0)
{
    if (capacity > 0 && lineCapacity > 0) {
        data.reserve(capacity * lineCapacity);
    }
    offsets.reserve(capacity);
}

void CharRingBuffer::add(std::string&& line) {
    if (capacity == 0) {
        return;
    }
    // if we have space for more lines
    if (count < capacity) {
        offsets.push_back(data.size());
        data.insert(data.end(), line.begin(), line.end());
        count++;
    } else {
        // remove first line
        size_t first_start = offsets[0];
        size_t first_end = (count > 1) ? offsets[1] : data.size();
        size_t first_len = first_end - first_start;
        // erase first line from data
        data.erase(data.begin(), data.begin() + first_len);
        // shift offsets left
        for (size_t i = 1; i < offsets.size(); ++i) {
            offsets[i - 1] = offsets[i] - first_len;
        }
        offsets[count - 1] = data.size();
        data.insert(data.end(), line.begin(), line.end());
    }
}

void CharRingBuffer::print() const {
    for (size_t i = 0; i < count; ++i) {
        size_t start = offsets[i];
        size_t end = (i + 1 < count) ? offsets[i + 1] : data.size();
        std::cout.write(&data[start], end - start);
        std::cout.put('\n');
    }
}

size_t CharRingBuffer::memoryUsage() const {
    return sizeof(CharRingBuffer) + data.capacity() * sizeof(char) + offsets.capacity() * sizeof(size_t);
}

