#ifndef CLI_H
#define CLI_H

#include <string>
#include <vector>

struct CLIOptions {
    int n = 10;             // Number of lines to print (default = 10)
    size_t lineCapacity = 0; // Optional pre-reserve size for each line
    std::vector<std::string> filenames;   // Names of files to process
    std::string zipEntry;   // Optional entry name for zip files
    size_t zlibBufferSize = 1 << 20; // Buffer size for zlib operations
};

class CLI {
public:
    static CLIOptions parse(int argc, char* argv[]);
    static void usage(const char* progName);
};

#endif // CLI_H
