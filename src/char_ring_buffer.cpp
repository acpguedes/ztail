#include "char_ring_buffer.h"
#include <iostream>
#include <algorithm>

template <size_t MaxBytes>
CharRingBuffer<MaxBytes>::CharRingBuffer(size_t cap, size_t lineCapacity)
    : data(), offsets(cap), capacity(cap), start(0), end(0), offsetStart(0),
      count(0), lineInProgress(false), currentLineStart(0)
{
    if (capacity > 0 && lineCapacity > 0) {
        data.resize(capacity * lineCapacity);
    }
}

template <size_t MaxBytes>
void CharRingBuffer<MaxBytes>::add(std::string&& line) {
    append_segment(line.data(), line.size());
    end_line();
}

template <size_t MaxBytes>
void CharRingBuffer<MaxBytes>::append_segment(const char* segment, size_t len) {
    if (capacity == 0 || data.empty() || len == 0) {
        return;
    }

    const size_t dataCap = data.size();
    if (len > dataCap) {
        return; // segment too large to fit
    }

    auto freeSpace = [&]() {
        size_t s = static_cast<size_t>(start);
        size_t e = static_cast<size_t>(end);
        size_t used = (e >= s) ? e - s : dataCap - (s - e);
        return dataCap - used;
    };

    auto drop_oldest = [&]() {
        if (count == 0) return;
        Offset second_start = (count > 1) ? offsets[(offsetStart + 1) % capacity] : end;
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

    size_t e = static_cast<size_t>(end);
    if (e + len <= dataCap) {
        std::copy(segment, segment + len, data.begin() + e);
        end = static_cast<Offset>((e + len) % dataCap);
    } else {
        size_t first_part = dataCap - e;
        std::copy(segment, segment + first_part, data.begin() + e);
        std::copy(segment + first_part, segment + len, data.begin());
        end = static_cast<Offset>(len - first_part);
    }
}

template <size_t MaxBytes>
void CharRingBuffer<MaxBytes>::end_line() {
    if (capacity == 0 || data.empty()) {
        lineInProgress = false;
        return;
    }

    if (!lineInProgress) {
        while (count == capacity) {
            if (count == 0) break;
            Offset second_start = (count > 1) ? offsets[(offsetStart + 1) % capacity] : end;
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
        Offset second_start = (count > 1) ? offsets[(offsetStart + 1) % capacity] : end;
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

template <size_t MaxBytes>
void CharRingBuffer<MaxBytes>::print() const {
    if (count == 0) {
        return;
    }
    const size_t dataCap = data.size();
    std::string output;
    output.reserve(dataCap + count);

    for (size_t i = 0; i < count; ++i) {
        size_t idx = (offsetStart + i) % capacity;
        size_t nextIdx = (i + 1 < count) ? (offsetStart + i + 1) % capacity : idx;
        Offset startPos = offsets[idx];
        Offset endPos = (i + 1 < count) ? offsets[nextIdx] : end;

        if (startPos <= endPos) {
            output.append(&data[static_cast<size_t>(startPos)], static_cast<size_t>(endPos - startPos));
        } else {
            output.append(&data[static_cast<size_t>(startPos)], dataCap - static_cast<size_t>(startPos));
            output.append(&data[0], static_cast<size_t>(endPos));
        }
        output.push_back('\n');
    }
    std::cout.write(output.data(), static_cast<std::streamsize>(output.size()));
}

template <size_t MaxBytes>
size_t CharRingBuffer<MaxBytes>::memoryUsage() const {
    return sizeof(CharRingBuffer<MaxBytes>) + data.size() * sizeof(char) + offsets.size() * sizeof(Offset);
}

template class CharRingBuffer<UINT32_MAX>;
template class CharRingBuffer<static_cast<size_t>(UINT32_MAX) + 1>;

