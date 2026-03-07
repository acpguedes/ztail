#ifndef COMPRESSOR_TAR_H
#define COMPRESSOR_TAR_H

#include <archive.h>
#include <archive_entry.h>
#include <memory>
#include <string>
#include <vector>
#include "icompressor.h"
#include "file_ptr.h"

class CompressorTar : public ICompressor {
public:
    explicit CompressorTar(FilePtr&& file, const std::string& filename, const std::string& entryPattern = "");
    ~CompressorTar() noexcept override;

    [[nodiscard]] bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) override;

private:
    bool advanceToNextMatchingEntry();

    FilePtr file;
    std::unique_ptr<archive, decltype(&archive_read_free)> ar;
    std::string filename;
    std::string entryPattern;
    bool eof;
    bool entryOpen;
};

#endif // COMPRESSOR_TAR_H
