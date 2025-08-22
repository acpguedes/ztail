#ifndef CHAR_RING_BUFFER_H
#define CHAR_RING_BUFFER_H

#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>
#include <type_traits>

template <size_t MaxBytes = UINT32_MAX>
class CharRingBuffer {
public:
    using Offset = std::conditional_t<MaxBytes<=UINT32_MAX,uint32_t,uint64_t>;

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

    void print(size_t aggregationThreshold) const;
    size_t memoryUsage() const;

private:
    std::vector<char> data;             // underlying byte storage
    std::vector<Offset> offsets;        // ring of line start positions
    size_t capacity;                    // maximum number of lines
    Offset start;                       // index of first byte in data
    Offset end;                         // index one past the last byte
    size_t offsetStart;                 // index of first entry in offsets
    size_t count;                       // current number of lines
    bool lineInProgress;                // whether a line is being built
    Offset currentLineStart;            // start offset of current line
};

#endif // CHAR_RING_BUFFER_H
