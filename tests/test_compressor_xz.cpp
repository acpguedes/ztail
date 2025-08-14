#include <gtest/gtest.h>
#include "compressor_xz.h"
#include "compression_type.h"
#include <fstream>
#include <lzma.h>

// Helper function to create a temporary xz file for testing
void create_xz_file(const std::string& filename, const std::string& content) {
    size_t out_pos = 0;
    size_t out_size = lzma_stream_buffer_bound(content.size());
    std::vector<uint8_t> out(out_size);
    lzma_ret ret = lzma_easy_buffer_encode(6, LZMA_CHECK_CRC64, nullptr,
                                           reinterpret_cast<const uint8_t*>(content.data()),
                                           content.size(), out.data(), &out_pos, out_size);
    ASSERT_EQ(ret, LZMA_OK) << "Failed to encode xz";
    std::ofstream ofs(filename, std::ios::binary);
    ofs.write(reinterpret_cast<char*>(out.data()), out_pos);
}

TEST(CompressorXzTest, DecompressValidFile) {
    const std::string filename = "test.xz";
    const std::string content = "Line A\nLine B\nLine C\n";

    create_xz_file(filename, content);

    DetectionResult det = detectCompressionType(filename);
    ASSERT_EQ(det.type, CompressionType::XZ);
    CompressorXz compressor(std::move(det.file), filename);
    std::vector<char> buffer(1024);
    size_t bytesDecompressed = 0;

    std::string decompressed;
    while (compressor.decompress(buffer, bytesDecompressed)) {
        decompressed.append(buffer.data(), bytesDecompressed);
    }

    EXPECT_EQ(decompressed, content);
    remove(filename.c_str());
}

TEST(CompressorXzTest, DecompressInvalidFile) {
    const std::string filename = "test_invalid.xz";

    std::ofstream ofs(filename);
    ofs << "Invalid data";
    ofs.close();

    EXPECT_THROW({
        DetectionResult det = detectCompressionType(filename);
        CompressorXz compressor(std::move(det.file), filename);
        std::vector<char> buffer(1024);
        size_t bytesDecompressed = 0;
        compressor.decompress(buffer, bytesDecompressed);
    }, std::runtime_error);

    remove(filename.c_str());
}

