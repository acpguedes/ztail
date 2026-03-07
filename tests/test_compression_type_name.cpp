#include <gtest/gtest.h>
#include "compression_type.h"
#include <cstring>

TEST(CompressionTypeName, AllFormats) {
    EXPECT_STREQ(compressionTypeName(CompressionType::GZIP), "gzip");
    EXPECT_STREQ(compressionTypeName(CompressionType::BZIP2), "bzip2");
    EXPECT_STREQ(compressionTypeName(CompressionType::XZ), "xz");
    EXPECT_STREQ(compressionTypeName(CompressionType::ZSTD), "zstd");
    EXPECT_STREQ(compressionTypeName(CompressionType::ZIP), "zip");
    EXPECT_STREQ(compressionTypeName(CompressionType::LZ4), "lz4");
    EXPECT_STREQ(compressionTypeName(CompressionType::TAR), "tar");
    EXPECT_STREQ(compressionTypeName(CompressionType::NONE), "plain");
}

TEST(CompressionDetection, BgzfExtension) {
    // Create a simple gzip file with .bgzf extension
    const std::string fname = "test_sample.bgzf";
    // Write gzip magic bytes so detection works
    FILE* f = std::fopen(fname.c_str(), "wb");
    ASSERT_TRUE(f);
    unsigned char gz_header[] = {0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00};
    std::fwrite(gz_header, 1, sizeof(gz_header), f);
    std::fclose(f);

    auto result = detectCompressionType(fname);
    EXPECT_EQ(result.type, CompressionType::GZIP);
    std::remove(fname.c_str());
}

TEST(CompressionDetection, BgzfExtensionFallback) {
    // Plain text with .bgzf extension should fallback to GZIP
    const std::string fname = "test_fallback.bgzf";
    FILE* f = std::fopen(fname.c_str(), "w");
    ASSERT_TRUE(f);
    std::fprintf(f, "plain text data\n");
    std::fclose(f);

    auto result = detectCompressionType(fname);
    EXPECT_EQ(result.type, CompressionType::GZIP);
    std::remove(fname.c_str());
}

TEST(CompressionDetection, DetectFromMagicBytes) {
    // Test magic bytes detection directly
    unsigned char gzip_bytes[] = {0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00};
    EXPECT_EQ(detectCompressionTypeFromBytes(gzip_bytes, 6), CompressionType::GZIP);

    unsigned char zstd_bytes[] = {0x28, 0xB5, 0x2F, 0xFD, 0x00, 0x00};
    EXPECT_EQ(detectCompressionTypeFromBytes(zstd_bytes, 6), CompressionType::ZSTD);

    unsigned char lz4_bytes[] = {0x04, 0x22, 0x4D, 0x18, 0x00, 0x00};
    EXPECT_EQ(detectCompressionTypeFromBytes(lz4_bytes, 6), CompressionType::LZ4);

    unsigned char bz2_bytes[] = {'B', 'Z', 'h', '9', 0x00, 0x00};
    EXPECT_EQ(detectCompressionTypeFromBytes(bz2_bytes, 6), CompressionType::BZIP2);

    unsigned char xz_bytes[] = {0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00};
    EXPECT_EQ(detectCompressionTypeFromBytes(xz_bytes, 6), CompressionType::XZ);

    unsigned char zip_bytes[] = {0x50, 0x4B, 0x03, 0x04, 0x00, 0x00};
    EXPECT_EQ(detectCompressionTypeFromBytes(zip_bytes, 6), CompressionType::ZIP);

    unsigned char plain_bytes[] = {'H', 'e', 'l', 'l', 'o', '\n'};
    EXPECT_EQ(detectCompressionTypeFromBytes(plain_bytes, 6), CompressionType::NONE);

    // Test with insufficient bytes
    EXPECT_EQ(detectCompressionTypeFromBytes(gzip_bytes, 1), CompressionType::NONE);
}
