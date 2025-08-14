#ifndef COMPRESSOR_ZSTD_H
#define COMPRESSOR_ZSTD_H

#include <string>
#include <vector>
#include <zstd.h>
#include <memory>
#include "icompressor.h"
#include "file_ptr.h"

class CompressorZstd : public ICompressor {
public:
    explicit CompressorZstd(FilePtr&& file, const std::string& filename);
    ~CompressorZstd();

    bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) override;

private:
    FilePtr file;
    std::unique_ptr<ZSTD_DStream, decltype(&ZSTD_freeDStream)> stream;
    std::vector<char> inBuffer;
    bool eof;
    std::string filename;
};

#endif // COMPRESSOR_ZSTD_H
