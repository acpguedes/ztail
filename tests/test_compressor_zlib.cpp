#include <gtest/gtest.h>
#include "compressor_zlib.h"
#include <fstream>

// Helper function to create a temporary gz file for testing
void create_gz_file(const std::string& filename, const std::string& content) {
    gzFile gz = gzopen(filename.c_str(), "wb");
    ASSERT_TRUE(gz) << "Failed to create gz file";
    gzwrite(gz, content.c_str(), content.size());
    gzclose(gz);
}

TEST(CompressorZlibTest, DecompressValidFile) {
    const std::string filename = "test.gz";
    const std::string content = "Line A\nLine B\nLine C\n";

    create_gz_file(filename, content);

    CompressorZlib compressor(filename);
    std::vector<char> buffer(1024);
    size_t bytesDecompressed = 0;

    std::string decompressed;
    while (compressor.decompress(buffer, bytesDecompressed)) {
        decompressed.append(buffer.data(), bytesDecompressed);
    }

    EXPECT_EQ(decompressed, content);
    remove(filename.c_str());
}

TEST(CompressorZlibTest, DecompressInvalidFile) {
    const std::string filename = "test_invalid.gz";

    std::ofstream ofs(filename);
    ofs << "Invalid data";
    ofs.close();

    EXPECT_THROW({
        CompressorZlib compressor(filename);
    }, std::runtime_error);

    remove(filename.c_str());
}
