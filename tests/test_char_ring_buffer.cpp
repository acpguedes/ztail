#include <gtest/gtest.h>
#include "char_ring_buffer.h"

TEST(CharRingBufferTest, AddAndRetrieve) {
    CharRingBuffer cb(3, 16);
    cb.add("Line 1");
    cb.add("Line 2");
    cb.add("Line 3");
    cb.add("Line 4"); // Overwrites "Line 1"

    testing::internal::CaptureStdout();
    cb.print();
    std::string output = testing::internal::GetCapturedStdout();

    std::string expected = "Line 2\nLine 3\nLine 4\n";
    EXPECT_EQ(output, expected);
}

TEST(CharRingBufferTest, EmptyBuffer) {
    CharRingBuffer cb(0);
    cb.add("Line 1");
    cb.add("Line 2");

    testing::internal::CaptureStdout();
    cb.print();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "");
}
