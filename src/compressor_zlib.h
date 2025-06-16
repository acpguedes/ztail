#ifndef COMPRESSOR_ZLIB_H
#define COMPRESSOR_ZLIB_H

#include <zlib.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include "icompressor.h"

struct GzCloser { void operator()(gzFile f) const { if (f) gzclose(f); } };

class CompressorZlib : public ICompressor {
public:
    explicit CompressorZlib(const std::string& filename);

    // Reads the next chunk of decompressed data
    // Returns true while data is available, false on EOF
    bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) override;

private:
    std::unique_ptr<gzFile_s, GzCloser> gz;
    bool eof;
};

#endif // COMPRESSOR_ZLIB_H
