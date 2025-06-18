#ifndef COMPRESSOR_ZIP_H
#define COMPRESSOR_ZIP_H

#include <string>
#include <vector>
#include <zip.h>
#include <memory>
#include "icompressor.h"

class CompressorZip : public ICompressor {
public:
    explicit CompressorZip(const std::string& filename, const std::string& entry = "");
    ~CompressorZip();

    // Reads the next chunk of decompressed data
    // Returns true while data is available, false on EOF
    bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) override;

private:
    std::unique_ptr<zip_t, decltype(&zip_close)> za;
    std::unique_ptr<zip_file_t, decltype(&zip_fclose)> zf;
    std::string entry;
    bool eof;
};

#endif // COMPRESSOR_ZIP_H
