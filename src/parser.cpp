#include "parser.h"
#include <cstring> // memchr

Parser::Parser(CircularBuffer& cb, size_t lineCapacity)
    : circularBuffer(cb), residual()
{
    if (lineCapacity > 0) {
        residual.reserve(lineCapacity);
    }
}

void Parser::parse(const char* data, size_t size) {
    size_t pos = 0;
    while (pos < size) {
        const char* start_ptr = data + pos;
        size_t remaining = size - pos;

        const char* newline_ptr = static_cast<const char*>(
            memchr(start_ptr, '\n', remaining)
        );

        if (newline_ptr) {
            size_t line_length = static_cast<size_t>(newline_ptr - start_ptr);
            if (!residual.empty()) {
                circularBuffer.append_segment(residual.data(), residual.size());
                residual.clear();
                if (line_length > 0) {
                    circularBuffer.append_segment(start_ptr, line_length);
                }
                circularBuffer.end_line();
            } else {
                circularBuffer.append_line(start_ptr, line_length);
            }
            pos += line_length + 1;
        } else {
            residual.insert(residual.end(), start_ptr, start_ptr + remaining);
            break;
        }
    }
}

void Parser::finalize() {
    if (!residual.empty()) {
        circularBuffer.append_segment(residual.data(), residual.size());
        circularBuffer.end_line();
        residual.clear();
    }
}
