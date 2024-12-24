#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <string>
#include <vector>

class CircularBuffer {
public:
    explicit CircularBuffer(size_t capacity);
    void add(std::string&& line);
    void print() const;

private:
    std::vector<std::string> buffer;
    size_t capacity;
    size_t next;
    size_t count;
};

#endif // CIRCULAR_BUFFER_H
