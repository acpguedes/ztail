#ifndef COMPRESSION_TYPE_H
#define COMPRESSION_TYPE_H

#include <string>
#include <cstddef>
#include "file_ptr.h"

enum class CompressionType {
    NONE,
    GZIP,
    BZIP2,
    XZ,
    ZIP,
    ZSTD,
    LZ4,
    TAR
};

struct DetectionResult {
    CompressionType type;
    FilePtr file;
};

[[nodiscard]] DetectionResult detectCompressionType(const std::string& filename);
[[nodiscard]] CompressionType detectCompressionTypeFromBytes(const unsigned char* bytes, size_t len);

void listZipEntries(const std::string& filename);
void listTarEntries(const std::string& filename);

#endif // COMPRESSION_TYPE_H
