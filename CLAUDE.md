# CLAUDE.md - ztail Project Guide for AI Agents

## Project Overview

**ztail** is a high-performance C++17 command-line tool that displays the last N lines of
compressed files without requiring full decompression. It supports gzip, bzip2, xz, zstd,
zip, lz4, and tar archives.

## Build Commands

```bash
# Configure (from project root)
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON

# Build
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Install
sudo make install
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_PORTABLE` | OFF | Use `-mtune=generic` for max compatibility |
| `USE_NATIVE` | OFF | Use `-march=native` for host-specific optimization |
| `USE_CHAR_RING_BUFFER` | ON | Use high-performance CharRingBuffer backend |
| `ZTAIL_USE_THREADS` | ON | Enable producer/consumer threading |
| `ZTAIL_ENABLE_TAR` | ON | Enable TAR support via libarchive |
| `BUILD_TESTING` | OFF | Build unit tests |

### Required Dependencies

- zlib (zlib1g-dev)
- bzip2 (libbz2-dev)
- liblzma (liblzma-dev)
- libzstd (libzstd-dev)
- libzip (libzip-dev)

### Optional Dependencies

- liblz4 (liblz4-dev) - LZ4 support
- libarchive (libarchive-dev) - TAR support

Install all on Ubuntu/Debian:
```bash
sudo apt install zlib1g-dev libbz2-dev liblzma-dev libzstd-dev libzip-dev libarchive-dev liblz4-dev
```

## Project Structure

```
src/
  main.cpp              - Entry point, streaming pipeline, threading
  cli.h/cpp             - Command-line argument parsing (getopt_long)
  parser.h/cpp          - Line splitting from decompressed byte stream
  circular_buffer.h/cpp - std::string-based ring buffer (debug backend)
  char_ring_buffer.h/cpp - Zero-copy byte-level ring buffer (production)
  compression_type.h/cpp - Format detection (magic bytes + extension)
  compressor_factory.h/cpp - Factory pattern for compressor creation
  icompressor.h         - ICompressor interface (polymorphic decompression)
  compressor_*.h/cpp    - Individual format implementations
  tail_plain.h/cpp      - Efficient reverse-scan for plain text files
  file_ptr.h            - RAII wrapper for FILE*
tests/
  test_*.cpp            - Google Test unit tests
```

## Architecture

1. **Detection**: Magic bytes (6 bytes) identify format; extension is fallback
2. **Decompression**: ICompressor interface with streaming decompress() calls
3. **Parsing**: Parser splits byte stream by newlines into CircularBuffer
4. **Buffering**: Ring buffer retains only last N lines in memory
5. **Threading**: Optional producer (decompress) / consumer (parse) pipeline
6. **Output**: Aggregated writes to stdout for I/O efficiency

## Code Style

- C++17 standard, no extensions
- RAII for all resources (FILE*, compression contexts, etc.)
- `[[nodiscard]]` on functions whose return values must be checked
- Error handling via exceptions (std::runtime_error)
- No raw `new`/`delete` - use smart pointers
- Templates for performance-critical paths (CharRingBuffer, processStream)
- Conditional compilation with `#ifdef` for optional features (LZ4, TAR)
- Test each compressor with valid and invalid data

## Testing

Tests use Google Test 1.14.0 (auto-fetched via CMake FetchContent).

```bash
cd build
ctest --output-on-failure

# Run specific test
./ztail_tests --gtest_filter="*Zlib*"
```

## Common Patterns

### Adding a new compression format

1. Create `src/compressor_newformat.h` and `src/compressor_newformat.cpp`
2. Implement `ICompressor` interface
3. Add magic bytes to `detectCompressionTypeFromBytes()` in `compression_type.cpp`
4. Add extension to `fallbackByExtension()` in `compression_type.cpp`
5. Add enum value to `CompressionType` and name to `compressionTypeName()`
6. Register factory in `compressor_factory.cpp`
7. Add tests in `tests/test_compressor_newformat.cpp`
8. Update CMakeLists.txt sources and (if optional) conditional compilation
9. Update CLI format parsing in `cli.cpp`

### Key Invariants

- Parser.residual is a `std::vector<char>` (dynamically sized, no truncation)
- CharRingBuffer: `count <= capacity`, `start` and `end` are valid indices
- ICompressor::decompress() returns false on EOF, sets bytesDecompressed=0
- Double-buffer threading: only resize the buffer not in use by consumer
