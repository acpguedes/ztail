#include <gtest/gtest.h>
#include "compressor_zip.h"
#include <fstream>
#include <zip.h>

// Helper function to create a single-entry zip file for testing
void create_zip_file(const std::string& filename,
                     const std::string& entry,
                     const std::string& content) {
    int error = 0;
    zip_t* za = zip_open(filename.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    ASSERT_NE(za, nullptr) << "Failed to create zip file";

    zip_source_t* zs = zip_source_buffer(za, content.c_str(), content.size(), 0);
    ASSERT_NE(zs, nullptr) << "Failed to create zip source";

    int idx = zip_file_add(za, entry.c_str(), zs, ZIP_FL_OVERWRITE);
    ASSERT_GE(idx, 0) << "Failed to add file to zip";

    ASSERT_EQ(zip_close(za), 0) << "Failed to close zip file";
}

// Helper to create a zip with two entries for entry selection tests
void create_zip_with_two_entries(const std::string& filename,
                                 const std::string& entry1,
                                 const std::string& content1,
                                 const std::string& entry2,
                                 const std::string& content2) {
    int error = 0;
    zip_t* za = zip_open(filename.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    ASSERT_NE(za, nullptr) << "Failed to create zip file";

    zip_source_t* zs1 = zip_source_buffer(za, content1.c_str(), content1.size(), 0);
    ASSERT_NE(zs1, nullptr) << "Failed to create zip source";
    int idx1 = zip_file_add(za, entry1.c_str(), zs1, ZIP_FL_OVERWRITE);
    ASSERT_GE(idx1, 0) << "Failed to add file to zip";

    zip_source_t* zs2 = zip_source_buffer(za, content2.c_str(), content2.size(), 0);
    ASSERT_NE(zs2, nullptr) << "Failed to create zip source";
    int idx2 = zip_file_add(za, entry2.c_str(), zs2, ZIP_FL_OVERWRITE);
    ASSERT_GE(idx2, 0) << "Failed to add file to zip";

    ASSERT_EQ(zip_close(za), 0) << "Failed to close zip file";
}

TEST(CompressorZipTest, DecompressValidFile) {
    const std::string filename = "test.zip";
    const std::string content = "Line X\nLine Y\nLine Z\n";

    create_zip_file(filename, "test.txt", content);

    CompressorZip compressor(filename);
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
        CompressorZip compressor(filename);
        std::vector<char> buffer(1024);
        size_t bytesDecompressed = 0;

        compressor.decompress(buffer, bytesDecompressed);
    }, std::runtime_error);

    remove(filename.c_str());
}

TEST(CompressorZipTest, DecompressSpecificEntry) {
    const std::string filename = "multi.zip";
    const std::string content1 = "One\nTwo\n";
    const std::string content2 = "Three\nFour\n";

    create_zip_with_two_entries(filename, "first.txt", content1,
                                "second.txt", content2);

    CompressorZip compressor(filename, "second.txt");
    std::vector<char> buffer(1024);
    size_t bytesDecompressed = 0;

    std::string decompressed;
    while (compressor.decompress(buffer, bytesDecompressed)) {
        decompressed.append(buffer.data(), bytesDecompressed);
    }

    EXPECT_EQ(decompressed, content2);
    remove(filename.c_str());
}
