#include "cli.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <limits>
#include <stdexcept>

void CLI::usage(const char* progName) {
    std::cerr
        << "Usage: " << progName << " [options] <files...>\n"
        << "  -n N        : print the last N lines (default = 10)\n"
        << "  -e <name>   : entry name inside zip archive\n"
        << "If no file is provided, the program reads from stdin.\n"
        << "Compression type is detected automatically.\n";
}

CLIOptions CLI::parse(int argc, char* argv[]) {
    CLIOptions options;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-n") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("-n requires a number");
            }
            char* end = nullptr;
            errno = 0;
            long val = std::strtol(argv[i + 1], &end, 10);
            if (errno != 0 || end == argv[i + 1] || *end != '\0' ||
                val <= 0 || val > std::numeric_limits<int>::max()) {
                throw std::runtime_error("-n requires a positive integer");
            }
            options.n = static_cast<int>(val);
            ++i;
        }
        else if (std::strcmp(argv[i], "-e") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("-e requires an entry name");
            }
            options.zipEntry = argv[i + 1];
            ++i;
        }
        else if (argv[i][0] == '-') {
            throw std::runtime_error(std::string("Unknown option: ") + argv[i]);
        }
        else {
            options.filenames.push_back(argv[i]);
        }
    }

    return options;
}
