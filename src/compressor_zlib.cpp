#include "compressor_zlib.h"
#include <fstream>
#include <stdexcept>
#include <cerrno>

CompressorZlib::CompressorZlib(const std::string& filename)
    : gz(nullptr), eof(false), filename(filename)
{
    std::ifstream check(filename, std::ios::binary);
    if (!check) {
        throw std::runtime_error("zlib error (" + std::to_string(errno) + ") while opening '" + filename + "'");
    }

    unsigned char header[2];
    check.read(reinterpret_cast<char*>(header), 2);
    if (check.gcount() != 2 || header[0] != 0x1f || header[1] != 0x8b) {
        throw std::runtime_error("zlib error (0) invalid header in '" + filename + "'");
    }

    gz.reset(gzopen(filename.c_str(), "rb"));
    if (!gz) {
        throw std::runtime_error("zlib error (" + std::to_string(errno) + ") while opening '" + filename + "'");
    }
}

bool CompressorZlib::decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) {
    if (eof) {
        bytesDecompressed = 0;
        return false;
    }

    int ret = gzread(gz.get(), outBuffer.data(), static_cast<unsigned int>(outBuffer.size()));
    if (ret < 0) {
        int errnum = 0;
        gzerror(gz.get(), &errnum);
        throw std::runtime_error("zlib error (" + std::to_string(errnum) + ") while decompressing '" + filename + "'");
    }
    if (ret == 0) {
        eof = true;
        bytesDecompressed = 0;
        return false;
    }

    bytesDecompressed = static_cast<size_t>(ret);
    return true;
}
