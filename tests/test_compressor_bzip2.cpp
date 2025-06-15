#include <gtest/gtest.h>
#include "compressor_bzip2.h"
#include <bzlib.h>
#include <cstdio>

// Helper function to create a temporary bz2 file for testing
void create_bz2_file(const std::string& filename, const std::string& content) {
    FILE* f = fopen(filename.c_str(), "wb");
    ASSERT_NE(f, nullptr) << "Failed to create bz2 file";
    int bzerror = BZ_OK;
    BZFILE* bf = BZ2_bzWriteOpen(&bzerror, f, 9, 0, 0);
    ASSERT_EQ(bzerror, BZ_OK) << "Failed to open bz2 writer";
    BZ2_bzWrite(&bzerror, bf, const_cast<char*>(content.data()), content.size());
    ASSERT_EQ(bzerror, BZ_OK) << "Failed to write bz2 data";
    BZ2_bzWriteClose(&bzerror, bf, 0, nullptr, nullptr);
    fclose(f);
}

TEST(CompressorBzip2Test, DecompressValidFile) {
    const std::string filename = "test.bz2";
    const std::string content = "Line A\nLine B\nLine C\n";

    create_bz2_file(filename, content);

    CompressorBzip2 compressor(filename);
    std::vector<char> buffer(1024);
    size_t bytesDecompressed = 0;

    std::string decompressed;
    while (compressor.decompress(buffer, bytesDecompressed)) {
        decompressed.append(buffer.data(), bytesDecompressed);
    }

    EXPECT_EQ(decompressed, content);
    remove(filename.c_str());
}

TEST(CompressorBzip2Test, DecompressInvalidFile) {
    const std::string filename = "test_invalid.bz2";

    FILE* f = fopen(filename.c_str(), "wb");
    fprintf(f, "Invalid data");
    fclose(f);

    EXPECT_THROW({
        CompressorBzip2 compressor(filename);
        std::vector<char> buffer(1024);
        size_t bytesDecompressed = 0;

        compressor.decompress(buffer, bytesDecompressed);
    }, std::runtime_error);

    remove(filename.c_str());
}

