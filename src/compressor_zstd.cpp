#include "compressor_zstd.h"
#include <cstdio>
#include <cstring>

CompressorZstd::CompressorZstd(const std::string& filename)
    : file(nullptr), stream(nullptr), inBuffer(ZSTD_DStreamInSize()), eof(false)
{
    file = fopen(filename.c_str(), "rb");
    if (!file) {
        throw std::runtime_error("Failed to open zst file: " + filename);
    }
    stream = ZSTD_createDStream();
    if (!stream) {
        fclose(file);
        throw std::runtime_error("Failed to create ZSTD stream");
    }
    size_t ret = ZSTD_initDStream(stream);
    if (ZSTD_isError(ret)) {
        ZSTD_freeDStream(stream);
        fclose(file);
        throw std::runtime_error("Failed to init ZSTD stream");
    }
}

CompressorZstd::~CompressorZstd() {
    if (stream) ZSTD_freeDStream(stream);
    if (file) fclose(file);
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
            in.size = fread(inBuffer.data(), 1, inBuffer.size(), file);
            in.pos = 0;
            if (in.size == 0) {
                eof = true;
                break;
            }
        }

        size_t ret = ZSTD_decompressStream(stream, &out, &in);
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
