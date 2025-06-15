#ifndef COMPRESSOR_BZIP2_H
#define COMPRESSOR_BZIP2_H

#include <bzlib.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdio>

class CompressorBzip2 {
public:
    explicit CompressorBzip2(const std::string& filename);
    ~CompressorBzip2();

    // Reads the next chunk of decompressed data
    // Returns true while data is available, false on EOF
    bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed);

private:
    FILE* file;
    BZFILE* bz;
    int bzerror;
    bool eof;
};

#endif // COMPRESSOR_BZIP2_H
