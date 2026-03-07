#include "compressor_zip.h"
#include <stdexcept>
#include <unistd.h>
#include <fnmatch.h>

CompressorZip::CompressorZip(FilePtr&& file, const std::string& filename, const std::string& entryName)
    : za(nullptr, &zip_close), zf(nullptr, &zip_fclose), entry(entryName), eof(false), filename(filename)
{
    if (!file) {
        throw std::runtime_error("libzip error (0) while opening '" + filename + "'");
    }
    std::fseek(file.get(), 0, SEEK_SET);
    int fd = fileno(file.release());
    int zipError = 0;
    za.reset(zip_fdopen(fd, ZIP_RDONLY, &zipError));
    if (!za) {
        ::close(fd);
        throw std::runtime_error("libzip error (" + std::to_string(zipError) + ") while opening '" + filename + "'");
    }

    zip_int64_t numEntries = zip_get_num_entries(za.get(), 0);
    if (numEntries == 0) {
        za.reset();
        throw std::runtime_error("libzip error (0) empty zip file '" + filename + "'");
    }

    if (!entry.empty()) {
        const bool pattern = entry.find_first_of("*?[") != std::string::npos;
        if (pattern) {
            zip_uint64_t matchedIndex = static_cast<zip_uint64_t>(-1);
            std::string matchedName;
            for (zip_uint64_t i = 0; i < static_cast<zip_uint64_t>(numEntries); ++i) {
                const char* name = zip_get_name(za.get(), i, 0);
                if (name && fnmatch(entry.c_str(), name, 0) == 0) {
                    matchedIndex = i;
                    matchedName = name;
                    break;
                }
            }
            if (matchedIndex == static_cast<zip_uint64_t>(-1)) {
                za.reset();
                throw std::runtime_error("libzip error (0) no entry matching pattern '" + entry + "' in '" + filename + "'");
            }
            zf.reset(zip_fopen_index(za.get(), matchedIndex, 0));
            entry = matchedName;
        } else {
            zf.reset(zip_fopen(za.get(), entry.c_str(), 0));
        }

        if (!zf) {
            int err = zip_error_code_zip(zip_get_error(za.get()));
            za.reset();
            throw std::runtime_error("libzip error (" + std::to_string(err) + ") while opening entry '" + entry + "' in '" + filename + "'");
        }
    } else {
        zf.reset(zip_fopen_index(za.get(), 0, 0));
        if (!zf) {
            int err = zip_error_code_zip(zip_get_error(za.get()));
            za.reset();
            throw std::runtime_error("libzip error (" + std::to_string(err) + ") while opening first entry in '" + filename + "'");
        }
    }
}

CompressorZip::~CompressorZip() {
    zf.reset();
    za.reset();
}

bool CompressorZip::decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) {
    if (eof) {
        bytesDecompressed = 0;
        return false;
    }

    zip_int64_t ret = zip_fread(zf.get(), outBuffer.data(), outBuffer.size());
    if (ret < 0) {
        int err = zip_error_code_zip(zip_file_get_error(zf.get()));
        throw std::runtime_error("libzip error (" + std::to_string(err) + ") while reading entry '" + entry + "' from '" + filename + "'");
    }
    if (ret == 0) {
        eof = true;
        bytesDecompressed = 0;
        return false;
    }

    bytesDecompressed = static_cast<size_t>(ret);
    return true;
}
