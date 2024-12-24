#include "circular_buffer.h"
#include "cli.h"
#include "parser.h"
#include "compressor_zlib.h"
#include "compressor_zip.h"

#include <iostream>
#include <stdexcept>
#include <cstdio>      // for fread
#include <vector>
#include <cstdlib>     // for EXIT_SUCCESS/EXIT_FAILURE

static const size_t READ_BUFFER_SIZE = 1 << 20; // 1MB

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
            bool isZip = false;

            // Check the file extension
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
            else {
                std::cerr << "Unrecognized extension in \"" << filename
                          << "\". Only .gz, .bgz, and .zip are supported.\n";
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
