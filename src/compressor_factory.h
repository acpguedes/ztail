#ifndef COMPRESSOR_FACTORY_H
#define COMPRESSOR_FACTORY_H

#include "icompressor.h"
#include "compression_type.h"
#include "cli.h"
#include "file_ptr.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

using CompressorFactory = std::function<std::unique_ptr<ICompressor>(
    FilePtr&&, const std::string&, const CLIOptions&)>;

[[nodiscard]]
std::unordered_map<CompressionType, CompressorFactory> buildCompressorFactoryMap();

#endif // COMPRESSOR_FACTORY_H
