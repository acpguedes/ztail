#include "compressor_zlib.h"

CompressorZlib::CompressorZlib(const std::string& filename)
    : gz(nullptr), eof(false)
{
    gz = gzopen(filename.c_str(), "rb");
    if (!gz) {
        throw std::runtime_error("Failed to open gz/bgz file: " + filename);
    }
}

CompressorZlib::~CompressorZlib() {
    if (gz) {
        gzclose(gz);
    }
}

bool CompressorZlib::decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) {
    if (eof) {
        bytesDecompressed = 0;
        return false;
    }

    int ret = gzread(gz, outBuffer.data(), static_cast<unsigned int>(outBuffer.size()));
    if (ret < 0) {
        throw std::runtime_error("Error while decompressing gz/bgz file");
    }
    if (ret == 0) {
        eof = true;
        bytesDecompressed = 0;
        return false;
    }

    bytesDecompressed = static_cast<size_t>(ret);
    return true;
}
