#include "cli.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <cerrno>
#include <limits>
#include <stdexcept>

void CLI::usage(const char* progName) {
    std::cerr
        << "Usage: " << progName << " [options] <files...>\n"
        << "  -n, --lines N   : print the last N lines (default = 10)\n"
        << "  -c, --line-capacity N : pre-reserve N bytes for each line\n"
        << "      --bytes-budget N : limit total bytes stored for lines\n"
        << "  -b, --zlib-buffer N : set zlib buffer size in bytes (default = 1048576)\n"
        << "      --xz-buffer N   : set xz buffer size in bytes (default = 32768)\n"
        << "      --zstd-window N : set max zstd window size in bytes (default = unlimited)\n"
        << "  -r, --read-buffer N : set read buffer size in bytes (default = 1048576)\n"
        << "  -e, --entry <name> : entry name inside zip archive\n"
        << "      --print-aggregation-threshold N : threshold in bytes for aggregated output (default = 8388608)\n"
        << "      --no-threads   : disable producer/consumer threading\n"
        << "  -V, --version  : display program version and exit\n"
        << "  -h, --help     : display this help and exit\n"
        << "If no file is provided, the program reads from stdin.\n"
        << "Compression type is detected automatically.\n"
        << "CharRingBuffer backend is enabled by default. Build with -DUSE_CHAR_RING_BUFFER=OFF to\n"
           "use the std::string-based buffer for debugging.\n";
}

CLIOptions CLI::parse(int argc, char* argv[]) {
    CLIOptions options;

    static struct option long_opts[] = {
        {"help",          no_argument,       nullptr, 'h'},
        {"version",       no_argument,       nullptr, 'V'},
        {"lines",         required_argument, nullptr, 'n'},
        {"line-capacity", required_argument, nullptr, 'c'},
        {"bytes-budget", required_argument, nullptr, 1001},
        {"zlib-buffer",   required_argument, nullptr, 'b'},
        {"xz-buffer",     required_argument, nullptr, 1003},
        {"zstd-window",   required_argument, nullptr, 1004},
        {"read-buffer",   required_argument, nullptr, 'r'},
        {"entry",         required_argument, nullptr, 'e'},
        {"print-aggregation-threshold", required_argument, nullptr, 1000},
        {"no-threads",    no_argument,       nullptr, 1002},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "hn:c:b:r:e:V", long_opts, nullptr)) != -1) {
        switch (opt) {
        case 'h':
            CLI::usage(argv[0]);
            std::exit(EXIT_SUCCESS);
        case 'V':
            std::cout << argv[0] << " version " << ZTAIL_VERSION << std::endl;
            std::exit(EXIT_SUCCESS);
        case 'n': {
            char* end = nullptr;
            errno = 0;
            long val = std::strtol(optarg, &end, 10);
            if (errno != 0 || end == optarg || *end != '\0' ||
                val <= 0 || val > std::numeric_limits<int>::max()) {
                throw std::runtime_error("-n/--lines requires a positive integer");
            }
            options.n = static_cast<int>(val);
            break;
        }
        case 'c': {
            char* end = nullptr;
            errno = 0;
            long val = std::strtol(optarg, &end, 10);
            if (errno != 0 || end == optarg || *end != '\0' || val < 0) {
                throw std::runtime_error("-c/--line-capacity requires a non-negative integer");
            }
            options.lineCapacity = static_cast<size_t>(val);
            break;
        }
        case 'b': {
            char* end = nullptr;
            errno = 0;
            long val = std::strtol(optarg, &end, 10);
            if (errno != 0 || end == optarg || *end != '\0' || val <= 0 ||
                val > std::numeric_limits<unsigned int>::max()) {
                throw std::runtime_error("-b/--zlib-buffer requires a positive integer not exceeding " +
                                         std::to_string(std::numeric_limits<unsigned int>::max()));
            }
            options.zlibBufferSize = static_cast<size_t>(val);
            break;
        }
        case 1003: {
            char* end = nullptr;
            errno = 0;
            long val = std::strtol(optarg, &end, 10);
            if (errno != 0 || end == optarg || *end != '\0' || val <= 0) {
                throw std::runtime_error("--xz-buffer requires a positive integer");
            }
            options.xzBufferSize = static_cast<size_t>(val);
            break;
        }
        case 1004: {
            char* end = nullptr;
            errno = 0;
            long val = std::strtol(optarg, &end, 10);
            if (errno != 0 || end == optarg || *end != '\0' || val < 0) {
                throw std::runtime_error("--zstd-window requires a non-negative integer");
            }
            options.zstdWindowSize = static_cast<size_t>(val);
            break;
        }
        case 'r': {
            char* end = nullptr;
            errno = 0;
            long val = std::strtol(optarg, &end, 10);
            if (errno != 0 || end == optarg || *end != '\0' || val <= 0) {
                throw std::runtime_error("-r/--read-buffer requires a positive integer");
            }
            options.readBufferSize = static_cast<size_t>(val);
            break;
        }
        case 'e':
            options.zipEntry = optarg;
            break;
        case 1000: {
            char* end = nullptr;
            errno = 0;
            long val = std::strtol(optarg, &end, 10);
            if (errno != 0 || end == optarg || *end != '\0' || val < 0) {
                throw std::runtime_error("--print-aggregation-threshold requires a non-negative integer");
            }
            options.printAggregationThreshold = static_cast<size_t>(val);
            break;
        }
        case 1001: {
            char* end = nullptr;
            errno = 0;
            long val = std::strtol(optarg, &end, 10);
            if (errno != 0 || end == optarg || *end != '\0' || val < 0) {
                throw std::runtime_error("--bytes-budget requires a non-negative integer");
            }
            options.bytesBudget = static_cast<size_t>(val);
            break;
        }
        case 1002:
            options.useThreads = false;
            break;
        case '?':
        default:
            throw std::runtime_error("Unknown option");
        }
    }

    for (int index = optind; index < argc; ++index) {
        options.filenames.push_back(argv[index]);
    }

    return options;
}
