#ifndef COMPRESSOR_ZLIB_H
#define COMPRESSOR_ZLIB_H

#include <zlib.h>
#include <string>
#include <vector>
#include <stdexcept>

class CompressorZlib {
public:
    explicit CompressorZlib(const std::string& filename);
    ~CompressorZlib();

    // Reads the next chunk of decompressed data
    // Returns true while data is available, false on EOF
    bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed);

private:
    gzFile gz;
    bool eof;
};

#endif // COMPRESSOR_ZLIB_H
