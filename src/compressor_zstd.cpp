#include "compressor_zstd.h"
#include <cstdio>
#include <stdexcept>

CompressorZstd::CompressorZstd(const std::string& filename)
    : file(nullptr, &fclose), stream(nullptr, &ZSTD_freeDStream), inBuffer(ZSTD_DStreamInSize()), eof(false)
{
    file.reset(fopen(filename.c_str(), "rb"));
    if (!file) {
        throw std::runtime_error("Failed to open zst file: " + filename);
    }
    stream.reset(ZSTD_createDStream());
    if (!stream) {
        file.reset();
        throw std::runtime_error("Failed to create ZSTD stream");
    }
    size_t ret = ZSTD_initDStream(stream.get());
    if (ZSTD_isError(ret)) {
        stream.reset();
        file.reset();
        throw std::runtime_error("Failed to init ZSTD stream");
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
            throw std::runtime_error("ZSTD decompression error");
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
