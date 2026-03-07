#include "cli.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <cerrno>
#include <limits>
#include <stdexcept>

namespace {

size_t parsePositiveSize(const char* optarg, const char* optName) {
    char* end = nullptr;
    errno = 0;
    unsigned long long val = std::strtoull(optarg, &end, 10);
    if (errno != 0 || end == optarg || *end != '\0' || val == 0) {
        throw std::runtime_error(std::string(optName) + " requires a positive integer");
    }
    return static_cast<size_t>(val);
}

size_t parseNonNegativeSize(const char* optarg, const char* optName) {
    char* end = nullptr;
    errno = 0;
    unsigned long long val = std::strtoull(optarg, &end, 10);
    if (errno != 0 || end == optarg || *end != '\0') {
        throw std::runtime_error(std::string(optName) + " requires a non-negative integer");
    }
    return static_cast<size_t>(val);
}

CompressionType parseFormat(const char* s) {
    if (strcmp(s, "gzip") == 0 || strcmp(s, "gz") == 0) return CompressionType::GZIP;
    if (strcmp(s, "bzip2") == 0 || strcmp(s, "bz2") == 0) return CompressionType::BZIP2;
    if (strcmp(s, "xz") == 0) return CompressionType::XZ;
    if (strcmp(s, "zstd") == 0 || strcmp(s, "zst") == 0) return CompressionType::ZSTD;
    if (strcmp(s, "zip") == 0) return CompressionType::ZIP;
#ifdef ZTAIL_HAS_LZ4
    if (strcmp(s, "lz4") == 0) return CompressionType::LZ4;
#endif
    if (strcmp(s, "plain") == 0 || strcmp(s, "none") == 0) return CompressionType::NONE;
    throw std::runtime_error(std::string("Unknown format: ") + s);
}

} // namespace

void CLI::usage(const char* progName) {
    std::cerr
        << "Usage: " << progName << " [options] <files...>\n"
        << "  -n, --lines N   : print the last N lines (default = 10)\n"
        << "  -c, --line-capacity N : pre-reserve N bytes for each line (default = 512)\n"
        << "      --bytes-budget N : limit total bytes stored for lines\n"
        << "  -b, --zlib-buffer N : set zlib buffer size in bytes (default = 1048576)\n"
        << "      --xz-buffer N   : set xz buffer size in bytes (default = 131072)\n"
        << "      --xz-memlimit N : set xz decoder memory limit in bytes (default = 268435456)\n"
        << "      --zstd-window N : set max zstd window size in bytes (default = unlimited)\n"
        << "  -r, --read-buffer N : set read buffer size in bytes (default = 1048576)\n"
        << "  -e, --entry <name> : entry name inside zip archive\n"
        << "      --list-entries  : list entries in zip archive and exit\n"
        << "  -f, --format <fmt> : force compression format (gzip|bzip2|xz|zstd|zip"
#ifdef ZTAIL_HAS_LZ4
        << "|lz4"
#endif
        << "|plain)\n"
        << "      --print-aggregation-threshold N : threshold in bytes for aggregated output (default = 8388608)\n"
        << "      --no-threads   : disable producer/consumer threading\n"
        << "  -V, --version  : display program version and exit\n"
        << "  -h, --help     : display this help and exit\n"
        << "If no file is provided, the program reads from stdin.\n"
        << "Compression type is detected automatically from magic bytes.\n";
}

CLIOptions CLI::parse(int argc, char* argv[]) {
    CLIOptions options;

    static struct option long_opts[] = {
        {"help",          no_argument,       nullptr, 'h'},
        {"version",       no_argument,       nullptr, 'V'},
        {"lines",         required_argument, nullptr, 'n'},
        {"line-capacity", required_argument, nullptr, 'c'},
        {"bytes-budget",  required_argument, nullptr, 1001},
        {"zlib-buffer",   required_argument, nullptr, 'b'},
        {"xz-buffer",     required_argument, nullptr, 1003},
        {"xz-memlimit",   required_argument, nullptr, 1005},
        {"zstd-window",   required_argument, nullptr, 1004},
        {"read-buffer",   required_argument, nullptr, 'r'},
        {"entry",         required_argument, nullptr, 'e'},
        {"list-entries",  no_argument,       nullptr, 1006},
        {"format",        required_argument, nullptr, 'f'},
        {"print-aggregation-threshold", required_argument, nullptr, 1000},
        {"no-threads",    no_argument,       nullptr, 1002},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "hn:c:b:r:e:f:V", long_opts, nullptr)) != -1) {
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
        case 'c':
            options.lineCapacity = parseNonNegativeSize(optarg, "-c/--line-capacity");
            break;
        case 'b': {
            size_t val = parsePositiveSize(optarg, "-b/--zlib-buffer");
            if (val > std::numeric_limits<unsigned int>::max()) {
                throw std::runtime_error("-b/--zlib-buffer requires a value not exceeding " +
                                         std::to_string(std::numeric_limits<unsigned int>::max()));
            }
            options.zlibBufferSize = val;
            break;
        }
        case 1003:
            options.xzBufferSize = parsePositiveSize(optarg, "--xz-buffer");
            break;
        case 1005:
            options.xzMemLimit = parsePositiveSize(optarg, "--xz-memlimit");
            break;
        case 1004:
            options.zstdWindowSize = parseNonNegativeSize(optarg, "--zstd-window");
            break;
        case 'r':
            options.readBufferSize = parsePositiveSize(optarg, "-r/--read-buffer");
            break;
        case 'e':
            options.zipEntry = optarg;
            break;
        case 'f':
            options.forceFormat = parseFormat(optarg);
            break;
        case 1000:
            options.printAggregationThreshold = parseNonNegativeSize(optarg, "--print-aggregation-threshold");
            break;
        case 1001:
            options.bytesBudget = parseNonNegativeSize(optarg, "--bytes-budget");
            break;
        case 1002:
            options.useThreads = false;
            break;
        case 1006:
            options.listEntries = true;
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
