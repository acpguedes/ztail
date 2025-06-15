#ifndef COMPRESSION_TYPE_H
#define COMPRESSION_TYPE_H

#include <string>

enum class CompressionType {
    NONE,
    GZIP,
    BZIP2,
    XZ,
    ZIP,
    ZSTD
};

CompressionType detectCompressionType(const std::string& filename);


#endif // COMPRESSION_TYPE_H
