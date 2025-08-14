#ifndef COMPRESSOR_BZIP2_H
#define COMPRESSOR_BZIP2_H

#include <bzlib.h>
#include <string>
#include <vector>
#include <cstdio>
#include <memory>
#include "icompressor.h"
#include "file_ptr.h"

class CompressorBzip2 : public ICompressor {
public:
    explicit CompressorBzip2(FilePtr&& file, const std::string& filename);
    ~CompressorBzip2();

    // Reads the next chunk of decompressed data
    // Returns true while data is available, false on EOF
    bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) override;

private:
    FilePtr file;
    BZFILE* bz;
    int bzerror;
    bool eof;
    std::string filename;
};

#endif // COMPRESSOR_BZIP2_H
