#include "compression_type.h"
#include <cstdio>

DetectionResult detectCompressionType(const std::string& filename) {
    FilePtr file(std::fopen(filename.c_str(), "rb"));
    if (!file) {
        return {CompressionType::NONE, FilePtr(nullptr)};
    }

    unsigned char bytes[6] = {0};
    size_t read = std::fread(bytes, 1, sizeof(bytes), file.get());

    CompressionType type = CompressionType::NONE;
    if (read >= 4 && bytes[0] == 0x28 && bytes[1] == 0xB5 && bytes[2] == 0x2F && bytes[3] == 0xFD) {
        type = CompressionType::ZSTD;
    } else if (read >= 2 && bytes[0] == 0x1f && bytes[1] == 0x8b) {
        type = CompressionType::GZIP;      // gzip or bgz
    } else if (read >= 3 && bytes[0] == 'B' && bytes[1] == 'Z' && bytes[2] == 'h') {
        type = CompressionType::BZIP2;     // bzip2
    } else if (read >= 6 && bytes[0] == 0xFD && bytes[1] == 0x37 && bytes[2] == 0x7A &&
               bytes[3] == 0x58 && bytes[4] == 0x5A && bytes[5] == 0x00) {
        type = CompressionType::XZ;        // xz
    } else if (read >= 4 && bytes[0] == 0x50 && bytes[1] == 0x4B &&
               (bytes[2] == 0x03 || bytes[2] == 0x05 || bytes[2] == 0x07) &&
               (bytes[3] == 0x04 || bytes[3] == 0x06 || bytes[3] == 0x08)) {
        type = CompressionType::ZIP;       // zip
    } else {
        // Fallback based on file extension
        if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".zst") == 0) {
            type = CompressionType::ZSTD;
        } else if (filename.size() >= 3 && filename.compare(filename.size() - 3, 3, ".gz") == 0) {
            type = CompressionType::GZIP;
        } else if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".bgz") == 0) {
            type = CompressionType::GZIP;
        } else if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".bz2") == 0) {
            type = CompressionType::BZIP2;
        } else if (filename.size() >= 3 && filename.compare(filename.size() - 3, 3, ".xz") == 0) {
            type = CompressionType::XZ;
        } else if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".zip") == 0) {
            type = CompressionType::ZIP;
        }
    }

    return {type, std::move(file)};
}
