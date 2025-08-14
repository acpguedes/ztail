#include "compressor_zstd.h"
#include <stdexcept>
#include <cerrno>

CompressorZstd::CompressorZstd(FilePtr&& file, const std::string& filename)
    : file(std::move(file)), stream(nullptr, &ZSTD_freeDStream), inBuffer(ZSTD_DStreamInSize()), eof(false), filename(filename)
{
    if (!this->file) {
        throw std::runtime_error("zstd error (" + std::to_string(errno) + ") while opening '" + filename + "'");
    }
    std::fseek(this->file.get(), 0, SEEK_SET);
    stream.reset(ZSTD_createDStream());
    if (!stream) {
        this->file.reset();
        throw std::runtime_error("zstd error (0) while creating stream for '" + filename + "'");
    }
    size_t ret = ZSTD_initDStream(stream.get());
    if (ZSTD_isError(ret)) {
        stream.reset();
        this->file.reset();
        throw std::runtime_error("zstd error (" + std::to_string(static_cast<int>(ret)) + ") while initializing '" + filename + "'");
    }
}

CompressorZstd::~CompressorZstd() {
    stream.reset();
    file.reset();
}

bool CompressorZstd::decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) {
    if (eof) {
        bytesDecompressed = 0;
        return false;
    }

    ZSTD_outBuffer out{ outBuffer.data(), outBuffer.size(), 0 };
    ZSTD_inBuffer in{ inBuffer.data(), 0, 0 };

    while (out.pos < out.size) {
        if (in.pos == in.size && !eof) {
            in.size = fread(inBuffer.data(), 1, inBuffer.size(), file.get());
            in.pos = 0;
            if (in.size == 0) {
                eof = true;
                break;
            }
        }

        size_t ret = ZSTD_decompressStream(stream.get(), &out, &in);
        if (ZSTD_isError(ret)) {
            throw std::runtime_error("zstd error (" + std::to_string(static_cast<int>(ret)) + ") while decompressing '" + filename + "'");
        }
        if (ret == 0 && in.pos == in.size) {
            eof = true;
            break;
        }
        if (out.pos == out.size) {
            break;
        }
    }

    bytesDecompressed = out.pos;
    return bytesDecompressed > 0;
}
