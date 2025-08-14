#include <gtest/gtest.h>
#include "circular_buffer.h"
#include "char_ring_buffer.h"
#include <chrono>
#include <string>
#include <iostream>

TEST(BufferBenchmark, PerformanceComparison) {
    const size_t lines = 10000;
    std::string sample = "Sample line";

    CircularBuffer cb(lines, sample.size());
    CharRingBuffer crb(lines, sample.size());

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < lines; ++i) {
        cb.add("Sample line");
    }
    auto mid = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < lines; ++i) {
        crb.add("Sample line");
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto cbTime = std::chrono::duration_cast<std::chrono::microseconds>(mid - start).count();
    auto crbTime = std::chrono::duration_cast<std::chrono::microseconds>(end - mid).count();
    std::cout << "CircularBuffer add time: " << cbTime << "us\n";
    std::cout << "CharRingBuffer add time: " << crbTime << "us\n";
}

TEST(BufferBenchmark, MemoryUsageComparison) {
    const size_t lines = 1000;
    CircularBuffer cb(lines, 20);
    CharRingBuffer crb(lines, 20);
    for (size_t i = 0; i < lines; ++i) {
        cb.add(std::string(20, 'x'));
        crb.add(std::string(20, 'x'));
    }
    size_t memCb = cb.memoryUsage();
    size_t memCrb = crb.memoryUsage();
    std::cout << "CircularBuffer memory: " << memCb << " bytes\n";
    std::cout << "CharRingBuffer memory: " << memCrb << " bytes\n";
    EXPECT_LE(memCrb, memCb);
}
