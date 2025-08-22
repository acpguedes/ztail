#include <gtest/gtest.h>
#include "char_ring_buffer.h"

TEST(CharRingBufferTest, AddAndRetrieve) {
    CharRingBuffer cb(3, 16);
    cb.append_segment("Line 1", 6); cb.end_line();
    cb.append_segment("Line 2", 6); cb.end_line();
    cb.append_segment("Line 3", 6); cb.end_line();
    cb.append_segment("Line 4", 6); cb.end_line(); // Overwrites "Line 1"

    testing::internal::CaptureStdout();
    cb.print();
    std::string output = testing::internal::GetCapturedStdout();

    std::string expected = "Line 2\nLine 3\nLine 4\n";
    EXPECT_EQ(output, expected);
}

TEST(CharRingBufferTest, EmptyBuffer) {
    CharRingBuffer cb(0);
    cb.append_segment("Line 1", 6); cb.end_line();
    cb.append_segment("Line 2", 6); cb.end_line();

    testing::internal::CaptureStdout();
    cb.print();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "");
}
