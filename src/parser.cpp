#include "parser.h"
#include <cstring> // memchr, memcpy

Parser::Parser(CircularBuffer& cb, size_t /*lineCapacity*/)
    : circularBuffer(cb), residual(), residualLen(0)
{
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
            size_t line_length = newline_ptr - start_ptr;
            if (residualLen > 0) {
                circularBuffer.append_segment(residual, residualLen);
                residualLen = 0;
                if (line_length > 0) {
                    circularBuffer.append_segment(start_ptr, line_length);
                }
                circularBuffer.end_line();
            } else {
                circularBuffer.append_line(start_ptr, line_length);
            }
            pos += line_length + 1;
        } else {
            size_t copy_len = remaining;
            if (copy_len + residualLen > RESIDUAL_SIZE) {
                copy_len = RESIDUAL_SIZE - residualLen;
            }
            if (copy_len > 0) {
                memcpy(residual + residualLen, start_ptr, copy_len);
                residualLen += copy_len;
            }
            break;
        }
    }
}

void Parser::finalize() {
    if (residualLen > 0) {
        circularBuffer.append_segment(residual, residualLen);
        circularBuffer.end_line();
        residualLen = 0;
    }
}
