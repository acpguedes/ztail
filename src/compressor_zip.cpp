#include "compressor_zip.h"
#include <stdexcept>

CompressorZip::CompressorZip(const std::string& filename, const std::string& entryName)
    : za(nullptr, &zip_close), zf(nullptr, &zip_fclose), entry(entryName), eof(false)
{
    int zipError = 0;
    za.reset(zip_open(filename.c_str(), ZIP_RDONLY, &zipError));
    if (!za) {
        throw std::runtime_error("Failed to open zip file: " + filename);
    }

    zip_int64_t numEntries = zip_get_num_entries(za.get(), 0);
    if (numEntries == 0) {
        za.reset();
        throw std::runtime_error("Empty zip file: " + filename);
    }

    if (!entry.empty()) {
        zf.reset(zip_fopen(za.get(), entry.c_str(), 0));
        if (!zf) {
            za.reset();
            throw std::runtime_error("Failed to open entry " + entry + " in zip: " + filename);
        }
    } else {
        zf.reset(zip_fopen_index(za.get(), 0, 0));
        if (!zf) {
            za.reset();
            throw std::runtime_error("Failed to open the first file in zip: " + filename);
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
        throw std::runtime_error("Error while reading from zip file");
    }
    if (ret == 0) {
        eof = true;
        bytesDecompressed = 0;
        return false;
    }

    bytesDecompressed = static_cast<size_t>(ret);
    return true;
}
