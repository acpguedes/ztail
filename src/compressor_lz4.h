#ifndef COMPRESSOR_LZ4_H
#define COMPRESSOR_LZ4_H

#include <lz4frame.h>
#include <string>
#include <vector>
#include "icompressor.h"
#include "file_ptr.h"

class CompressorLz4 : public ICompressor {
public:
    explicit CompressorLz4(FilePtr&& file, const std::string& filename);
    ~CompressorLz4() noexcept;

    [[nodiscard]] bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) override;

private:
    FilePtr file;
    LZ4F_dctx* dctx;
    std::vector<char> inBuffer;
    size_t inPos;
    size_t inSize;
    bool eof;
    std::string filename;
};

#endif // COMPRESSOR_LZ4_H
