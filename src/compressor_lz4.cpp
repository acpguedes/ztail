#include "compressor_lz4.h"
#include <stdexcept>
#include <cerrno>

CompressorLz4::CompressorLz4(FilePtr&& file, const std::string& filename)
    : file(std::move(file)), dctx(nullptr), inBuffer(1 << 20), inPos(0), inSize(0), eof(false), filename(filename)
{
    if (!this->file) {
        throw std::runtime_error("lz4 error (" + std::to_string(errno) + ") while opening '" + filename + "'");
    }
    std::fseek(this->file.get(), 0, SEEK_SET);

    LZ4F_errorCode_t ret = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
    if (LZ4F_isError(ret)) {
        this->file.reset();
        throw std::runtime_error(std::string("lz4 error while creating context for '") + filename + "': " + LZ4F_getErrorName(ret));
    }
}

CompressorLz4::~CompressorLz4() noexcept {
    if (dctx) {
        LZ4F_freeDecompressionContext(dctx);
    }
    file.reset();
}

bool CompressorLz4::decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) {
    if (eof) {
        bytesDecompressed = 0;
        return false;
    }

    size_t outPos = 0;

    while (outPos < outBuffer.size()) {
        if (inPos >= inSize && !eof) {
            inSize = fread(inBuffer.data(), 1, inBuffer.size(), file.get());
            inPos = 0;
            if (inSize == 0) {
                eof = true;
                break;
            }
        }

        size_t srcSize = inSize - inPos;
        size_t dstSize = outBuffer.size() - outPos;

        size_t ret = LZ4F_decompress(dctx,
            outBuffer.data() + outPos, &dstSize,
            inBuffer.data() + inPos, &srcSize,
            nullptr);

        if (LZ4F_isError(ret)) {
            throw std::runtime_error(std::string("lz4 error while decompressing '") + filename + "': " + LZ4F_getErrorName(ret));
        }

        inPos += srcSize;
        outPos += dstSize;

        if (ret == 0 && inPos >= inSize && feof(file.get())) {
            eof = true;
            break;
        }
    }

    bytesDecompressed = outPos;
    return bytesDecompressed > 0;
}
