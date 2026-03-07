#ifndef CLI_H
#define CLI_H

#include <string>
#include <vector>
#include "compression_type.h"

struct CLIOptions {
    int n = 10;             // Number of lines to print (default = 10)
    size_t lineCapacity = 512; // Optional pre-reserve size for each line (default = 512)
    size_t bytesBudget = 0;  // Optional total bytes budget for stored lines
    std::vector<std::string> filenames;   // Names of files to process
    std::string zipEntry;   // Optional entry name for zip files
    size_t zlibBufferSize = 1 << 20; // Buffer size for zlib operations
    size_t xzBufferSize = 1 << 17;   // Buffer size for xz operations (128KB)
    size_t xzMemLimit = 256 * 1024 * 1024; // Memory limit for xz decoder (256MB)
    size_t zstdWindowSize = 0;       // Max window size for zstd (0 = default)
    size_t readBufferSize = 1 << 20; // Buffer size for reading files
    size_t printAggregationThreshold = 8 * 1024 * 1024; // Threshold for block printing
    bool useThreads = true; // Enable producer/consumer threads
    CompressionType forceFormat = CompressionType::NONE; // Force decompressor
    bool listEntries = false; // List zip entries and exit
    bool multiFileHeaders = true; // Print file headers when processing multiple files
    bool alwaysShowHeaders = false; // Print file headers even for a single file
};

class CLI {
public:
    [[nodiscard]] static CLIOptions parse(int argc, char* argv[]);
    static void usage(const char* progName);
};

#endif // CLI_H
