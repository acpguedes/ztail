#ifndef PARSER_H
#define PARSER_H

#include "circular_buffer.h"
#include <cstddef>

// The Parser is responsible for splitting incoming data into lines
// (separated by '\n') and adding them to the CircularBuffer.
class Parser {
public:
    explicit Parser(CircularBuffer& cb, size_t lineCapacity = 0);

    // Processes a chunk of data, splitting by '\n'
    void parse(const char* data, size_t size);

    // After reading is complete, finalize any leftover partial data
    void finalize();

private:
    CircularBuffer& circularBuffer;
    static constexpr size_t RESIDUAL_SIZE = 4096;
    char residual[RESIDUAL_SIZE];
    size_t residualLen;
};

#endif // PARSER_H
