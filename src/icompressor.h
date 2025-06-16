#ifndef ICOMPRESSOR_H
#define ICOMPRESSOR_H

#include <vector>
#include <cstddef>

class ICompressor {
public:
    virtual ~ICompressor() = default;
    virtual bool decompress(std::vector<char>& outBuffer, size_t& bytesDecompressed) = 0;
};

#endif // ICOMPRESSOR_H
