#ifndef COMPRESSOR_ZIP_H
#define COMPRESSOR_ZIP_H

#include <string>
#include <vector>
#include <stdexcept>
#include <zip.h>

class CompressorZip {
public:
    explicit CompressorZip(const std::string& filename,
                           const std::string& entryName = "");
    ~CompressorZip();

    // Reads the next chunk of decompressed data
    // Returns true while data is available, false on EOF
    bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed);

private:
    zip_t* za;
    zip_file_t* zf;
    bool eof;
};

#endif // COMPRESSOR_ZIP_H
