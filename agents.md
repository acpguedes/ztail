# agents.md - AI Agent Collaboration Guide for ztail

## Overview

This document describes how AI agents (Claude Code, Codex, Copilot, etc.) should
collaborate when working on the ztail project. It defines workflows, responsibilities,
and best practices for agent-assisted development.

## Agent Workflows

### 1. Bug Fix Workflow

```
1. Reproduce: Write or run a test that demonstrates the bug
2. Locate: Find the root cause in the source code
3. Fix: Apply the minimal change that fixes the bug
4. Test: Ensure the fix passes and doesn't break existing tests
5. Document: Add a test case that prevents regression
```

### 2. New Feature Workflow

```
1. Design: Define the feature scope and interface
2. Implement: Write the feature code following existing patterns
3. Test: Add unit tests for the new feature
4. Document: Update README.md and CLI help text
5. Build: Verify clean build with all configurations
```

### 3. Performance Optimization Workflow

```
1. Profile: Identify the bottleneck (I/O, CPU, memory)
2. Benchmark: Measure baseline performance
3. Optimize: Apply targeted improvements
4. Verify: Run benchmarks to confirm improvement
5. Test: Ensure correctness is maintained
```

### 4. Code Review Workflow

```
1. Architecture: Check alignment with ICompressor/Factory patterns
2. Safety: Verify RAII, no leaks, no UB, no integer overflow
3. Performance: Check for unnecessary allocations or copies
4. Tests: Verify adequate coverage of new/changed code
5. Style: Ensure consistency with existing codebase
```

## Multi-Agent Collaboration

When multiple agents work on ztail:

- **Parallel work**: Different agents can work on different compressor modules
  simultaneously since they are independent
- **Sequential work**: Changes to `main.cpp`, `parser.cpp`, or `char_ring_buffer.cpp`
  should be coordinated since they affect the core pipeline
- **Conflict resolution**: Always prefer the change that includes tests
- **Communication**: Use commit messages to document what was changed and why

## Quality Gates

Before merging any agent-generated changes:

1. `cmake .. -DBUILD_TESTING=ON && make -j$(nproc)` succeeds
2. `ctest --output-on-failure` passes all tests
3. No compiler warnings with `-Wall -Wextra`
4. No memory leaks (test with valgrind if available)
5. Changes are minimal and focused (no unrelated modifications)

## Project-Specific Rules

- Never modify the ICompressor interface without updating ALL implementations
- Always check magic bytes before extension in format detection
- Use `FilePtr` (RAII) instead of raw `FILE*` for ownership
- Use `size_t` for buffer sizes, not `int` or `unsigned int`
- Clamp library-specific size parameters (e.g., gzread takes `unsigned int`)
- Test both valid and invalid inputs for compressors
- Keep optional dependencies conditional (`#ifdef ZTAIL_HAS_LZ4`, etc.)

## Context for Agents

Key files an agent should read before making changes:

| Task | Must Read |
|------|-----------|
| Any change | CLAUDE.md, CMakeLists.txt |
| New format | icompressor.h, compressor_factory.cpp, compression_type.cpp |
| Parser changes | parser.h/cpp, char_ring_buffer.h/cpp |
| CLI changes | cli.h/cpp, main.cpp |
| Threading changes | main.cpp (processStream function) |
| Build changes | CMakeLists.txt, .github/workflows/build.yml |
