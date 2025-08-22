#include <gtest/gtest.h>
#include "circular_buffer.h"

TEST(CircularBufferTest, AddAndRetrieve) {
    CircularBuffer cb(3, 16);
    cb.add("Line 1");
    cb.add("Line 2");
    cb.add("Line 3");
    cb.add("Line 4"); // Overwrites "Line 1"

    testing::internal::CaptureStdout();
    cb.print(1024);
    std::string output = testing::internal::GetCapturedStdout();

    std::string expected = "Line 2\nLine 3\nLine 4\n";
    EXPECT_EQ(output, expected);
}

TEST(CircularBufferTest, EmptyBuffer) {
    CircularBuffer cb(0);
    cb.add("Line 1");
    cb.add("Line 2");

    testing::internal::CaptureStdout();
    cb.print(1024);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "");
}

TEST(CircularBufferTest, RespectsBytesBudget) {
    CircularBuffer cb(10, 0, 16);
    cb.add("12345678");
    cb.add("abcdefgh");
    cb.add("ijkl");

    testing::internal::CaptureStdout();
    cb.print(1024);
    std::string output = testing::internal::GetCapturedStdout();

    std::string expected = "abcdefgh\nijkl\n";
    EXPECT_EQ(output, expected);
}

TEST(CircularBufferTest, OversizedLineIgnored) {
    CircularBuffer cb(10, 0, 16);
    cb.add("This line is way too long to fit in the budget");

    testing::internal::CaptureStdout();
    cb.print(1024);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "");
}
