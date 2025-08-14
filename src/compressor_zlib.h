#ifndef COMPRESSOR_ZLIB_H
#define COMPRESSOR_ZLIB_H

#include <zlib.h>
#include <string>
#include <vector>
#include <memory>
#include "icompressor.h"
#include "file_ptr.h"

struct GzCloser { void operator()(gzFile f) const { if (f) gzclose(f); } };

class CompressorZlib : public ICompressor {
public:
    explicit CompressorZlib(FilePtr&& file, const std::string& filename);

    // Reads the next chunk of decompressed data
    // Returns true while data is available, false on EOF
    bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) override;

private:
    std::unique_ptr<gzFile_s, GzCloser> gz;
    bool eof;
    std::string filename;
};

#endif // COMPRESSOR_ZLIB_H
