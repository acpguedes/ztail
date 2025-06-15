#include "compressor_zip.h"

CompressorZip::CompressorZip(const std::string& filename,
                             const std::string& entryName)
    : za(nullptr), zf(nullptr), eof(false)
{
    int zipError = 0;
    za = zip_open(filename.c_str(), ZIP_RDONLY, &zipError);
    if (!za) {
        throw std::runtime_error("Failed to open zip file: " + filename);
    }

    zip_int64_t numEntries = zip_get_num_entries(za, 0);
    if (numEntries == 0) {
        zip_close(za);
        throw std::runtime_error("Empty zip file: " + filename);
    }

    zip_uint64_t idx = 0;
    if (!entryName.empty()) {
        zip_int64_t loc = zip_name_locate(za, entryName.c_str(), 0);
        if (loc < 0) {
            zip_close(za);
            throw std::runtime_error("Failed to locate entry in zip: " + entryName);
        }
        idx = static_cast<zip_uint64_t>(loc);
    }

    // Open the requested file (default index 0)
    zf = zip_fopen_index(za, idx, 0);
    if (!zf) {
        zip_close(za);
        throw std::runtime_error("Failed to open entry in zip: " + filename);
    }
}

CompressorZip::~CompressorZip() {
    if (zf) {
        zip_fclose(zf);
    }
    if (za) {
        zip_close(za);
    }
}

bool CompressorZip::decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) {
    if (eof) {
        bytesDecompressed = 0;
        return false;
    }

    zip_int64_t ret = zip_fread(zf, outBuffer.data(), outBuffer.size());
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
