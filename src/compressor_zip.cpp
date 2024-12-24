#include "compressor_zip.h"

CompressorZip::CompressorZip(const std::string& filename)
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

    // We only open the first file in the zip archive
    zf = zip_fopen_index(za, 0, 0);
    if (!zf) {
        zip_close(za);
        throw std::runtime_error("Failed to open the first file in zip: " + filename);
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
