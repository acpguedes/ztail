#include <gtest/gtest.h>
#include "compressor_tar.h"
#include "compression_type.h"

#include <archive.h>
#include <archive_entry.h>
#include <cstdio>
#include <vector>

static void create_tar_gz_with_entries(const std::string& filename) {
    archive* a = archive_write_new();
    ASSERT_NE(a, nullptr);
    ASSERT_EQ(archive_write_add_filter_gzip(a), ARCHIVE_OK);
    ASSERT_EQ(archive_write_set_format_pax_restricted(a), ARCHIVE_OK);
    ASSERT_EQ(archive_write_open_filename(a, filename.c_str()), ARCHIVE_OK);

    const std::string name1 = "logs/app.log";
    const std::string data1 = "line1\nline2\n";
    archive_entry* e1 = archive_entry_new();
    archive_entry_set_pathname(e1, name1.c_str());
    archive_entry_set_filetype(e1, AE_IFREG);
    archive_entry_set_perm(e1, 0644);
    archive_entry_set_size(e1, static_cast<la_int64_t>(data1.size()));
    ASSERT_EQ(archive_write_header(a, e1), ARCHIVE_OK);
    ASSERT_EQ(archive_write_data(a, data1.data(), data1.size()), static_cast<la_ssize_t>(data1.size()));
    archive_entry_free(e1);

    const std::string name2 = "meta/readme.txt";
    const std::string data2 = "meta\n";
    archive_entry* e2 = archive_entry_new();
    archive_entry_set_pathname(e2, name2.c_str());
    archive_entry_set_filetype(e2, AE_IFREG);
    archive_entry_set_perm(e2, 0644);
    archive_entry_set_size(e2, static_cast<la_int64_t>(data2.size()));
    ASSERT_EQ(archive_write_header(a, e2), ARCHIVE_OK);
    ASSERT_EQ(archive_write_data(a, data2.data(), data2.size()), static_cast<la_ssize_t>(data2.size()));
    archive_entry_free(e2);

    ASSERT_EQ(archive_write_close(a), ARCHIVE_OK);
    ASSERT_EQ(archive_write_free(a), ARCHIVE_OK);
}

TEST(CompressorTarTest, DecompressTarGzValidFile) {
    const std::string filename = "test_archive.tar.gz";
    create_tar_gz_with_entries(filename);

    DetectionResult det = detectCompressionType(filename);
    ASSERT_EQ(det.type, CompressionType::TAR);

    CompressorTar comp(std::move(det.file), filename);
    std::vector<char> buf(1024);
    size_t n = 0;
    std::string out;
    while (comp.decompress(buf, n)) {
        out.append(buf.data(), n);
    }

    EXPECT_NE(out.find("line1\nline2\n"), std::string::npos);
    EXPECT_NE(out.find("meta\n"), std::string::npos);

    std::remove(filename.c_str());
}

TEST(CompressorTarTest, DecompressTarGzPatternEntry) {
    const std::string filename = "test_archive_pattern.tar.gz";
    create_tar_gz_with_entries(filename);

    DetectionResult det = detectCompressionType(filename);
    ASSERT_EQ(det.type, CompressionType::TAR);

    CompressorTar comp(std::move(det.file), filename, "logs/*.log");
    std::vector<char> buf(64);
    size_t n = 0;
    std::string out;
    while (comp.decompress(buf, n)) {
        out.append(buf.data(), n);
    }

    EXPECT_EQ(out, "line1\nline2\n");

    std::remove(filename.c_str());
}
