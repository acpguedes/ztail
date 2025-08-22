#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <string>
#include <vector>

#ifdef USE_CHAR_RING_BUFFER
#include "char_ring_buffer.h"
using CircularBuffer = CharRingBuffer<>;
#else

class CircularBuffer {
public:
    explicit CircularBuffer(size_t capacity, size_t lineCapacity = 0, size_t bytesBudget = 0);
    void add(std::string&& line);

    // Streaming API compatible with CharRingBuffer
    void append_segment(const char* segment, size_t len);
    void end_line();

    void print(size_t aggregationThreshold) const;
    size_t memoryUsage() const;

private:
    std::vector<std::string> buffer;
    size_t capacity;
    size_t next;
    size_t count;
    std::string current_line;
    size_t bytesBudget;
    size_t currentBytes;
};

#endif // USE_CHAR_RING_BUFFER

#endif // CIRCULAR_BUFFER_H
