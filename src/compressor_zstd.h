#ifndef COMPRESSOR_ZSTD_H
#define COMPRESSOR_ZSTD_H

#include <string>
#include <vector>
#include <zstd.h>
#include <stdexcept>

class CompressorZstd {
public:
    explicit CompressorZstd(const std::string& filename);
    ~CompressorZstd();

    bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed);

private:
    FILE* file;
    ZSTD_DStream* stream;
    std::vector<char> inBuffer;
    bool eof;
};

#endif // COMPRESSOR_ZSTD_H
