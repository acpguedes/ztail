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
        << "  -e, --entry <name> : entry name inside zip archive\n"
        << "  -V, --version  : display program version and exit\n"
        << "  -h, --help     : display this help and exit\n"
        << "If no file is provided, the program reads from stdin.\n"
        << "Compression type is detected automatically.\n";
}

CLIOptions CLI::parse(int argc, char* argv[]) {
    CLIOptions options;

    static struct option long_opts[] = {
        {"help",    no_argument,       nullptr, 'h'},
        {"version", no_argument,       nullptr, 'V'},
        {"lines",   required_argument, nullptr, 'n'},
        {"entry",   required_argument, nullptr, 'e'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "hn:e:V", long_opts, nullptr)) != -1) {
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
        case 'e':
            options.zipEntry = optarg;
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
