#include <gtest/gtest.h>
#include "compressor_zip.h"
#include "compression_type.h"
#include <fstream>
#include <zip.h>

// Helper function to create a zip file for testing
void create_zip_file(const std::string& filename, const std::string& content) {
    int error = 0;
    zip_t* za = zip_open(filename.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    ASSERT_NE(za, nullptr) << "Failed to create zip file";

    zip_source_t* zs = zip_source_buffer(za, content.c_str(), content.size(), 0);
    ASSERT_NE(zs, nullptr) << "Failed to create zip source";

    int idx = zip_file_add(za, "test.txt", zs, ZIP_FL_OVERWRITE);
    ASSERT_GE(idx, 0) << "Failed to add file to zip";

    ASSERT_EQ(zip_close(za), 0) << "Failed to close zip file";
}

TEST(CompressorZipTest, DecompressValidFile) {
    const std::string filename = "test.zip";
    const std::string content = "Line X\nLine Y\nLine Z\n";

    create_zip_file(filename, content);

    DetectionResult det = detectCompressionType(filename);
    ASSERT_EQ(det.type, CompressionType::ZIP);
    CompressorZip compressor(std::move(det.file), filename, "test.txt");
    std::vector<char> buffer(1024);
    size_t bytesDecompressed = 0;

    std::string decompressed;
    while (compressor.decompress(buffer, bytesDecompressed)) {
        decompressed.append(buffer.data(), bytesDecompressed);
    }

    EXPECT_EQ(decompressed, content);
    remove(filename.c_str());
}

TEST(CompressorZipTest, DecompressInvalidFile) {
    const std::string filename = "test_invalid.zip";

    std::ofstream ofs(filename);
    ofs << "Invalid zip data";
    ofs.close();

    EXPECT_THROW({
        DetectionResult det = detectCompressionType(filename);
        CompressorZip compressor(std::move(det.file), filename);
        std::vector<char> buffer(1024);
        size_t bytesDecompressed = 0;

        compressor.decompress(buffer, bytesDecompressed);
    }, std::runtime_error);

    remove(filename.c_str());
}
