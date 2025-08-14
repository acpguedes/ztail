#ifndef COMPRESSOR_ZSTD_H
#define COMPRESSOR_ZSTD_H

#include <string>
#include <vector>
#include <zstd.h>
#include <memory>
#include "icompressor.h"

class CompressorZstd : public ICompressor {
public:
    explicit CompressorZstd(const std::string& filename);
    ~CompressorZstd();

    bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) override;

private:
    std::unique_ptr<FILE, decltype(&fclose)> file;
    std::unique_ptr<ZSTD_DStream, decltype(&ZSTD_freeDStream)> stream;
    std::vector<char> inBuffer;
    bool eof;
    std::string filename;
};

#endif // COMPRESSOR_ZSTD_H
