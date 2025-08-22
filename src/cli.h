#ifndef CLI_H
#define CLI_H

#include <string>
#include <vector>

struct CLIOptions {
    int n = 10;             // Number of lines to print (default = 10)
    size_t lineCapacity = 0; // Optional pre-reserve size for each line
    size_t bytesBudget = 0;  // Optional total bytes budget for stored lines
    std::vector<std::string> filenames;   // Names of files to process
    std::string zipEntry;   // Optional entry name for zip files
    size_t zlibBufferSize = 1 << 20; // Buffer size for zlib operations
    size_t readBufferSize = 1 << 20; // Buffer size for reading files
    size_t printAggregationThreshold = 8 * 1024 * 1024; // Threshold for block printing
};

class CLI {
public:
    static CLIOptions parse(int argc, char* argv[]);
    static void usage(const char* progName);
};

#endif // CLI_H
