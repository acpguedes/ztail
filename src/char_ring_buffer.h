#ifndef CHAR_RING_BUFFER_H
#define CHAR_RING_BUFFER_H

#include <vector>
#include <string>
#include <cstddef>

class CharRingBuffer {
public:
    explicit CharRingBuffer(size_t capacity, size_t lineCapacity = 0);
    void add(std::string&& line);
    void print() const;
    size_t memoryUsage() const;

private:
    std::vector<char> data;             // underlying byte storage
    std::vector<size_t> offsets;        // ring of line start positions
    size_t capacity;                    // maximum number of lines
    size_t start;                       // index of first byte in data
    size_t end;                         // index one past the last byte
    size_t offsetStart;                 // index of first entry in offsets
    size_t count;                       // current number of lines
};

#endif // CHAR_RING_BUFFER_H
