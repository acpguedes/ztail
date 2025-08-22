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
    std::streamoff size = file.tellg();
    std::string chunk(bufferSize, '\0');
    size_t lines = 0;

    while (size > 0 && lines <= n) {
        size_t toRead = static_cast<size_t>(std::min<std::streamoff>(bufferSize, size));
        size -= toRead;
        file.seekg(size, std::ios::beg);
        file.read(&chunk[0], toRead);
        lines += static_cast<size_t>(std::count(chunk.begin(), chunk.begin() + toRead, '\n'));
        parser.parse(chunk.data(), toRead);
        if (size == 0) break;
    }

    parser.finalize();
}
