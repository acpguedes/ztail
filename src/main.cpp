#include "circular_buffer.h"
#include "cli.h"
#include "parser.h"
#include "compressor_zlib.h"
#include "compressor_zip.h"
#include "compressor_bzip2.h"
#include "compressor_xz.h"
#include "compressor_zstd.h"
#include "icompressor.h"
#include "compression_type.h"
#include "tail_plain.h"

#include <iostream>
#include <stdexcept>
#include <cstdio>      // for fread
#include <vector>
#include <cstdlib>     // for EXIT_SUCCESS/EXIT_FAILURE

static const size_t READ_BUFFER_SIZE = 1 << 20; // 1MB

#ifndef ZTAIL_NO_MAIN
int main(int argc, char* argv[]) {
    try {
        CLIOptions options = CLI::parse(argc, argv);

        if (!options.filenames.empty()) {
            for (const auto& filename : options.filenames) {
                CircularBuffer cb(options.n);
                Parser parser(cb);
                std::vector<char> buffer(READ_BUFFER_SIZE);
                size_t bytesDecompressed = 0;

                DetectionResult det = detectCompressionType(filename);

                std::unique_ptr<ICompressor> comp;
                if (det.type == CompressionType::GZIP) {
                    comp = std::make_unique<CompressorZlib>(std::move(det.file), filename);
                } else if (det.type == CompressionType::BZIP2) {
                    comp = std::make_unique<CompressorBzip2>(std::move(det.file), filename);
                } else if (det.type == CompressionType::XZ) {
                    comp = std::make_unique<CompressorXz>(std::move(det.file), filename);
                } else if (det.type == CompressionType::ZIP) {
                    comp = std::make_unique<CompressorZip>(std::move(det.file), filename, options.zipEntry);
                } else if (det.type == CompressionType::ZSTD) {
                    comp = std::make_unique<CompressorZstd>(std::move(det.file), filename);
                }

                if (comp) {
                    while (comp->decompress(buffer, bytesDecompressed)) {
                        if (bytesDecompressed)
                            parser.parse(buffer.data(), bytesDecompressed);
                    }
                    parser.finalize();
                } else {
                    det.file.reset();
                    tailPlainFile(filename, parser, options.n, READ_BUFFER_SIZE);
                }

                cb.print();
            }
        } else {
            CircularBuffer cb(options.n);
            Parser parser(cb);
            std::vector<char> buffer(READ_BUFFER_SIZE);
            while (true) {
                size_t bytesRead = std::fread(buffer.data(), 1, buffer.size(), stdin);
                if (bytesRead == 0) break;
                parser.parse(buffer.data(), bytesRead);
            }
            parser.finalize();
            cb.print();
        }
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
#endif // ZTAIL_NO_MAIN
