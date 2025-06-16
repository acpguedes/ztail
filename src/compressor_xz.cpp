#include "compressor_xz.h"
#include <fstream>

CompressorXz::CompressorXz(const std::string& filename)
    : file(nullptr, &fclose), strm(LZMA_STREAM_INIT), eof(false), inBuffer(1 << 15)
{
    std::ifstream check(filename, std::ios::binary);
    if (!check) {
        throw std::runtime_error("Failed to open xz file: " + filename);
    }

    file.reset(fopen(filename.c_str(), "rb"));
    if (!file) {
        throw std::runtime_error("Failed to open xz file: " + filename);
    }

    lzma_ret ret = lzma_stream_decoder(&strm, UINT64_MAX, 0);
    if (ret != LZMA_OK) {
        file.reset();
        throw std::runtime_error("Failed to initialize xz decoder");
    }
}

CompressorXz::~CompressorXz() {
    lzma_end(&strm);
    file.reset();
}

bool CompressorXz::decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) {
    if (eof) {
        bytesDecompressed = 0;
        return false;
    }

    strm.next_out = reinterpret_cast<uint8_t*>(outBuffer.data());
    strm.avail_out = outBuffer.size();

    bytesDecompressed = 0;

    while (strm.avail_out > 0) {
        if (strm.avail_in == 0 && !eof) {
            size_t nread = fread(inBuffer.data(), 1, inBuffer.size(), file.get());
            if (nread == 0) {
                eof = true;
            }
            strm.next_in = inBuffer.data();
            strm.avail_in = nread;
        }

        lzma_ret ret = lzma_code(&strm, eof && strm.avail_in == 0 ? LZMA_FINISH : LZMA_RUN);
        if (ret == LZMA_STREAM_END) {
            eof = true;
            break;
        }
        if (ret != LZMA_OK && ret != LZMA_BUF_ERROR) {
            throw std::runtime_error("Error while decompressing xz file");
        }
        if (ret == LZMA_BUF_ERROR && strm.avail_in == 0) {
            // Need more input
            continue;
        }
        if (ret == LZMA_BUF_ERROR) {
            break;
        }
    }

    bytesDecompressed = outBuffer.size() - strm.avail_out;
    return bytesDecompressed > 0;
}

