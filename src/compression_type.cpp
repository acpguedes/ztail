#include "compression_type.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <zip.h>
#ifdef ZTAIL_HAS_TAR
#include <archive.h>
#include <archive_entry.h>
#endif

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

const char* compressionTypeName(CompressionType type) {
    switch (type) {
    case CompressionType::GZIP:  return "gzip";
    case CompressionType::BZIP2: return "bzip2";
    case CompressionType::XZ:    return "xz";
    case CompressionType::ZSTD:  return "zstd";
    case CompressionType::ZIP:   return "zip";
    case CompressionType::LZ4:   return "lz4";
    case CompressionType::TAR:   return "tar";
    case CompressionType::NONE:  return "plain";
    }
    return "unknown";
}

static bool endsWith(const std::string& s, const char* suffix, size_t suffixLen) {
    return s.size() >= suffixLen && s.compare(s.size() - suffixLen, suffixLen, suffix) == 0;
}

static CompressionType fallbackByExtension(const std::string& filename) {
#ifdef ZTAIL_HAS_TAR
    // TAR compound extensions must be checked before simple ones
    if (endsWith(filename, ".tar.gz", 7))  return CompressionType::TAR;
    if (endsWith(filename, ".tar.bz2", 8)) return CompressionType::TAR;
    if (endsWith(filename, ".tar.xz", 7))  return CompressionType::TAR;
    if (endsWith(filename, ".tar.zst", 8)) return CompressionType::TAR;
    if (endsWith(filename, ".tar.lz4", 8)) return CompressionType::TAR;
    if (endsWith(filename, ".tgz", 4))     return CompressionType::TAR;
    if (endsWith(filename, ".tbz2", 5))    return CompressionType::TAR;
    if (endsWith(filename, ".tb2", 4))     return CompressionType::TAR;
    if (endsWith(filename, ".txz", 4))     return CompressionType::TAR;
    if (endsWith(filename, ".tzst", 5))    return CompressionType::TAR;
    if (endsWith(filename, ".taz", 4))     return CompressionType::TAR;
    if (endsWith(filename, ".tlz4", 5))    return CompressionType::TAR;
    if (endsWith(filename, ".tar", 4))     return CompressionType::TAR;
#endif

    if (endsWith(filename, ".zst", 4))  return CompressionType::ZSTD;
    if (endsWith(filename, ".lz4", 4))  return CompressionType::LZ4;
    if (endsWith(filename, ".gz", 3))   return CompressionType::GZIP;
    if (endsWith(filename, ".bgz", 4))  return CompressionType::GZIP;
    if (endsWith(filename, ".bgzf", 5)) return CompressionType::GZIP;
    if (endsWith(filename, ".bz2", 4))  return CompressionType::BZIP2;
    if (endsWith(filename, ".xz", 3))   return CompressionType::XZ;
    if (endsWith(filename, ".zip", 4))  return CompressionType::ZIP;
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
#ifdef ZTAIL_HAS_TAR
    if (byExt == CompressionType::TAR) {
        return {CompressionType::TAR, std::move(file)};
    }
#endif

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

#ifdef ZTAIL_HAS_TAR
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
#else
void listTarEntries(const std::string& filename) {
    throw std::runtime_error("TAR support is not available in this build (libarchive not found): " + filename);
}
#endif
