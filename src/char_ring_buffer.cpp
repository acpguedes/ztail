#include "char_ring_buffer.h"
#include <iostream>
#include <algorithm>

CharRingBuffer::CharRingBuffer(size_t cap, size_t lineCapacity)
    : data(), offsets(cap), capacity(cap), start(0), end(0), offsetStart(0),
      count(0), lineInProgress(false), currentLineStart(0)
{
    if (capacity > 0 && lineCapacity > 0) {
        data.resize(capacity * lineCapacity);
    }
}

void CharRingBuffer::add(std::string&& line) {
    append_segment(line.data(), line.size());
    end_line();
}

void CharRingBuffer::append_segment(const char* segment, size_t len) {
    if (capacity == 0 || data.empty() || len == 0) {
        return;
    }

    const size_t dataCap = data.size();
    if (len > dataCap) {
        return; // segment too large to fit
    }

    auto freeSpace = [&]() {
        size_t used = (end >= start) ? end - start : dataCap - (start - end);
        return dataCap - used;
    };

    auto drop_oldest = [&]() {
        if (count == 0) return;
        size_t second_start = (count > 1) ? offsets[(offsetStart + 1) % capacity] : end;
        start = second_start;
        offsetStart = (offsetStart + 1) % capacity;
        count--;
    };

    if (!lineInProgress) {
        while (count == capacity) {
            drop_oldest();
        }
        currentLineStart = end;
        lineInProgress = true;
    }

    while (freeSpace() < len) {
        if (count == 0) break;
        drop_oldest();
    }

    if (freeSpace() < len) {
        return; // not enough space even after dropping
    }

    if (end + len <= dataCap) {
        std::copy(segment, segment + len, data.begin() + end);
        end = (end + len) % dataCap;
    } else {
        size_t first_part = dataCap - end;
        std::copy(segment, segment + first_part, data.begin() + end);
        std::copy(segment + first_part, segment + len, data.begin());
        end = len - first_part;
    }
}

void CharRingBuffer::end_line() {
    if (capacity == 0 || data.empty()) {
        lineInProgress = false;
        return;
    }

    if (!lineInProgress) {
        while (count == capacity) {
            if (count == 0) break;
            size_t second_start = (count > 1) ? offsets[(offsetStart + 1) % capacity] : end;
            start = second_start;
            offsetStart = (offsetStart + 1) % capacity;
            count--;
        }
        offsets[(offsetStart + count) % capacity] = end;
        if (count == 0) {
            start = end;
        }
        count++;
        return;
    }

    while (count == capacity) {
        size_t second_start = (count > 1) ? offsets[(offsetStart + 1) % capacity] : end;
        start = second_start;
        offsetStart = (offsetStart + 1) % capacity;
        count--;
    }

    offsets[(offsetStart + count) % capacity] = currentLineStart;
    if (count == 0) {
        start = currentLineStart;
    }
    count++;
    lineInProgress = false;
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

