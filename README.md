# ztail
<img width="1024" height="1024" alt="ChatGPT Image 22 de ago  de 2025, 06_52_55" src="https://github.com/user-attachments/assets/a7459c72-132f-43d9-80ad-9f6e8ca02c4f" />

**ztail** is an efficient command-line tool to display the last N lines of compressed `.gz`, `.bgz`, `.bgzf`, `.bz2`, `.xz`, `.zip`, `.zst`, optional tar-family archives (`.tar`, `.tar.gz`, `.tar.bz2`, `.tar.xz`, `.tar.zst`, `.tar.lz4` and related variants), and optional `.lz4` files. Designed for high performance, **ztail** is ideal for working with large compressed text files.

## Features

- **Multi-format support:** Decompresses `.gz`, `.bgz`, `.bgzf`, `.bz2`, `.xz`, `.zip`, `.zst`, tar archives, and optional `.lz4` efficiently.
- **Automatic detection:** Compression type is identified from file contents (magic bytes) even when the extension is missing or wrong.
- **High performance:** Optimized for large files - streams data without full decompression, using zero-copy ring buffers and optional producer/consumer threading.
- **Optimized I/O:** Uses `posix_fadvise` readahead hints, full-buffered stdout, and `memchr`-based line scanning for maximum throughput.
- **Intuitive CLI:** Simple usage with flexible options, including `-q` for quiet mode and `--format` to force a specific decompressor.
- **Efficient ring buffer:** Uses a CharRingBuffer backend by default. Build with `-DUSE_CHAR_RING_BUFFER=OFF` to switch to the slower std::string-based buffer for debugging.
- **Stdin support:** Auto-detects compression format from piped data.
- **Archive browsing:** List entries in ZIP and TAR archives with `--list-entries`.

## Installation

### Prerequisites

- **C++17** or higher.
- **CMake** (version 3.16 or higher).
- **zlib** - for `.gz` and `.bgz`/`.bgzf` files.
- **bzip2** - for `.bz2` files.
- **liblzma** - for `.xz` files.
- **libzstd** - for `.zst` files.
- **libzip** - for `.zip` files.
- **liblz4** (optional) - for `.lz4` files.
- **libarchive** (optional) - for tar-based archives.

On Ubuntu/Debian:

```bash
sudo apt install zlib1g-dev libbz2-dev liblzma-dev libzstd-dev libzip-dev libarchive-dev liblz4-dev
```

Library names may vary on other operating systems.

### Build Steps

1. **Clone the Repository:**

   ```bash
   git clone https://github.com/acpguedes/ztail.git
   cd ztail
   ```

2. **Create a Build Directory and Navigate Into It:**

   ```bash
   mkdir build
   cd build
   ```

3. **Configure the Project with CMake:**

   ```bash
   cmake .. -DBUILD_TESTING=ON
   ```

   By default, **ztail** compiles with `-march=x86-64-v2`, which offers a good balance
   of performance and compatibility for most x86-64 systems.

   - For maximum compatibility with older processors:

     ```bash
     cmake .. -DBUILD_PORTABLE=ON
     ```

   - For the highest possible performance on the build machine:

     ```bash
     cmake .. -DUSE_NATIVE=ON
     ```

   - To disable threading:

     ```bash
     cmake .. -DZTAIL_USE_THREADS=OFF
     ```

   - To use the debug buffer backend:

     ```bash
     cmake .. -DUSE_CHAR_RING_BUFFER=OFF
     ```

4. **Build the Project:**

   ```bash
   make -j$(nproc)
   ```

## Usage

### Display the Last N Lines

```bash
ztail -n 20 file.gz
ztail -n 5 file.bz2
ztail file.xz              # default: last 10 lines
ztail -n 2 file.zst
ztail -n 2 file.zip
ztail -n 2 file.tar.gz
cat file.gz | ztail -n 5   # stdin auto-detection
```

### Archive Operations

```bash
ztail -e entry.txt archive.zip           # specific entry
ztail -e "logs/*.txt" archive.zip        # glob pattern
ztail -e "logs/*.txt" archive.tar.gz     # tar entry pattern
ztail --list-entries archive.zip         # list entries
ztail --list-entries archive.tar.gz      # list tar entries
```

### Multi-file and Headers

```bash
ztail file1.gz file2.gz                  # headers shown by default
ztail -q file1.gz file2.gz              # suppress headers
ztail --always-headers file.gz           # force headers for single file
```

### Advanced Options

```bash
ztail -f gzip file.dat                   # force gzip format
ztail --no-threads large.gz              # single-threaded mode
ztail -c 4096 -n 100 file.gz            # larger line capacity
ztail --bytes-budget 1048576 file.gz     # limit total memory
ztail -b 2097152 file.gz                # 2MB zlib buffer
ztail --xz-buffer 262144 file.xz        # 256KB xz buffer
ztail --xz-memlimit 536870912 file.xz   # 512MB xz memory limit
ztail --zstd-window 4194304 file.zst    # 4MB zstd window
```

## CLI Reference

| Option | Short | Description | Default |
|--------|-------|-------------|---------|
| `--lines N` | `-n` | Print the last N lines | 10 |
| `--line-capacity N` | `-c` | Pre-reserve N bytes per line | 512 |
| `--bytes-budget N` | | Limit total bytes for stored lines | unlimited |
| `--zlib-buffer N` | `-b` | Zlib buffer size in bytes | 1048576 (1MB) |
| `--xz-buffer N` | | XZ input buffer size in bytes | 131072 (128KB) |
| `--xz-memlimit N` | | XZ decoder memory limit in bytes | 268435456 (256MB) |
| `--zstd-window N` | | Max zstd window size (power of 2) | unlimited |
| `--read-buffer N` | `-r` | Read buffer size in bytes | 1048576 (1MB) |
| `--entry NAME` | `-e` | Entry name/pattern in archive | first entry |
| `--format FMT` | `-f` | Force format (gzip\|bzip2\|xz\|zstd\|zip\|tar\|lz4\|plain) | auto |
| `--list-entries` | | List archive entries and exit | |
| `--print-aggregation-threshold N` | | Aggregated output threshold | 8388608 (8MB) |
| `--no-threads` | | Disable producer/consumer threading | |
| `--quiet-headers` | `-q` | Suppress multi-file headers | |
| `--always-headers` | | Always show file headers | |
| `--version` | `-V` | Show version and exit | |
| `--help` | `-h` | Show help and exit | |

## Output Format

- Each line is output followed by a newline character (`\n`)
- Lines are output in the same order as they appear in the file
- For compressed files, the entire file is streamed but only the last N lines are kept in memory
- For plain text files, a reverse-scan optimization avoids reading the full file
- Multi-file output separates files with `==> filename <==` headers (like `tail`)

## Suggested Use Cases

1. **Quick inspection of huge files**
   - Compressed files with tens or hundreds of gigabytes
   - Server logs, database dumps, bioinformatics files (FASTQ, VCF)

2. **I/O-limited environments**
   - Cloud storage (S3, GCS), NFS, spinning disks
   - Remote servers over SSH with limited bandwidth

3. **Data pipeline diagnostics**
   - ETL/ELT debugging: check the last processed batch
   - CI/CD pipeline log inspection

4. **Bioinformatics and scientific data**
   - Compressed FASTQ, BAM, VCF files
   - Simulation outputs and time series

5. **Shell scripting**
   - Drop-in replacement for `zcat file.gz | tail -n 20`
   - Works with any supported format automatically

## Supported Formats

| Format | Extensions | Detection | Library | Status |
|--------|-----------|-----------|---------|--------|
| Gzip | `.gz`, `.bgz`, `.bgzf` | Magic bytes | zlib | Required |
| Bzip2 | `.bz2` | Magic bytes | libbzip2 | Required |
| XZ | `.xz` | Magic bytes | liblzma | Required |
| Zstandard | `.zst` | Magic bytes | libzstd | Required |
| ZIP | `.zip` | Magic bytes | libzip | Required |
| LZ4 | `.lz4` | Magic bytes | liblz4 | Optional |
| TAR | `.tar`, `.tgz`, `.tbz2`, `.txz`, `.tzst`, `.taz`, `.tlz4`, `.tar.*` | Extension | libarchive | Optional |
| Plain text | any other | Fallback | stdio | Built-in |

## Tests

Unit tests use Google Test and are located in `tests/`.

```bash
cd build
cmake .. -DBUILD_TESTING=ON
make -j$(nproc)
ctest --output-on-failure
```

## Contributing

Contributions are welcome! To contribute:

1. Fork the repository.
2. Create a branch for your feature or bug fix:
   ```bash
   git checkout -b feature/new-feature
   ```
3. Make your changes and commit using [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/):
   ```bash
   git commit -m "feat: Add new feature"
   ```
4. Push and open a Pull Request.

Please ensure your contributions pass all tests and follow the project's coding standards.
See [CLAUDE.md](CLAUDE.md) for detailed development guidelines.

## License

This project is licensed under the [Apache License 2.0](LICENSE).

## Security Considerations

- **Input Validation:** File paths and CLI arguments are validated before use.
- **Memory Limits:** XZ decoder has a configurable memory limit (default 256MB).
- **Buffer Clamping:** Library-specific buffer sizes are clamped to prevent integer overflow.
- **Error Handling:** All library errors are caught and reported with descriptive messages.
- **Dependency Management:** Keep dependencies up-to-date to mitigate potential security risks.

## Development Highlights

- Modular architecture with ICompressor interface and factory pattern
- Zero-copy CharRingBuffer for minimal memory footprint
- Producer/consumer threading with adaptive buffer sizing
- Magic-byte detection for reliable format identification
- RAII throughout for safe resource management
- Comprehensive test suite with Google Test
- CI/CD with GitHub Actions
