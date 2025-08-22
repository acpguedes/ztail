#include <gtest/gtest.h>
#include "char_ring_buffer.h"

TEST(CharRingBufferTest, AddAndRetrieve) {
    CharRingBuffer cb(3, 16);
    cb.append_segment("Line 1", 6); cb.end_line();
    cb.append_segment("Line 2", 6); cb.end_line();
    cb.append_segment("Line 3", 6); cb.end_line();
    cb.append_segment("Line 4", 6); cb.end_line(); // Overwrites "Line 1"

    testing::internal::CaptureStdout();
    cb.print(1024);
    std::string output = testing::internal::GetCapturedStdout();

    std::string expected = "Line 2\nLine 3\nLine 4\n";
    EXPECT_EQ(output, expected);
}

TEST(CharRingBufferTest, AppendLine) {
    CharRingBuffer cb(3, 16);
    cb.append_line("Line 1", 6);
    cb.append_line("Line 2", 6);
    cb.append_line("Line 3", 6);
    cb.append_line("Line 4", 6);

    testing::internal::CaptureStdout();
    cb.print(1024);
    std::string output = testing::internal::GetCapturedStdout();

    std::string expected = "Line 2\nLine 3\nLine 4\n";
    EXPECT_EQ(output, expected);
}

TEST(CharRingBufferTest, EmptyBuffer) {
    CharRingBuffer cb(0);
    cb.append_segment("Line 1", 6); cb.end_line();
    cb.append_segment("Line 2", 6); cb.end_line();

    testing::internal::CaptureStdout();
    cb.print(1024);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "");
}

TEST(CharRingBufferTest, LargeBufferPromotesOffset) {
    using BigBuffer = CharRingBuffer<static_cast<size_t>(UINT32_MAX) + 1>;
    static_assert(sizeof(typename BigBuffer::Offset) == sizeof(uint64_t),
                  "Offset should be 64-bit for large buffers");

    BigBuffer cb(2, 1);
    cb.append_segment("A", 1); cb.end_line();

    testing::internal::CaptureStdout();
    cb.print(1024);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "A\n");
}

TEST(CharRingBufferTest, RespectsBytesBudget) {
    CharRingBuffer cb(10, 0, 16);
    cb.add("12345678");
    cb.add("abcdefgh");
    cb.add("ijkl");

    testing::internal::CaptureStdout();
    cb.print(1024);
    std::string output = testing::internal::GetCapturedStdout();

    std::string expected = "abcdefgh\nijkl\n";
    EXPECT_EQ(output, expected);
}

TEST(CharRingBufferTest, OversizedLineIgnored) {
    CharRingBuffer cb(10, 0, 16);
    cb.add("This line is way too long to fit in the budget");

    testing::internal::CaptureStdout();
    cb.print(1024);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "");
}

TEST(CharRingBufferTest, LazyAllocatesData) {
    CharRingBuffer cb(3);
    cb.add("Hi");

    testing::internal::CaptureStdout();
    cb.print(1024);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "Hi\n");
}
