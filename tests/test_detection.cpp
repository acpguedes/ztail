#include <gtest/gtest.h>
#include "compression_type.h"
#include <fstream>
#include <cstdio>
#include <zlib.h>
#include <bzlib.h>
#include <zip.h>

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
    EXPECT_EQ(detectCompressionType(fname), CompressionType::GZIP);
    std::remove(fname.c_str());
}

TEST(CompressionDetection, Bzip2NoExtension){
    const std::string fname = "sample";
    create_bz2(fname, "b\n");
    EXPECT_EQ(detectCompressionType(fname), CompressionType::BZIP2);
    std::remove(fname.c_str());
}

TEST(CompressionDetection, ZipMisleadingExtension){
    const std::string fname = "archive.gz";
    create_zip(fname, "c\n");
    EXPECT_EQ(detectCompressionType(fname), CompressionType::ZIP);
    std::remove(fname.c_str());
}

