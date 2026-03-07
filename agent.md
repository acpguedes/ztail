# agent.md - Individual Agent Configuration for ztail

## Agent Identity

This file configures the behavior of a single AI agent working on the ztail project.

## Environment Setup

Before starting work, the agent should:

```bash
# Install dependencies
sudo apt-get update && sudo apt-get install -y \
  cmake build-essential pkg-config \
  zlib1g-dev libbz2-dev liblzma-dev libzstd-dev libzip-dev \
  libarchive-dev liblz4-dev

# Build the project
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
make -j$(nproc)

# Verify tests pass
ctest --output-on-failure
```

## Agent Capabilities

### What this agent CAN do:
- Fix bugs in existing code
- Add new compression format support
- Optimize performance of existing code
- Add or improve unit tests
- Update documentation
- Refactor code for clarity

### What this agent should NOT do:
- Remove existing compression format support
- Change the ICompressor interface without full impact analysis
- Modify test infrastructure (Google Test setup)
- Change the license
- Add external dependencies without justification

## Decision Framework

When facing ambiguous choices:

1. **Correctness over performance**: Always fix bugs before optimizing
2. **Simplicity over cleverness**: Prefer readable code
3. **Consistency over novelty**: Follow existing patterns
4. **Tests over documentation**: Working tests prove correctness

## Commit Message Format

Follow conventional commits:

```
type(scope): description

Types: fix, feat, perf, refactor, test, docs, build, ci
Scopes: cli, parser, buffer, zlib, bzip2, xz, zstd, zip, lz4, tar, detection
```

Examples:
```
fix(bzip2): clamp buffer size to INT_MAX to prevent overflow
feat(detection): add .bgzf extension recognition
perf(tail): use memchr for newline counting instead of std::count
test(parser): add test for lines spanning multiple parse chunks
```

## Error Handling Conventions

- Use `std::runtime_error` for all recoverable errors
- Include the filename in error messages: `"format error (code) while action 'filename'"`
- Never silently swallow errors
- Check return values of all library calls
- Validate user input in CLI parsing, not in library code

## Performance Priorities

1. **I/O throughput**: Minimize syscalls, use large buffers, advise kernel
2. **Memory efficiency**: Ring buffer keeps only N lines, no full decompression
3. **CPU efficiency**: Use memchr/memcpy, avoid unnecessary string copies
4. **Thread efficiency**: Double-buffering, adaptive buffer sizing
