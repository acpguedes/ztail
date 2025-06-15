#ifndef CLI_H
#define CLI_H

#include <string>

struct CLIOptions {
    int n = 10;             // Number of lines to print (default = 10)
    std::string filename;   // Name of the file (empty if reading from stdin)
    std::string zip_entry;  // Name of the entry inside a zip archive
};

class CLI {
public:
    static CLIOptions parse(int argc, char* argv[]);
    static void usage(const char* progName);
};

#endif // CLI_H
