#ifndef CHAR_RING_BUFFER_H
#define CHAR_RING_BUFFER_H

#include <vector>
#include <string>
#include <cstddef>

class CharRingBuffer {
public:
    explicit CharRingBuffer(size_t capacity, size_t lineCapacity = 0);

    // Existing line based API
    void add(std::string&& line);

    // Append raw bytes of the current line.  Bytes are copied directly into
    // the underlying ring buffer.  If this is the first segment of a new line
    // it implicitly starts that line.
    void append_segment(const char* segment, size_t len);

    // Mark the end of the current line.  The line becomes visible to print()
    // and counts toward the ring capacity.
    void end_line();

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
    bool lineInProgress;                // whether a line is being built
    size_t currentLineStart;            // start offset of current line
};

#endif // CHAR_RING_BUFFER_H
