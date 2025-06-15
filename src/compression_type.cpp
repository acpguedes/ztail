#include "compression_type.h"
#include <fstream>

CompressionType detectCompressionType(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return CompressionType::NONE;
    }

    unsigned char bytes[6] = {0};
    file.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
    std::size_t read = static_cast<std::size_t>(file.gcount());

    if (read >= 4 && bytes[0] == 0x28 && bytes[1] == 0xB5 && bytes[2] == 0x2F && bytes[3] == 0xFD) {
        return CompressionType::ZSTD;
    }
    if (read >= 2 && bytes[0] == 0x1f && bytes[1] == 0x8b) {
        return CompressionType::GZIP;      // gzip or bgz
    }
    if (read >= 3 && bytes[0] == 'B' && bytes[1] == 'Z' && bytes[2] == 'h') {
        return CompressionType::BZIP2;     // bzip2
    }
    if (read >= 6 && bytes[0] == 0xFD && bytes[1] == 0x37 && bytes[2] == 0x7A &&
        bytes[3] == 0x58 && bytes[4] == 0x5A && bytes[5] == 0x00) {
        return CompressionType::XZ;        // xz
    }
    if (read >= 4 && bytes[0] == 0x50 && bytes[1] == 0x4B &&
        (bytes[2] == 0x03 || bytes[2] == 0x05 || bytes[2] == 0x07) &&
        (bytes[3] == 0x04 || bytes[3] == 0x06 || bytes[3] == 0x08)) {
        return CompressionType::ZIP;       // zip
    }

    // Fallback based on file extension
    if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".zst") == 0) {
        return CompressionType::ZSTD;
    }
    if (filename.size() >= 3 && filename.compare(filename.size() - 3, 3, ".gz") == 0) {
        return CompressionType::GZIP;
    }
    if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".bgz") == 0) {
        return CompressionType::GZIP;
    }
    if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".bz2") == 0) {
        return CompressionType::BZIP2;
    }
    if (filename.size() >= 3 && filename.compare(filename.size() - 3, 3, ".xz") == 0) {
        return CompressionType::XZ;
    }
    if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".zip") == 0) {
        return CompressionType::ZIP;
    }

    return CompressionType::NONE;
}
