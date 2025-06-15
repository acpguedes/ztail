#ifndef COMPRESSION_TYPE_H
#define COMPRESSION_TYPE_H

#include <string>

enum class CompressionType {
    NONE,
    GZIP,
    BZIP2,
    XZ,
    ZIP
};

CompressionType detectCompressionType(const std::string& filename);
CompressionType detect_type(const std::string& filename);

#endif // COMPRESSION_TYPE_H
