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
    std::vector<char> data;
    std::vector<size_t> offsets; // start positions of lines within data
    size_t capacity;
    size_t count;
};

#endif // CHAR_RING_BUFFER_H
