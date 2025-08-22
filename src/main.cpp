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
#include <fstream>
#include <string>
#if ZTAIL_USE_THREADS
#include <thread>
#include <mutex>
#include <condition_variable>
#endif

#ifndef ZTAIL_NO_MAIN
namespace {

template <typename Reader>
void processStream(Reader reader, Parser& parser, CircularBuffer& cb,
                   size_t bufferSize, size_t aggregationThreshold, bool useThreads) {
#if ZTAIL_USE_THREADS
    auto memoryLimited = [](size_t bs) {
#if defined(__linux__)
        std::ifstream meminfo("/proc/meminfo");
        std::string key, unit;
        size_t value = 0;
        while (meminfo >> key >> value >> unit) {
            if (key == "MemAvailable:") {
                size_t avail = value * 1024; // value in kB
                return bs * 2 > avail / 2; // reserve at most half of available memory
            }
        }
#endif
        return false;
    };

    bool threaded = useThreads && !memoryLimited(bufferSize);
    if (threaded) {
        size_t currentSize = bufferSize;
        const size_t minSize = 64 * 1024;
        int idleCount = 0;

        std::vector<char> buffers[2] = {
            std::vector<char>(currentSize),
            std::vector<char>(currentSize)
        };
        size_t sizes[2] = {0, 0};
        bool ready[2] = {false, false};
        bool finished = false;
        std::mutex m;
        std::condition_variable cv;

        std::thread producer([&]() {
            int idx = 0;
            size_t n = 0;
            while (true) {
                {
                    std::unique_lock<std::mutex> lock(m);
                    if (!ready[0] && !ready[1]) {
                        ++idleCount;
                        if (idleCount >= 3 && currentSize > minSize) {
                            currentSize /= 2;
                            for (auto &b : buffers) {
                                std::vector<char>(currentSize).swap(b);
                            }
                            idleCount = 0;
                        }
                    } else {
                        idleCount = 0;
                    }
                }
                if (!reader(buffers[idx], n)) {
                    break;
                }
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
            cb.print(aggregationThreshold);
        });

        producer.join();
        consumer.join();
    } else {
        std::vector<char> buf(bufferSize);
        size_t n = 0;
        while (reader(buf, n)) {
            parser.parse(buf.data(), n);
        }
        parser.finalize();
        cb.print(aggregationThreshold);
    }
#else
    (void)useThreads;
    std::vector<char> buf(bufferSize);
    size_t n = 0;
    while (reader(buf, n)) {
        parser.parse(buf.data(), n);
    }
    parser.finalize();
    cb.print(aggregationThreshold);
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
                CircularBuffer cb(options.n, options.lineCapacity, options.bytesBudget);
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
                        parser, cb, options.readBufferSize, options.printAggregationThreshold, options.useThreads);
                } else {
                    det.file.reset();
                    tailPlainFile(filename, parser, options.n, options.readBufferSize);
                    cb.print(options.printAggregationThreshold);
                }
            }
        } else {
            CircularBuffer cb(options.n, options.lineCapacity, options.bytesBudget);
            Parser parser(cb, options.lineCapacity);
            processStream([
                &](std::vector<char>& buf, size_t& n) {
                    n = std::fread(buf.data(), 1, buf.size(), stdin);
                    return n > 0;
                },
                parser, cb, options.readBufferSize, options.printAggregationThreshold, options.useThreads);
        }
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
#endif // ZTAIL_NO_MAIN
