#include "compressor_factory.h"
#include "compressor_zlib.h"
#include "compressor_bzip2.h"
#include "compressor_xz.h"
#include "compressor_zstd.h"
#include "compressor_zip.h"
#ifdef ZTAIL_HAS_TAR
#include "compressor_tar.h"
#endif
#ifdef ZTAIL_HAS_LZ4
#include "compressor_lz4.h"
#endif

std::unordered_map<CompressionType, CompressorFactory> buildCompressorFactoryMap() {
    std::unordered_map<CompressionType, CompressorFactory> map;
    map[CompressionType::GZIP] = [](FilePtr&& f, const std::string& name, const CLIOptions& opts) {
        return std::make_unique<CompressorZlib>(std::move(f), name, opts.zlibBufferSize);
    };
    map[CompressionType::BZIP2] = [](FilePtr&& f, const std::string& name, const CLIOptions&) {
        return std::make_unique<CompressorBzip2>(std::move(f), name);
    };
    map[CompressionType::XZ] = [](FilePtr&& f, const std::string& name, const CLIOptions& opts) {
        return std::make_unique<CompressorXz>(std::move(f), name, opts.xzBufferSize, opts.xzMemLimit);
    };
    map[CompressionType::ZIP] = [](FilePtr&& f, const std::string& name, const CLIOptions& opts) {
        return std::make_unique<CompressorZip>(std::move(f), name, opts.zipEntry);
    };
    map[CompressionType::ZSTD] = [](FilePtr&& f, const std::string& name, const CLIOptions& opts) {
        return std::make_unique<CompressorZstd>(std::move(f), name, opts.zstdWindowSize);
    };
#ifdef ZTAIL_HAS_TAR
    map[CompressionType::TAR] = [](FilePtr&& f, const std::string& name, const CLIOptions& opts) {
        return std::make_unique<CompressorTar>(std::move(f), name, opts.zipEntry);
    };
#endif
#ifdef ZTAIL_HAS_LZ4
    map[CompressionType::LZ4] = [](FilePtr&& f, const std::string& name, const CLIOptions&) {
        return std::make_unique<CompressorLz4>(std::move(f), name);
    };
#endif
    return map;
}
