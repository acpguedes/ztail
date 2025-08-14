#ifndef COMPRESSION_TYPE_H
#define COMPRESSION_TYPE_H

#include <string>
#include "file_ptr.h"

enum class CompressionType {
    NONE,
    GZIP,
    BZIP2,
    XZ,
    ZIP,
    ZSTD
};

struct DetectionResult {
    CompressionType type;
    FilePtr file;
};

DetectionResult detectCompressionType(const std::string& filename);


#endif // COMPRESSION_TYPE_H
