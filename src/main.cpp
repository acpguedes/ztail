#include "circular_buffer.h"
#include "cli.h"
#include "parser.h"
#include "compressor_zlib.h"
#include "compressor_zip.h"
#include "compressor_bzip2.h"
#include "compression_type.h"

#include <iostream>
#include <stdexcept>
#include <cstdio>      // for fread
#include <vector>
#include <cstdlib>     // for EXIT_SUCCESS/EXIT_FAILURE
#include <fstream>

// Detect the compression type by inspecting the file's magic bytes
CompressionType detectCompressionType(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return CompressionType::NONE;
    }

    unsigned char bytes[6] = {0};
    file.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
    std::size_t read = static_cast<std::size_t>(file.gcount());

    if (read >= 2 && bytes[0] == 0x1f && bytes[1] == 0x8b) {
        return CompressionType::GZIP;      // gzip or bgz
    }
    if (read >= 3 && bytes[0] == 'B' && bytes[1] == 'Z' && bytes[2] == 'h') {
        return CompressionType::BZIP2;     // bzip2
    }
    if (read >= 6 && bytes[0] == 0xFD && bytes[1] == 0x37 && bytes[2] == 0x7A &&
        bytes[3] == 0x58 && bytes[4] == 0x5A && bytes[5] == 0x00) {
        return CompressionType::XZ;        // xz
    }
    if (read >= 4 && bytes[0] == 0x50 && bytes[1] == 0x4B &&
        (bytes[2] == 0x03 || bytes[2] == 0x05 || bytes[2] == 0x07) &&
        (bytes[3] == 0x04 || bytes[3] == 0x06 || bytes[3] == 0x08)) {
        return CompressionType::ZIP;       // zip
    }

    return CompressionType::NONE;
}

static const size_t READ_BUFFER_SIZE = 1 << 20; // 1MB

#ifndef ZTAIL_NO_MAIN
int main(int argc, char* argv[]) {
    // Parse command-line arguments
    CLIOptions options = CLI::parse(argc, argv);

    // Create a circular buffer that holds up to N lines
    CircularBuffer cb(options.n);

    try {
        // If a filename was provided, open that file
        if (!options.filename.empty()) {
            std::string filename = options.filename;

            bool isGz  = false;
            bool isBgz = false;
            bool isBz2 = false;
            bool isZip = false;
            bool isXz  = false;

            // First attempt to detect by magic bytes
            CompressionType ctype = detectCompressionType(filename);
            if (ctype == CompressionType::GZIP) {
                isGz = true;
            }
            else if (ctype == CompressionType::BZIP2) {
                isBz2 = true;
            }
            else if (ctype == CompressionType::ZIP) {
                isZip = true;
            }
            else if (ctype == CompressionType::XZ) {
                isXz = true;
            }

            // Fallback to extension check if detection failed
            if (ctype == CompressionType::NONE) {
                if (filename.size() >= 3 &&
                    (filename.compare(filename.size() - 3, 3, ".gz") == 0)) {
                    isGz = true;
                }
                else if (filename.size() >= 4 &&
                         (filename.compare(filename.size() - 4, 4, ".bgz") == 0)) {
                    isBgz = true;
                }
                else if (filename.size() >= 4 &&
                         (filename.compare(filename.size() - 4, 4, ".zip") == 0)) {
                    isZip = true;
                }
                else if (filename.size() >= 4 &&
                         (filename.compare(filename.size() - 4, 4, ".bz2") == 0)) {
                    isBz2 = true;
                }
                else {
                    std::cerr << "Unrecognized extension in \"" << filename
                              << "\". Only .gz, .bgz, .bz2, and .zip are supported.\n";
                    return EXIT_FAILURE;
                }
            }

            if (isXz) {
                std::cerr << "Compression type xz is not supported: " << filename << "\n";
                return EXIT_FAILURE;
            }

            Parser parser(cb);
            std::vector<char> decompressedBuffer(READ_BUFFER_SIZE);
            size_t bytesDecompressed = 0;

            if (isGz || isBgz) {
                // Use zlib
                CompressorZlib compressor(filename);

                while (compressor.decompress(decompressedBuffer, bytesDecompressed)) {
                    if (bytesDecompressed > 0) {
                        parser.parse(decompressedBuffer.data(), bytesDecompressed);
                    }
                }
                parser.finalize();
            }
            else if (isBz2) {
                // Use bzip2
                CompressorBzip2 compressor(filename);

                while (compressor.decompress(decompressedBuffer, bytesDecompressed)) {
                    if (bytesDecompressed > 0) {
                        parser.parse(decompressedBuffer.data(), bytesDecompressed);
                    }
                }
                parser.finalize();
            }
            else if (isZip) {
                // Use libzip
                CompressorZip compressor(filename);

                while (compressor.decompress(decompressedBuffer, bytesDecompressed)) {
                    if (bytesDecompressed > 0) {
                        parser.parse(decompressedBuffer.data(), bytesDecompressed);
                    }
                }
                parser.finalize();
            }
        } 
        else {
            // If no file is provided, read from stdin
            Parser parser(cb);
            std::vector<char> buffer(READ_BUFFER_SIZE);

            while (true) {
                size_t bytesRead = std::fread(buffer.data(), 1, buffer.size(), stdin);
                if (bytesRead == 0) {
                    break;
                }
                parser.parse(buffer.data(), bytesRead);
            }
            parser.finalize();
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    // Print the lines stored in the circular buffer (last N lines)
    cb.print();

    return EXIT_SUCCESS;
}
#endif // ZTAIL_NO_MAIN
