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

TEST(CLIExtendedTest, QuietShortFlag) {
    std::vector<std::string> args = {"ztail", "-q", "a.gz", "b.gz"};
    auto argv = makeArgv(args);
    optind = 1;
    CLIOptions options = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_FALSE(options.multiFileHeaders);
    EXPECT_FALSE(options.alwaysShowHeaders);
}

TEST(CLIExtendedTest, FormatFlag) {
    std::vector<std::string> args = {"ztail", "-f", "gzip", "file.dat"};
    auto argv = makeArgv(args);
    optind = 1;
    CLIOptions options = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_EQ(options.forceFormat, CompressionType::GZIP);
}

TEST(CLIExtendedTest, FormatFlagBzip2) {
    std::vector<std::string> args = {"ztail", "--format", "bz2", "file.dat"};
    auto argv = makeArgv(args);
    optind = 1;
    CLIOptions options = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_EQ(options.forceFormat, CompressionType::BZIP2);
}

TEST(CLIExtendedTest, FormatFlagPlain) {
    std::vector<std::string> args = {"ztail", "-f", "plain", "file.gz"};
    auto argv = makeArgv(args);
    optind = 1;
    CLIOptions options = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_EQ(options.forceFormat, CompressionType::NONE);
}

TEST(CLIExtendedTest, FormatFlagNone) {
    std::vector<std::string> args = {"ztail", "-f", "none", "file.gz"};
    auto argv = makeArgv(args);
    optind = 1;
    CLIOptions options = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_EQ(options.forceFormat, CompressionType::NONE);
}

TEST(CLIExtendedTest, InvalidFormatThrows) {
    std::vector<std::string> args = {"ztail", "-f", "invalid", "file.dat"};
    auto argv = makeArgv(args);
    optind = 1;
    EXPECT_THROW({
        (void)CLI::parse(static_cast<int>(argv.size()), argv.data());
    }, std::runtime_error);
}

TEST(CLIExtendedTest, ListEntriesFlag) {
    std::vector<std::string> args = {"ztail", "--list-entries", "archive.zip"};
    auto argv = makeArgv(args);
    optind = 1;
    CLIOptions options = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_TRUE(options.listEntries);
}

TEST(CLIExtendedTest, NoThreadsFlag) {
    std::vector<std::string> args = {"ztail", "--no-threads", "file.gz"};
    auto argv = makeArgv(args);
    optind = 1;
    CLIOptions options = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_FALSE(options.useThreads);
}

TEST(CLIExtendedTest, BytesBudget) {
    std::vector<std::string> args = {"ztail", "--bytes-budget", "1048576", "file.gz"};
    auto argv = makeArgv(args);
    optind = 1;
    CLIOptions options = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_EQ(options.bytesBudget, 1048576u);
}

TEST(CLIExtendedTest, EntryName) {
    std::vector<std::string> args = {"ztail", "-e", "logs/*.txt", "archive.zip"};
    auto argv = makeArgv(args);
    optind = 1;
    CLIOptions options = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_EQ(options.zipEntry, "logs/*.txt");
}

TEST(CLIExtendedTest, XzMemlimit) {
    std::vector<std::string> args = {"ztail", "--xz-memlimit", "536870912", "file.xz"};
    auto argv = makeArgv(args);
    optind = 1;
    CLIOptions options = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_EQ(options.xzMemLimit, 536870912u);
}

TEST(CLIExtendedTest, DefaultValues) {
    std::vector<std::string> args = {"ztail", "file.gz"};
    auto argv = makeArgv(args);
    optind = 1;
    CLIOptions options = CLI::parse(static_cast<int>(argv.size()), argv.data());
    EXPECT_EQ(options.n, 10);
    EXPECT_EQ(options.lineCapacity, 512u);
    EXPECT_EQ(options.bytesBudget, 0u);
    EXPECT_EQ(options.zlibBufferSize, 1u << 20);
    EXPECT_EQ(options.xzBufferSize, 1u << 17);
    EXPECT_EQ(options.xzMemLimit, 256u * 1024 * 1024);
    EXPECT_EQ(options.zstdWindowSize, 0u);
    EXPECT_EQ(options.readBufferSize, 1u << 20);
    EXPECT_TRUE(options.useThreads);
    EXPECT_TRUE(options.multiFileHeaders);
    EXPECT_FALSE(options.alwaysShowHeaders);
    EXPECT_FALSE(options.listEntries);
    EXPECT_EQ(options.forceFormat, CompressionType::NONE);
}
