#include "char_ring_buffer.h"
#include <iostream>
#include <cstring>

template <size_t MaxBytes>
CharRingBuffer<MaxBytes>::CharRingBuffer(size_t cap, size_t lineCapacity, size_t bytesBudget)
    : data(), offsets(cap), capacity(cap), start(0), end(0), offsetStart(0),
      count(0), lineInProgress(false), currentLineStart(0)
{
    if (bytesBudget > 0) {
        data.resize(bytesBudget);
    } else if (capacity > 0 && lineCapacity > 0) {
        data.resize(capacity * lineCapacity);
    }
}

template <size_t MaxBytes>
void CharRingBuffer<MaxBytes>::ensureStorage(size_t hint) {
    if (data.empty()) {
        size_t perLine = hint > 512 ? hint : 512;
        data.resize(capacity * perLine);
    }
}

template <size_t MaxBytes>
size_t CharRingBuffer<MaxBytes>::freeSpace() const {
    const size_t dataCap = data.size();
    size_t s = static_cast<size_t>(start);
    size_t e = static_cast<size_t>(end);
    size_t used = (e >= s) ? e - s : dataCap - (s - e);
    if (used == 0 && count > 0) {
        used = dataCap;
    }
    return dataCap - used;
}

template <size_t MaxBytes>
void CharRingBuffer<MaxBytes>::dropOldest() {
    if (count == 0) return;
    Offset second_start = (count > 1) ? offsets[(offsetStart + 1) % capacity] : end;
    start = second_start;
    offsetStart = (offsetStart + 1) % capacity;
    count--;
}

template <size_t MaxBytes>
void CharRingBuffer<MaxBytes>::copyToRing(const char* src, size_t len) {
    const size_t dataCap = data.size();
    size_t e = static_cast<size_t>(end);
    if (e + len <= dataCap) {
        if (len > 0) {
            memcpy(&data[e], src, len);
        }
        end = static_cast<Offset>((e + len) % dataCap);
    } else {
        size_t first_part = dataCap - e;
        memcpy(&data[e], src, first_part);
        memcpy(&data[0], src + first_part, len - first_part);
        end = static_cast<Offset>(len - first_part);
    }
}

template <size_t MaxBytes>
void CharRingBuffer<MaxBytes>::add(std::string&& line) {
    if (capacity == 0) return;

    size_t len = line.size();
    ensureStorage(len);

    if (len > data.size()) return;

    while (count == capacity) dropOldest();
    while (freeSpace() < len && count > 0) dropOldest();
    if (freeSpace() < len) return;

    Offset lineStart = end;
    copyToRing(line.data(), len);

    offsets[(offsetStart + count) % capacity] = lineStart;
    if (count == 0) start = lineStart;
    count++;
}

template <size_t MaxBytes>
void CharRingBuffer<MaxBytes>::append_segment(const char* segment, size_t len) {
    if (capacity == 0 || len == 0) return;

    ensureStorage(len);
    if (len > data.size()) return;

    if (!lineInProgress) {
        while (count == capacity) dropOldest();
        currentLineStart = end;
        lineInProgress = true;
    }

    while (freeSpace() < len) {
        if (count == 0) break;
        dropOldest();
    }
    if (freeSpace() < len) return;

    copyToRing(segment, len);
}

template <size_t MaxBytes>
void CharRingBuffer<MaxBytes>::append_line(const char* line, size_t len) {
    if (capacity == 0) return;

    ensureStorage(len);
    if (len > data.size()) return;

    while (count == capacity) dropOldest();
    while (freeSpace() < len && count > 0) dropOldest();
    if (freeSpace() < len) return;

    Offset lineStart = end;
    copyToRing(line, len);

    offsets[(offsetStart + count) % capacity] = lineStart;
    if (count == 0) start = lineStart;
    count++;
    lineInProgress = false;
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
            dropOldest();
        }
        offsets[(offsetStart + count) % capacity] = end;
        if (count == 0) start = end;
        count++;
        return;
    }

    while (count == capacity) dropOldest();

    offsets[(offsetStart + count) % capacity] = currentLineStart;
    if (count == 0) start = currentLineStart;
    count++;
    lineInProgress = false;
}

template <size_t MaxBytes>
void CharRingBuffer<MaxBytes>::print(size_t aggregationThreshold) const {
    if (count == 0) {
        return;
    }
    const size_t dataCap = data.size();
    size_t totalLen = 0;
    for (size_t i = 0; i < count; ++i) {
        size_t idx = (offsetStart + i) % capacity;
        size_t nextIdx = (i + 1 < count) ? (offsetStart + i + 1) % capacity : idx;
        Offset startPos = offsets[idx];
        Offset endPos = (i + 1 < count) ? offsets[nextIdx] : end;
        if (startPos <= endPos) {
            totalLen += static_cast<size_t>(endPos - startPos);
        } else {
            totalLen += dataCap - static_cast<size_t>(startPos);
            totalLen += static_cast<size_t>(endPos);
        }
        totalLen += 1; // newline
    }

    if (totalLen <= aggregationThreshold) {
        std::string output;
        output.reserve(totalLen);
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
    } else {
        std::string block;
        block.reserve(aggregationThreshold);
        for (size_t i = 0; i < count; ++i) {
            size_t idx = (offsetStart + i) % capacity;
            size_t nextIdx = (i + 1 < count) ? (offsetStart + i + 1) % capacity : idx;
            Offset startPos = offsets[idx];
            Offset endPos = (i + 1 < count) ? offsets[nextIdx] : end;

            auto append_segment_to_block = [&](const char* ptr, size_t len) {
                if (block.size() + len > aggregationThreshold && !block.empty()) {
                    std::cout.write(block.data(), static_cast<std::streamsize>(block.size()));
                    block.clear();
                }
                if (len > aggregationThreshold) {
                    std::cout.write(ptr, static_cast<std::streamsize>(len));
                } else {
                    block.append(ptr, len);
                }
            };

            if (startPos <= endPos) {
                append_segment_to_block(&data[static_cast<size_t>(startPos)], static_cast<size_t>(endPos - startPos));
            } else {
                append_segment_to_block(&data[static_cast<size_t>(startPos)], dataCap - static_cast<size_t>(startPos));
                append_segment_to_block(&data[0], static_cast<size_t>(endPos));
            }

            if (block.size() + 1 > aggregationThreshold && !block.empty()) {
                std::cout.write(block.data(), static_cast<std::streamsize>(block.size()));
                block.clear();
            }
            block.push_back('\n');
        }
        if (!block.empty()) {
            std::cout.write(block.data(), static_cast<std::streamsize>(block.size()));
        }
    }
}

template <size_t MaxBytes>
size_t CharRingBuffer<MaxBytes>::memoryUsage() const {
    return sizeof(CharRingBuffer<MaxBytes>) + data.size() * sizeof(char) + offsets.size() * sizeof(Offset);
}

template class CharRingBuffer<UINT32_MAX>;
template class CharRingBuffer<static_cast<size_t>(UINT32_MAX) + 1>;

