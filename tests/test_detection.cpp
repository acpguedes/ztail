#include <gtest/gtest.h>
#include "compression_type.h"
#include <fstream>
#include <cstdio>
#include <zlib.h>
#include <bzlib.h>
#include <zip.h>
#include <lzma.h>
#include <zstd.h>

static void create_gz(const std::string& fname, const std::string& data){
    gzFile gz = gzopen(fname.c_str(), "wb");
    ASSERT_TRUE(gz);
    gzwrite(gz, data.data(), data.size());
    gzclose(gz);
}

static void create_bz2(const std::string& fname, const std::string& data){
    FILE* f = fopen(fname.c_str(), "wb");
    ASSERT_TRUE(f);
    int bzerror = BZ_OK;
    BZFILE* bz = BZ2_bzWriteOpen(&bzerror, f, 9, 0, 0);
    ASSERT_EQ(bzerror, BZ_OK);
    BZ2_bzWrite(&bzerror, bz, const_cast<char*>(data.data()), data.size());
    ASSERT_EQ(bzerror, BZ_OK);
    BZ2_bzWriteClose(&bzerror, bz, 0, nullptr, nullptr);
    fclose(f);
}

static void create_zip(const std::string& fname, const std::string& data){
    int error = 0;
    zip_t* za = zip_open(fname.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    ASSERT_NE(za, nullptr);
    zip_source_t* zs = zip_source_buffer(za, data.c_str(), data.size(), 0);
    ASSERT_NE(zs, nullptr);
    int idx = zip_file_add(za, "file.txt", zs, ZIP_FL_OVERWRITE);
    ASSERT_GE(idx, 0);
    ASSERT_EQ(zip_close(za), 0);
}

TEST(CompressionDetection, GzipWrongExtension){
    const std::string fname = "sample.txt";
    create_gz(fname, "a\n");
    EXPECT_EQ(detectCompressionType(fname).type, CompressionType::GZIP);
    std::remove(fname.c_str());
}

TEST(CompressionDetection, Bzip2NoExtension){
    const std::string fname = "sample";
    create_bz2(fname, "b\n");
    EXPECT_EQ(detectCompressionType(fname).type, CompressionType::BZIP2);
    std::remove(fname.c_str());
}

TEST(CompressionDetection, ZipMisleadingExtension){
    const std::string fname = "archive.gz";
    create_zip(fname, "c\n");
    EXPECT_EQ(detectCompressionType(fname).type, CompressionType::ZIP);
    std::remove(fname.c_str());
}

static void create_xz(const std::string& fname, const std::string& data){
    size_t out_pos = 0;
    size_t out_size = lzma_stream_buffer_bound(data.size());
    std::vector<uint8_t> out(out_size);
    lzma_ret ret = lzma_easy_buffer_encode(6, LZMA_CHECK_CRC64, nullptr,
                                           reinterpret_cast<const uint8_t*>(data.data()),
                                           data.size(), out.data(), &out_pos, out_size);
    ASSERT_EQ(ret, LZMA_OK);
    std::ofstream ofs(fname, std::ios::binary);
    ofs.write(reinterpret_cast<char*>(out.data()), out_pos);
}

static void create_zst(const std::string& fname, const std::string& data){
    size_t const cBuffSize = ZSTD_compressBound(data.size());
    std::vector<char> cBuff(cBuffSize);
    size_t cSize = ZSTD_compress(cBuff.data(), cBuffSize, data.data(), data.size(), 1);
    ASSERT_FALSE(ZSTD_isError(cSize));
    std::ofstream ofs(fname, std::ios::binary);
    ofs.write(cBuff.data(), cSize);
}

TEST(CompressionDetection, XzNoExtension){
    const std::string fname = "archive";
    create_xz(fname, "d\n");
    EXPECT_EQ(detectCompressionType(fname).type, CompressionType::XZ);
    std::remove(fname.c_str());
}

TEST(CompressionDetection, ZstdNoExtension){
    const std::string fname = "compressed";
    create_zst(fname, "e\n");
    EXPECT_EQ(detectCompressionType(fname).type, CompressionType::ZSTD);
    std::remove(fname.c_str());
}

TEST(CompressionDetection, PlaintextFile){
    const std::string fname = "plain.txt";
    std::ofstream ofs(fname);
    ofs << "hello\n";
    ofs.close();
    EXPECT_EQ(detectCompressionType(fname).type, CompressionType::NONE);
    std::remove(fname.c_str());
}

