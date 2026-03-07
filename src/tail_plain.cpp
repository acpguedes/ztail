#include "tail_plain.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
#if defined(__linux__)
#include <fcntl.h>
#endif

void tailPlainFile(const std::string& filename, Parser& parser, size_t n, size_t bufferSize) {
    FILE* f = std::fopen(filename.c_str(), "rb");
    if (!f) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

#if defined(__linux__)
    {
        int fd = fileno(f);
        if (fd >= 0) {
            posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
        }
    }
#endif

    std::fseek(f, 0, SEEK_END);
    long fileSize = std::ftell(f);
    if (fileSize <= 0) {
        std::fclose(f);
        parser.finalize();
        return;
    }

    // Phase 1: Scan backward to find the start offset containing at least n+1 newlines
    // (n+1 because the last line may not end with \n)
    std::vector<char> chunk(bufferSize);
    size_t lines = 0;
    long remaining = fileSize;
    long startOffset = fileSize;

    while (remaining > 0 && lines <= n) {
        size_t toRead = static_cast<size_t>(std::min<long>(static_cast<long>(bufferSize), remaining));
        remaining -= static_cast<long>(toRead);
        std::fseek(f, remaining, SEEK_SET);
        size_t got = std::fread(chunk.data(), 1, toRead, f);

        // Count newlines using memchr (faster than std::count via SIMD in libc)
        const char* p = chunk.data();
        size_t left = got;
        while (left > 0) {
            const char* nl = static_cast<const char*>(std::memchr(p, '\n', left));
            if (!nl) break;
            ++lines;
            size_t advance = static_cast<size_t>(nl - p) + 1;
            p += advance;
            left -= advance;
        }
        startOffset = remaining;
    }

    // Phase 2: Read forward from startOffset and parse sequentially
    std::fseek(f, startOffset, SEEK_SET);
    long pos = startOffset;
    while (pos < fileSize) {
        size_t toRead = static_cast<size_t>(std::min<long>(static_cast<long>(bufferSize), fileSize - pos));
        size_t got = std::fread(chunk.data(), 1, toRead, f);
        if (got == 0) break;
        parser.parse(chunk.data(), got);
        pos += static_cast<long>(got);
    }

    std::fclose(f);
    parser.finalize();
}
