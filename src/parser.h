#ifndef PARSER_H
#define PARSER_H

#include <string>
#include "circular_buffer.h"

// The Parser is responsible for splitting incoming data into lines
// (separated by '\n') and adding them to the CircularBuffer.
class Parser {
public:
    explicit Parser(CircularBuffer& cb);

    // Processes a chunk of data, splitting by '\n'
    void parse(const char* data, size_t size);

    // After reading is complete, finalize any leftover partial data
    void finalize();

private:
    CircularBuffer& circularBuffer;
    std::string partial;
};

#endif // PARSER_H
