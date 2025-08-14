#ifndef COMPRESSOR_XZ_H
#define COMPRESSOR_XZ_H

#include <lzma.h>
#include <string>
#include <vector>
#include <cstdio>
#include <memory>
#include "icompressor.h"
#include "file_ptr.h"

class CompressorXz : public ICompressor {
public:
    explicit CompressorXz(FilePtr&& file, const std::string& filename);
    ~CompressorXz();

    // Reads the next chunk of decompressed data
    // Returns true while data is available, false on EOF
    bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) override;

private:
    FilePtr file;
    lzma_stream strm;
    bool eof;
    std::vector<uint8_t> inBuffer;
    std::string filename;
};

#endif // COMPRESSOR_XZ_H
