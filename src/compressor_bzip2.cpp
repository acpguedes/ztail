#include "compressor_bzip2.h"
#include <stdexcept>
#include <cerrno>

CompressorBzip2::CompressorBzip2(FilePtr&& file, const std::string& filename)
    : file(std::move(file)), bz(nullptr), bzerror(BZ_OK), eof(false), filename(filename)
{
    if (!this->file) {
        throw std::runtime_error("bzip2 error (" + std::to_string(errno) + ") while opening '" + filename + "'");
    }

    std::fseek(this->file.get(), 0, SEEK_SET);

    bz = BZ2_bzReadOpen(&bzerror, this->file.get(), 0, 0, nullptr, 0);
    if (!bz || bzerror != BZ_OK) {
        if (bz) {
            BZ2_bzReadClose(&bzerror, bz);
        }
        this->file.reset();
        throw std::runtime_error("bzip2 error (" + std::to_string(bzerror) + ") while initializing '" + filename + "'");
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
        throw std::runtime_error("bzip2 error (" + std::to_string(bzerror) + ") while decompressing '" + filename + "'");
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

