#include "compression_type.h"

#include <archive.h>
#include <archive_entry.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <zip.h>

CompressionType detectCompressionTypeFromBytes(const unsigned char* bytes, size_t read) {
    if (read >= 4 && bytes[0] == 0x28 && bytes[1] == 0xB5 && bytes[2] == 0x2F && bytes[3] == 0xFD) {
        return CompressionType::ZSTD;
    }
    if (read >= 4 && bytes[0] == 0x04 && bytes[1] == 0x22 && bytes[2] == 0x4D && bytes[3] == 0x18) {
        return CompressionType::LZ4;
    }
    if (read >= 2 && bytes[0] == 0x1f && bytes[1] == 0x8b) {
        return CompressionType::GZIP;
    }
    if (read >= 3 && bytes[0] == 'B' && bytes[1] == 'Z' && bytes[2] == 'h') {
        return CompressionType::BZIP2;
    }
    if (read >= 6 && bytes[0] == 0xFD && bytes[1] == 0x37 && bytes[2] == 0x7A &&
               bytes[3] == 0x58 && bytes[4] == 0x5A && bytes[5] == 0x00) {
        return CompressionType::XZ;
    }
    if (read >= 4 && bytes[0] == 0x50 && bytes[1] == 0x4B &&
               (bytes[2] == 0x03 || bytes[2] == 0x05 || bytes[2] == 0x07) &&
               (bytes[3] == 0x04 || bytes[3] == 0x06 || bytes[3] == 0x08)) {
        return CompressionType::ZIP;
    }
    return CompressionType::NONE;
}

static CompressionType fallbackByExtension(const std::string& filename) {
    if (filename.size() >= 7 && filename.compare(filename.size() - 7, 7, ".tar.gz") == 0) return CompressionType::TAR;
    if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".tgz") == 0) return CompressionType::TAR;
    if (filename.size() >= 8 && filename.compare(filename.size() - 8, 8, ".tar.bz2") == 0) return CompressionType::TAR;
    if (filename.size() >= 5 && filename.compare(filename.size() - 5, 5, ".tbz2") == 0) return CompressionType::TAR;
    if (filename.size() >= 7 && filename.compare(filename.size() - 7, 7, ".tar.xz") == 0) return CompressionType::TAR;
    if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".txz") == 0) return CompressionType::TAR;
    if (filename.size() >= 8 && filename.compare(filename.size() - 8, 8, ".tar.zst") == 0) return CompressionType::TAR;
    if (filename.size() >= 5 && filename.compare(filename.size() - 5, 5, ".tzst") == 0) return CompressionType::TAR;
    if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".tar") == 0) return CompressionType::TAR;

    if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".zst") == 0) return CompressionType::ZSTD;
    if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".lz4") == 0) return CompressionType::LZ4;
    if (filename.size() >= 3 && filename.compare(filename.size() - 3, 3, ".gz") == 0) return CompressionType::GZIP;
    if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".bgz") == 0) return CompressionType::GZIP;
    if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".bz2") == 0) return CompressionType::BZIP2;
    if (filename.size() >= 3 && filename.compare(filename.size() - 3, 3, ".xz") == 0) return CompressionType::XZ;
    if (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".zip") == 0) return CompressionType::ZIP;
    return CompressionType::NONE;
}

DetectionResult detectCompressionType(const std::string& filename) {
    FilePtr file(std::fopen(filename.c_str(), "rb"));
    if (!file) {
        throw std::runtime_error("Failed to open file '" + filename + "': " + std::strerror(errno));
    }

    unsigned char bytes[6] = {0};
    size_t read = std::fread(bytes, 1, sizeof(bytes), file.get());

    CompressionType byExt = fallbackByExtension(filename);
    if (byExt == CompressionType::TAR) {
        return {CompressionType::TAR, std::move(file)};
    }

    CompressionType type = detectCompressionTypeFromBytes(bytes, read);
    if (type == CompressionType::NONE) {
        type = byExt;
    }

    return {type, std::move(file)};
}

void listZipEntries(const std::string& filename) {
    int error = 0;
    zip_t* za = zip_open(filename.c_str(), ZIP_RDONLY, &error);
    if (!za) {
        throw std::runtime_error("Failed to open zip file '" + filename + "' (error " + std::to_string(error) + ")");
    }
    zip_int64_t numEntries = zip_get_num_entries(za, 0);
    for (zip_int64_t i = 0; i < numEntries; ++i) {
        const char* name = zip_get_name(za, static_cast<zip_uint64_t>(i), 0);
        if (name) {
            zip_stat_t st;
            if (zip_stat_index(za, static_cast<zip_uint64_t>(i), 0, &st) == 0 && (st.valid & ZIP_STAT_SIZE)) {
                std::cout << st.size << '\t' << name << '\n';
            } else {
                std::cout << "?\t" << name << '\n';
            }
        }
    }
    zip_close(za);
}

void listTarEntries(const std::string& filename) {
    archive* ar = archive_read_new();
    if (!ar) {
        throw std::runtime_error("libarchive error while initializing reader for '" + filename + "'");
    }

    archive_read_support_filter_all(ar);
    archive_read_support_format_tar(ar);

    if (archive_read_open_filename(ar, filename.c_str(), 10240) != ARCHIVE_OK) {
        std::string err = archive_error_string(ar) ? archive_error_string(ar) : "unknown";
        archive_read_free(ar);
        throw std::runtime_error("libarchive error while opening '" + filename + "': " + err);
    }

    archive_entry* entry = nullptr;
    while (true) {
        int ret = archive_read_next_header(ar, &entry);
        if (ret == ARCHIVE_EOF) {
            break;
        }
        if (ret != ARCHIVE_OK) {
            std::string err = archive_error_string(ar) ? archive_error_string(ar) : "unknown";
            archive_read_free(ar);
            throw std::runtime_error("libarchive error while listing entries in '" + filename + "': " + err);
        }

        const char* name = archive_entry_pathname(entry);
        la_int64_t size = archive_entry_size(entry);
        if (name) {
            if (size >= 0) {
                std::cout << static_cast<unsigned long long>(size) << '\t' << name << '\n';
            } else {
                std::cout << "?\t" << name << '\n';
            }
        }
        archive_read_data_skip(ar);
    }

    archive_read_free(ar);
}
