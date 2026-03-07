#include "compressor_tar.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fnmatch.h>
#include <stdexcept>

CompressorTar::CompressorTar(FilePtr&& input, const std::string& filename, const std::string& pattern)
    : file(std::move(input)), ar(archive_read_new(), &archive_read_free),
      filename(filename), entryPattern(pattern), eof(false), entryOpen(false)
{
    if (!ar) {
        throw std::runtime_error("libarchive error while initializing reader for '" + filename + "'");
    }

    archive_read_support_filter_all(ar.get());
    archive_read_support_format_tar(ar.get());

    int ret = ARCHIVE_FATAL;
    if (file) {
        std::fseek(file.get(), 0, SEEK_SET);
        int fd = fileno(file.get());
        if (fd < 0) {
            throw std::runtime_error("libarchive error (" + std::to_string(errno) + ") invalid file descriptor for '" + filename + "'");
        }
        ret = archive_read_open_fd(ar.get(), fd, 10240);
    } else {
        ret = archive_read_open_filename(ar.get(), filename.c_str(), 10240);
    }

    if (ret != ARCHIVE_OK) {
        throw std::runtime_error("libarchive error while opening '" + filename + "': " + archive_error_string(ar.get()));
    }
}

CompressorTar::~CompressorTar() noexcept {
    ar.reset();
    file.reset();
}

bool CompressorTar::advanceToNextMatchingEntry() {
    archive_entry* entry = nullptr;
    while (true) {
        int ret = archive_read_next_header(ar.get(), &entry);
        if (ret == ARCHIVE_EOF) {
            eof = true;
            return false;
        }
        if (ret != ARCHIVE_OK) {
            throw std::runtime_error("libarchive error while reading headers from '" + filename + "': " + archive_error_string(ar.get()));
        }

        const mode_t type = archive_entry_filetype(entry);
        if (type != AE_IFREG) {
            archive_read_data_skip(ar.get());
            continue;
        }

        const char* name = archive_entry_pathname(entry);
        if (!name) {
            archive_read_data_skip(ar.get());
            continue;
        }

        if (!entryPattern.empty() && fnmatch(entryPattern.c_str(), name, 0) != 0) {
            archive_read_data_skip(ar.get());
            continue;
        }

        entryOpen = true;
        return true;
    }
}

bool CompressorTar::decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) {
    if (eof) {
        bytesDecompressed = 0;
        return false;
    }

    size_t outPos = 0;
    while (outPos < outBuffer.size()) {
        if (!entryOpen) {
            if (!advanceToNextMatchingEntry()) {
                break;
            }
        }

        la_ssize_t n = archive_read_data(ar.get(), outBuffer.data() + outPos, outBuffer.size() - outPos);
        if (n > 0) {
            outPos += static_cast<size_t>(n);
            continue;
        }
        if (n == 0) {
            entryOpen = false;
            if (outPos > 0) {
                break;
            }
            continue;
        }

        throw std::runtime_error("libarchive error while reading data from '" + filename + "': " + archive_error_string(ar.get()));
    }

    bytesDecompressed = outPos;
    return bytesDecompressed > 0;
}
