#include "cli.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <limits>

void CLI::usage(const char* progName) {
    std::cerr
        << "Usage: " << progName << " [-n N] <file.gz | file.bgz | file.bz2 | file.zip>\n"
        << "       " << progName << " [-n N] [--entry name]\n"
        << "  -n N       : print the last N lines (default = 10)\n"
        << "  --entry nm : read the specified entry from a zip archive\n"
        << "  If no file is provided, the program reads from stdin.\n";
}

CLIOptions CLI::parse(int argc, char* argv[]) {
    CLIOptions options;
    int fileIndex = -1;

    if (argc < 1) {
        // Fallback usage call
        usage("ztail_modular");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-n") == 0) {
            if (i + 1 < argc) {
                char* end = nullptr;
                errno = 0;
                long val = std::strtol(argv[i + 1], &end, 10);
                if (errno != 0 || end == argv[i + 1] || *end != '\0' ||
                    val <= 0 || val > std::numeric_limits<int>::max()) {
                    std::cerr << "Error: -n requires a positive integer.\n";
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                options.n = static_cast<int>(val);
                i++;
            } else {
                std::cerr << "Error: -n requires a number.\n";
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        } else if (std::strcmp(argv[i], "--entry") == 0) {
            if (i + 1 < argc) {
                options.zip_entry = argv[i + 1];
                i++;
            } else {
                std::cerr << "Error: --entry requires a name.\n";
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        } else {
            // Only one file is supported; if a second one is provided, it's an error
            if (fileIndex == -1) {
                fileIndex = i;
                options.filename = argv[i];
            } else {
                std::cerr << "Error: multiple files provided. Only one is supported.\n";
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
    }

    return options;
}
