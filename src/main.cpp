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
#if ZTAIL_USE_THREADS
#include <thread>
#include <mutex>
#include <condition_variable>
#endif

#ifndef ZTAIL_NO_MAIN
namespace {

template <typename Reader>
void processStream(Reader reader, Parser& parser, CircularBuffer& cb, size_t bufferSize) {
#if ZTAIL_USE_THREADS
    std::vector<char> buffers[2] = {
        std::vector<char>(bufferSize),
        std::vector<char>(bufferSize)
    };
    size_t sizes[2] = {0, 0};
    bool ready[2] = {false, false};
    bool finished = false;
    std::mutex m;
    std::condition_variable cv;

    std::thread producer([&]() {
        int idx = 0;
        size_t n = 0;
        while (reader(buffers[idx], n)) {
            if (n > 0) {
                {
                    std::unique_lock<std::mutex> lock(m);
                    sizes[idx] = n;
                    ready[idx] = true;
                }
                cv.notify_one();
                idx ^= 1;
            }
        }
        {
            std::unique_lock<std::mutex> lock(m);
            finished = true;
        }
        cv.notify_all();
    });

    std::thread consumer([&]() {
        int idx = 0;
        while (true) {
            std::unique_lock<std::mutex> lock(m);
            cv.wait(lock, [&] { return ready[idx] || finished; });
            if (ready[idx]) {
                size_t n = sizes[idx];
                ready[idx] = false;
                lock.unlock();
                parser.parse(buffers[idx].data(), n);
                idx ^= 1;
            } else if (finished) {
                break;
            }
        }
        parser.finalize();
        cb.print();
    });

    producer.join();
    consumer.join();
#else
    std::vector<char> buf(bufferSize);
    size_t n = 0;
    while (reader(buf, n)) {
        parser.parse(buf.data(), n);
    }
    parser.finalize();
    cb.print();
#endif
}

} // namespace
int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cout.tie(nullptr);
    try {
        CLIOptions options = CLI::parse(argc, argv);

        if (!options.filenames.empty()) {
            for (const auto& filename : options.filenames) {
                CircularBuffer cb(options.n, options.lineCapacity);
                Parser parser(cb, options.lineCapacity);

                DetectionResult det = detectCompressionType(filename);

                std::unique_ptr<ICompressor> comp;
                if (det.type == CompressionType::GZIP) {
                    comp = std::make_unique<CompressorZlib>(std::move(det.file), filename, options.zlibBufferSize);
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
                    processStream([
                        &](std::vector<char>& buf, size_t& n) {
                            return comp->decompress(buf, n);
                        },
                        parser, cb, options.readBufferSize);
                } else {
                    det.file.reset();
                    tailPlainFile(filename, parser, options.n, options.readBufferSize);
                    cb.print();
                }
            }
        } else {
            CircularBuffer cb(options.n, options.lineCapacity);
            Parser parser(cb, options.lineCapacity);
            processStream([
                &](std::vector<char>& buf, size_t& n) {
                    n = std::fread(buf.data(), 1, buf.size(), stdin);
                    return n > 0;
                },
                parser, cb, options.readBufferSize);
        }
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
#endif // ZTAIL_NO_MAIN
