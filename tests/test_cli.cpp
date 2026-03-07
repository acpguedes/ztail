#include <gtest/gtest.h>
#include "cli.h"
#include <vector>
#include <getopt.h>
#include <string>

static std::vector<char*> makeArgv(std::vector<std::string>& args) {
    std::vector<char*> out;
    out.reserve(args.size());
    for (auto& s : args) {
        out.push_back(s.data());
    }
    return out;
}

TEST(CLITest, ParsesHeaderFlags) {
    std::vector<std::string> args = {"ztail", "--quiet-headers", "a.gz", "b.gz"};
    auto argv = makeArgv(args);
    optind = 1;
    CLIOptions options = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_FALSE(options.multiFileHeaders);
    EXPECT_FALSE(options.alwaysShowHeaders);
}

TEST(CLITest, ParsesAlwaysHeadersFlag) {
    std::vector<std::string> args = {"ztail", "--always-headers", "a.gz"};
    auto argv = makeArgv(args);
    optind = 1;
    CLIOptions options = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_TRUE(options.multiFileHeaders);
    EXPECT_TRUE(options.alwaysShowHeaders);
}

TEST(CLITest, RejectsInvalidZstdWindow) {
    std::vector<std::string> args = {"ztail", "--zstd-window", "1000", "a.zst"};
    auto argv = makeArgv(args);
    optind = 1;
    EXPECT_THROW({
        (void)CLI::parse(static_cast<int>(argv.size()), argv.data());
    }, std::runtime_error);
}
