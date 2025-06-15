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
    std::string data;
    std::string chunk;
    size_t lines = 0;

    while (size > 0 && lines <= n) {
        size_t toRead = static_cast<size_t>(std::min<std::streamoff>(bufferSize, size));
        size -= toRead;
        file.seekg(size, std::ios::beg);
        chunk.resize(toRead);
        file.read(&chunk[0], toRead);
        data.insert(0, chunk);
        lines = std::count(data.begin(), data.end(), '\n');
        if (size == 0) break;
    }

    parser.parse(data.data(), data.size());
    parser.finalize();
}
