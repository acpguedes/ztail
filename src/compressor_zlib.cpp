#include "compressor_zlib.h"
#include <stdexcept>
#include <cerrno>
#include <unistd.h>

CompressorZlib::CompressorZlib(FilePtr&& file, const std::string& filename)
    : gz(nullptr), eof(false), filename(filename)
{
    if (!file) {
        throw std::runtime_error("zlib error (0) while opening '" + filename + "'");
    }
    std::fseek(file.get(), 0, SEEK_SET);
    unsigned char header[2];
    size_t n = std::fread(header, 1, 2, file.get());
    if (n != 2 || header[0] != 0x1f || header[1] != 0x8b) {
        throw std::runtime_error("zlib error (0) invalid header in '" + filename + "'");
    }
    std::fseek(file.get(), 0, SEEK_SET);
    int fd = fileno(file.get());
    int dupfd = dup(fd);
    file.reset();
    if (dupfd == -1) {
        throw std::runtime_error("zlib error (" + std::to_string(errno) + ") while duplicating descriptor for '" + filename + "'");
    }
    ::lseek(dupfd, 0, SEEK_SET);
    gz.reset(gzdopen(dupfd, "rb"));
    if (!gz) {
        ::close(dupfd);
        throw std::runtime_error("zlib error (" + std::to_string(errno) + ") while opening '" + filename + "'");
    }
}

bool CompressorZlib::decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) {
    if (eof) {
        bytesDecompressed = 0;
        return false;
    }

    int ret = gzread(gz.get(), outBuffer.data(), static_cast<unsigned int>(outBuffer.size()));
    if (ret < 0) {
        int errnum = 0;
        gzerror(gz.get(), &errnum);
        throw std::runtime_error("zlib error (" + std::to_string(errnum) + ") while decompressing '" + filename + "'");
    }
    if (ret == 0) {
        eof = true;
        bytesDecompressed = 0;
        return false;
    }

    bytesDecompressed = static_cast<size_t>(ret);
    return true;
}
