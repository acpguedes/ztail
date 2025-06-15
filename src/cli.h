#ifndef CLI_H
#define CLI_H

#include <string>
#include <vector>

struct CLIOptions {
    int n = 10;             // Number of lines to print (default = 10)
    std::vector<std::string> filenames;   // Names of files to process
    std::string zipEntry;   // Optional entry name for zip files
};

class CLI {
public:
    static CLIOptions parse(int argc, char* argv[]);
    static void usage(const char* progName);
};

#endif // CLI_H
