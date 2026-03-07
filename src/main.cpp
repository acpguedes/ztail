#include "circular_buffer.h"
#include "cli.h"
#include "parser.h"
#include "compressor_zlib.h"
#include "compressor_zip.h"
#include "compressor_bzip2.h"
#include "compressor_xz.h"
#include "compressor_zstd.h"
#ifdef ZTAIL_HAS_LZ4
#include "compressor_lz4.h"
#endif
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
#include <functional>
#include <unordered_map>
#if ZTAIL_USE_THREADS
#include <thread>
#include <mutex>
#include <condition_variable>
#endif
#if defined(__linux__)
#include <fcntl.h>
#endif

#ifndef ZTAIL_NO_MAIN
namespace {

#if defined(__linux__)
void adviseSequential(FILE* f) {
    if (f) {
        int fd = fileno(f);
        if (fd >= 0) {
            posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
        }
    }
}
#else
void adviseSequential(FILE*) {}
#endif

using CompressorFactory = std::function<std::unique_ptr<ICompressor>(
    FilePtr&&, const std::string&, const CLIOptions&)>;

std::unordered_map<CompressionType, CompressorFactory> buildFactoryMap() {
    std::unordered_map<CompressionType, CompressorFactory> map;
    map[CompressionType::GZIP] = [](FilePtr&& f, const std::string& name, const CLIOptions& opts) {
        return std::make_unique<CompressorZlib>(std::move(f), name, opts.zlibBufferSize);
    };
    map[CompressionType::BZIP2] = [](FilePtr&& f, const std::string& name, const CLIOptions&) {
        return std::make_unique<CompressorBzip2>(std::move(f), name);
    };
    map[CompressionType::XZ] = [](FilePtr&& f, const std::string& name, const CLIOptions& opts) {
        return std::make_unique<CompressorXz>(std::move(f), name, opts.xzBufferSize, opts.xzMemLimit);
    };
    map[CompressionType::ZIP] = [](FilePtr&& f, const std::string& name, const CLIOptions& opts) {
        return std::make_unique<CompressorZip>(std::move(f), name, opts.zipEntry);
    };
    map[CompressionType::ZSTD] = [](FilePtr&& f, const std::string& name, const CLIOptions& opts) {
        return std::make_unique<CompressorZstd>(std::move(f), name, opts.zstdWindowSize);
    };
#ifdef ZTAIL_HAS_LZ4
    map[CompressionType::LZ4] = [](FilePtr&& f, const std::string& name, const CLIOptions&) {
        return std::make_unique<CompressorLz4>(std::move(f), name);
    };
#endif
    return map;
}

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
                    // Wait until the current buffer slot is consumed
                    cv.wait(lock, [&] { return !ready[idx]; });
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
                    cv.notify_one(); // wake producer waiting on this slot
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
        if (options.lineCapacity == 0) {
            std::cerr << "WARNING: line capacity is 0; no buffer will be preallocated for lines" << std::endl;
        }

        auto factories = buildFactoryMap();
        const bool multiFile = options.filenames.size() > 1;

        if (!options.filenames.empty()) {
            for (size_t fi = 0; fi < options.filenames.size(); ++fi) {
                const auto& filename = options.filenames[fi];
                if (multiFile) {
                    if (fi > 0) std::cout << '\n';
                    std::cout << "==> " << filename << " <==" << '\n';
                }

                CircularBuffer cb(options.n, options.lineCapacity, options.bytesBudget);
                Parser parser(cb, options.lineCapacity);

                // List zip entries mode
                if (options.listEntries) {
                    listZipEntries(filename);
                    continue;
                }

                CompressionType ctype;
                FilePtr file(nullptr);

                if (options.forceFormat != CompressionType::NONE) {
                    file = FilePtr(std::fopen(filename.c_str(), "rb"));
                    if (!file) {
                        throw std::runtime_error("Failed to open file: " + filename);
                    }
                    ctype = options.forceFormat;
                } else {
                    DetectionResult det = detectCompressionType(filename);
                    ctype = det.type;
                    file = std::move(det.file);
                }

                auto it = factories.find(ctype);
                if (it != factories.end()) {
                    adviseSequential(file.get());
                    auto comp = it->second(std::move(file), filename, options);
                    processStream([
                        &](std::vector<char>& buf, size_t& n) {
                            return comp->decompress(buf, n);
                        },
                        parser, cb, options.readBufferSize, options.printAggregationThreshold, options.useThreads);
                } else {
                    file.reset();
                    tailPlainFile(filename, parser, options.n, options.readBufferSize);
                    cb.print(options.printAggregationThreshold);
                }
            }
        } else {
            // Stdin: detect compression from first bytes
            CircularBuffer cb(options.n, options.lineCapacity, options.bytesBudget);
            Parser parser(cb, options.lineCapacity);

            unsigned char header[6] = {0};
            size_t headerRead = std::fread(header, 1, sizeof(header), stdin);
            CompressionType stdinType = options.forceFormat;

            if (stdinType == CompressionType::NONE && headerRead > 0) {
                stdinType = detectCompressionTypeFromBytes(header, headerRead);
            }

            if (stdinType != CompressionType::NONE) {
                // Write header bytes to a temp file, then stream stdin through it
                char tmpname[] = "/tmp/ztail_stdin_XXXXXX";
                int tmpfd = mkstemp(tmpname);
                if (tmpfd < 0) {
                    throw std::runtime_error("Failed to create temporary file for stdin");
                }
                FILE* tmpf = fdopen(tmpfd, "w+b");
                if (!tmpf) {
                    close(tmpfd);
                    unlink(tmpname);
                    throw std::runtime_error("Failed to open temporary file for stdin");
                }
                std::fwrite(header, 1, headerRead, tmpf);
                // Copy remaining stdin
                std::vector<char> copyBuf(options.readBufferSize);
                size_t nr;
                while ((nr = std::fread(copyBuf.data(), 1, copyBuf.size(), stdin)) > 0) {
                    std::fwrite(copyBuf.data(), 1, nr, tmpf);
                }
                std::fflush(tmpf);
                std::fseek(tmpf, 0, SEEK_SET);

                FilePtr fp(tmpf);
                std::string tmpFilename(tmpname);
                auto it = factories.find(stdinType);
                if (it != factories.end()) {
                    auto comp = it->second(std::move(fp), tmpFilename, options);
                    processStream([&](std::vector<char>& buf, size_t& n) {
                        return comp->decompress(buf, n);
                    }, parser, cb, options.readBufferSize, options.printAggregationThreshold, options.useThreads);
                }
                std::remove(tmpname);
            } else {
                // Plain text stdin — feed header bytes first, then stream
                if (headerRead > 0) {
                    parser.parse(reinterpret_cast<const char*>(header), headerRead);
                }
                processStream([&](std::vector<char>& buf, size_t& n) {
                    n = std::fread(buf.data(), 1, buf.size(), stdin);
                    return n > 0;
                }, parser, cb, options.readBufferSize, options.printAggregationThreshold, options.useThreads);
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
#endif // ZTAIL_NO_MAIN
