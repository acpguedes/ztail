#include "char_ring_buffer.h"
#include <iostream>
#include <algorithm>

CharRingBuffer::CharRingBuffer(size_t cap, size_t lineCapacity)
    : data(), offsets(cap), capacity(cap), start(0), end(0), offsetStart(0), count(0)
{
    if (capacity > 0 && lineCapacity > 0) {
        data.resize(capacity * lineCapacity);
    }
}

void CharRingBuffer::add(std::string&& line) {
    if (capacity == 0 || data.empty()) {
        return;
    }

    const size_t dataCap = data.size();
    const size_t lineLen = line.size();
    if (lineLen > dataCap) {
        return; // line too large to fit, ignore
    }

    auto freeSpace = [&]() {
        size_t used = (end >= start) ? end - start : dataCap - (start - end);
        return dataCap - used;
    };

    // ensure there is space and room for a new line slot
    while (count == capacity || freeSpace() < lineLen) {
        if (count == 0) break;
        size_t first_start = start;
        size_t second_start = (count > 1) ? offsets[(offsetStart + 1) % capacity] : end;
        size_t first_len = (second_start >= first_start)
                               ? (second_start - first_start)
                               : (dataCap - first_start + second_start);
        start = second_start;
        offsetStart = (offsetStart + 1) % capacity;
        count--;
    }

    size_t insert_pos = end;
    if (end + lineLen <= dataCap) {
        std::copy(line.begin(), line.end(), data.begin() + end);
        end = (end + lineLen) % dataCap;
    } else {
        size_t first_part = dataCap - end;
        std::copy(line.begin(), line.begin() + first_part, data.begin() + end);
        std::copy(line.begin() + first_part, line.end(), data.begin());
        end = lineLen - first_part;
    }

    offsets[(offsetStart + count) % capacity] = insert_pos;
    if (count == 0) {
        start = insert_pos;
    }
    count++;
}

void CharRingBuffer::print() const {
    if (count == 0) {
        return;
    }
    const size_t dataCap = data.size();
    std::string output;
    output.reserve(dataCap + count);

    for (size_t i = 0; i < count; ++i) {
        size_t idx = (offsetStart + i) % capacity;
        size_t nextIdx = (i + 1 < count) ? (offsetStart + i + 1) % capacity : idx;
        size_t startPos = offsets[idx];
        size_t endPos = (i + 1 < count) ? offsets[nextIdx] : end;

        if (startPos <= endPos) {
            output.append(&data[startPos], endPos - startPos);
        } else {
            output.append(&data[startPos], dataCap - startPos);
            output.append(&data[0], endPos);
        }
        output.push_back('\n');
    }
    std::cout.write(output.data(), static_cast<std::streamsize>(output.size()));
}

size_t CharRingBuffer::memoryUsage() const {
    return sizeof(CharRingBuffer) + data.size() * sizeof(char) + offsets.size() * sizeof(size_t);
}

