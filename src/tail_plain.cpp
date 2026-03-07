#include "tail_plain.h"
#include <fstream>
#include <algorithm>
#include <stdexcept>

void tailPlainFile(const std::string& filename, Parser& parser, size_t n, size_t bufferSize) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    file.seekg(0, std::ios::end);
    std::streamoff fileSize = file.tellg();
    if (fileSize <= 0) {
        parser.finalize();
        return;
    }

    // Phase 1: Scan backward to find the start offset containing at least n+1 newlines
    // (n+1 because the last line may not end with \n)
    std::string chunk(bufferSize, '\0');
    size_t lines = 0;
    std::streamoff remaining = fileSize;
    std::streamoff startOffset = fileSize;

    while (remaining > 0 && lines <= n) {
        size_t toRead = static_cast<size_t>(std::min<std::streamoff>(bufferSize, remaining));
        remaining -= static_cast<std::streamoff>(toRead);
        file.seekg(remaining, std::ios::beg);
        file.read(&chunk[0], static_cast<std::streamsize>(toRead));
        lines += static_cast<size_t>(std::count(chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(toRead), '\n'));
        startOffset = remaining;
    }

    // Phase 2: Read forward from startOffset and parse sequentially
    file.seekg(startOffset, std::ios::beg);
    std::streamoff pos = startOffset;
    while (pos < fileSize) {
        size_t toRead = static_cast<size_t>(std::min<std::streamoff>(bufferSize, fileSize - pos));
        file.read(&chunk[0], static_cast<std::streamsize>(toRead));
        parser.parse(chunk.data(), toRead);
        pos += static_cast<std::streamoff>(toRead);
    }

    parser.finalize();
}
