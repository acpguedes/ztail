#include <gtest/gtest.h>
#include "parser.h"
#include "circular_buffer.h"

TEST(ParserTest, ParseLines) {
    CircularBuffer cb(5);
    Parser parser(cb);

    const char* data = "Line A\nLine B\nLine C\n";
    parser.parse(data, strlen(data));

    testing::internal::CaptureStdout();
    cb.print();
    std::string output = testing::internal::GetCapturedStdout();

    std::string expected = "Line A\nLine B\nLine C\n";
    EXPECT_EQ(output, expected);
}

TEST(ParserTest, ParseWithPartialLine) {
    CircularBuffer cb(5);
    Parser parser(cb);

    const char* data = "Line A\nLine B";
    parser.parse(data, strlen(data));

    parser.finalize(); // Finalize to add partial lines

    testing::internal::CaptureStdout();
    cb.print();
    std::string output = testing::internal::GetCapturedStdout();

    std::string expected = "Line A\nLine B\n";
    EXPECT_EQ(output, expected);
}
