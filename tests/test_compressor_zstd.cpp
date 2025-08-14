#include <gtest/gtest.h>
#include "compressor_zstd.h"
#include "compression_type.h"
#include <zstd.h>
#include <fstream>

static void create_zst_file(const std::string& filename, const std::string& content){
    size_t bound = ZSTD_compressBound(content.size());
    std::vector<char> out(bound);
    size_t compSize = ZSTD_compress(out.data(), bound, content.data(), content.size(), 1);
    ASSERT_FALSE(ZSTD_isError(compSize));
    std::ofstream ofs(filename, std::ios::binary);
    ofs.write(out.data(), compSize);
}

TEST(CompressorZstdTest, DecompressValidFile){
    const std::string filename = "test.zst";
    const std::string content = "Line1\nLine2\n";
    create_zst_file(filename, content);
    DetectionResult det = detectCompressionType(filename);
    ASSERT_EQ(det.type, CompressionType::ZSTD);
    CompressorZstd comp(std::move(det.file), filename);
    std::vector<char> buf(1024);
    size_t n = 0;
    std::string out;
    while (comp.decompress(buf, n)) {
        out.append(buf.data(), n);
    }
    EXPECT_EQ(out, content);
    std::remove(filename.c_str());
}

TEST(CompressorZstdTest, DecompressInvalidFile){
    const std::string filename = "bad.zst";
    std::ofstream ofs(filename); ofs << "bad"; ofs.close();
    EXPECT_THROW({
        DetectionResult det = detectCompressionType(filename);
        CompressorZstd c(std::move(det.file), filename);
        std::vector<char> buf(32);
        size_t n = 0;
        c.decompress(buf, n);
    }, std::runtime_error);
    std::remove(filename.c_str());
}
