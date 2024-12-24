#include "parser.h"
#include <cstring> // memchr

Parser::Parser(CircularBuffer& cb)
    : circularBuffer(cb)
{
    partial.reserve(1024);
}

void Parser::parse(const char* data, size_t size) {
    size_t pos = 0;
    while (pos < size) {
        const char* start_ptr = data + pos;
        size_t remaining = size - pos;

        // Look for '\n'
        const char* newline_ptr = static_cast<const char*>(
            memchr(start_ptr, '\n', remaining)
        );

        if (newline_ptr) {
            // We found a newline
            size_t line_length = newline_ptr - start_ptr;
            partial.append(start_ptr, line_length);

            // Move the completed line into the CircularBuffer
            circularBuffer.add(std::move(partial));
            partial.clear();

            // Skip the newline character
            pos += line_length + 1;
        } else {
            // No newline found in the remaining chunk
            // Append everything to the partial and exit the loop
            partial.append(start_ptr, remaining);
            break;
        }
    }
}

void Parser::finalize() {
    // If there's leftover data, treat it as a final line
    if (!partial.empty()) {
        circularBuffer.add(std::move(partial));
        partial.clear();
    }
}
