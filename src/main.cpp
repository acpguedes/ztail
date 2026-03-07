#include "circular_buffer.h"
#include "cli.h"
#include "parser.h"
#include "compressor_factory.h"
#include "icompressor.h"
#include "compression_type.h"
#include "tail_plain.h"

#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <string>
#include <functional>
#include <unordered_map>
#include <exception>
#if ZTAIL_USE_THREADS
#include <thread>
#include <mutex>
#include <condition_variable>
#endif
#if defined(__linux__)
#include <fcntl.h>
#endif
#include <unistd.h>

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

void streamToFd(int fd, const unsigned char* header, size_t headerSize, size_t chunkSize) {
    auto writeAll = [fd](const char* data, size_t len) {
        size_t written = 0;
        while (written < len) {
            ssize_t w = ::write(fd, data + written, len - written);
            if (w <= 0) {
                throw std::runtime_error("Failed writing data to streaming pipe");
            }
            written += static_cast<size_t>(w);
        }
    };

    if (header && headerSize > 0) {
        writeAll(reinterpret_cast<const char*>(header), headerSize);
    }

    std::vector<char> copyBuf(chunkSize);
    size_t nr = 0;
    while ((nr = std::fread(copyBuf.data(), 1, copyBuf.size(), stdin)) > 0) {
        writeAll(copyBuf.data(), nr);
    }
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
                size_t avail = value * 1024;
                return bs * 2 > avail / 2;
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
                    cv.wait(lock, [&] { return !ready[idx]; });
                    if (!ready[0] && !ready[1]) {
                        ++idleCount;
                        if (idleCount >= 3 && currentSize > minSize) {
                            currentSize /= 2;
                            // Only resize the buffer we are about to use (idx),
                            // the other may still be in use by the consumer.
                            std::vector<char>(currentSize).swap(buffers[idx]);
                            idleCount = 0;
                        }
                    } else {
                        idleCount = 0;
                    }

                    if ((ready[0] || ready[1]) && currentSize < bufferSize) {
                        size_t grown = std::min(bufferSize, currentSize * 2);
                        if (grown != currentSize) {
                            currentSize = grown;
                            // Only resize the buffer we are about to use.
                            buffers[idx].resize(currentSize);
                        }
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
                    cv.notify_one();
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

    // Use full buffering on stdout for better I/O throughput with large output
    static char stdoutBuf[65536];
    std::setvbuf(stdout, stdoutBuf, _IOFBF, sizeof(stdoutBuf));

    try {
        CLIOptions options = CLI::parse(argc, argv);
        if (options.lineCapacity == 0) {
            std::cerr << "WARNING: line capacity is 0; no buffer will be preallocated for lines" << std::endl;
        }

        auto factories = buildCompressorFactoryMap();
        const bool multiFile = options.filenames.size() > 1;

        if (!options.filenames.empty()) {
            for (size_t fi = 0; fi < options.filenames.size(); ++fi) {
                const auto& filename = options.filenames[fi];
                if ((multiFile && options.multiFileHeaders) || options.alwaysShowHeaders) {
                    if (fi > 0) std::cout << '\n';
                    std::cout << "==> " << filename << " <==" << '\n';
                }

                CircularBuffer cb(options.n, options.lineCapacity, options.bytesBudget);
                Parser parser(cb, options.lineCapacity);

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

                if (options.listEntries) {
                    if (ctype == CompressionType::ZIP) {
                        listZipEntries(filename);
#ifdef ZTAIL_HAS_TAR
                    } else if (ctype == CompressionType::TAR) {
                        listTarEntries(filename);
                    } else {
#else
                    } else {
#endif
                        throw std::runtime_error("--list-entries is only supported for zip/tar inputs");
                    }
                    continue;
                }

                auto it = factories.find(ctype);
                if (it != factories.end()) {
                    adviseSequential(file.get());
                    auto comp = it->second(std::move(file), filename, options);
                    processStream([&](std::vector<char>& buf, size_t& n) {
                        return comp->decompress(buf, n);
                    }, parser, cb, options.readBufferSize, options.printAggregationThreshold, options.useThreads);
                } else {
                    file.reset();
                    tailPlainFile(filename, parser, options.n, options.readBufferSize);
                    cb.print(options.printAggregationThreshold);
                }
            }
        } else {
            CircularBuffer cb(options.n, options.lineCapacity, options.bytesBudget);
            Parser parser(cb, options.lineCapacity);

            unsigned char header[6] = {0};
            size_t headerRead = std::fread(header, 1, sizeof(header), stdin);
            CompressionType stdinType = options.forceFormat;

            if (stdinType == CompressionType::NONE && headerRead > 0) {
                stdinType = detectCompressionTypeFromBytes(header, headerRead);
            }

            if (stdinType != CompressionType::NONE) {
#if ZTAIL_USE_THREADS
                int fds[2];
                if (pipe(fds) != 0) {
                    throw std::runtime_error("Failed to create streaming pipe for stdin");
                }

                std::exception_ptr writerException = nullptr;
                std::thread writer([&]() {
                    try {
                        ::close(fds[0]);
                        streamToFd(fds[1], header, headerRead, options.readBufferSize);
                    } catch (...) {
                        writerException = std::current_exception();
                    }
                    ::close(fds[1]);
                });

                FILE* readFile = fdopen(fds[0], "rb");
                if (!readFile) {
                    ::close(fds[0]);
                    ::close(fds[1]);
                    writer.join();
                    throw std::runtime_error("Failed to open streaming reader for stdin");
                }

                FilePtr fp(readFile);
                auto it = factories.find(stdinType);
                if (it != factories.end()) {
                    auto comp = it->second(std::move(fp), "<stdin>", options);
                    processStream([&](std::vector<char>& buf, size_t& n) {
                        return comp->decompress(buf, n);
                    }, parser, cb, options.readBufferSize, options.printAggregationThreshold, options.useThreads);
                }

                writer.join();
                if (writerException) {
                    std::rethrow_exception(writerException);
                }
#else
                // Fallback when built without threads: stage stdin into an anonymous temp file.
                FILE* tmpf = std::tmpfile();
                if (!tmpf) {
                    throw std::runtime_error("Failed to create temporary file for stdin");
                }
                std::fwrite(header, 1, headerRead, tmpf);
                std::vector<char> copyBuf(options.readBufferSize);
                size_t nr = 0;
                while ((nr = std::fread(copyBuf.data(), 1, copyBuf.size(), stdin)) > 0) {
                    std::fwrite(copyBuf.data(), 1, nr, tmpf);
                }
                std::fflush(tmpf);
                std::fseek(tmpf, 0, SEEK_SET);

                FilePtr fp(tmpf);
                auto it = factories.find(stdinType);
                if (it != factories.end()) {
                    auto comp = it->second(std::move(fp), "<stdin>", options);
                    processStream([&](std::vector<char>& buf, size_t& n) {
                        return comp->decompress(buf, n);
                    }, parser, cb, options.readBufferSize, options.printAggregationThreshold, options.useThreads);
                }
#endif
            } else {
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
