#include "compressor_bzip2.h"
#include <fstream>

CompressorBzip2::CompressorBzip2(const std::string& filename)
    : file(nullptr, &fclose), bz(nullptr), bzerror(BZ_OK), eof(false)
{
    std::ifstream check(filename, std::ios::binary);
    if (!check) {
        throw std::runtime_error("Failed to open bz2 file: " + filename);
    }

    file.reset(fopen(filename.c_str(), "rb"));
    if (!file) {
        throw std::runtime_error("Failed to open bz2 file: " + filename);
    }

    bz = BZ2_bzReadOpen(&bzerror, file.get(), 0, 0, nullptr, 0);
    if (!bz || bzerror != BZ_OK) {
        if (bz) {
            BZ2_bzReadClose(&bzerror, bz);
        }
        file.reset();
        throw std::runtime_error("Failed to initialize bz2 decompression: " + filename);
    }
}

CompressorBzip2::~CompressorBzip2() {
    if (bz) {
        BZ2_bzReadClose(&bzerror, bz);
    }
    file.reset();
}

bool CompressorBzip2::decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) {
    if (eof) {
        bytesDecompressed = 0;
        return false;
    }

    int nread = BZ2_bzRead(&bzerror, bz, outBuffer.data(), static_cast<int>(outBuffer.size()));
    if (bzerror != BZ_OK && bzerror != BZ_STREAM_END) {
        throw std::runtime_error("Error while decompressing bz2 file");
    }
    if (bzerror == BZ_STREAM_END) {
        eof = true;
    }
    if (nread < 0) {
        bytesDecompressed = 0;
        return false;
    }
    bytesDecompressed = static_cast<size_t>(nread);
    return bytesDecompressed > 0;
}

