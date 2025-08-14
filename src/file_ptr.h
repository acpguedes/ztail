#ifndef FILE_PTR_H
#define FILE_PTR_H
#include <cstdio>
#include <memory>

struct FileCloser {
    void operator()(FILE* f) const {
        if (f) std::fclose(f);
    }
};
using FilePtr = std::unique_ptr<FILE, FileCloser>;

#endif // FILE_PTR_H
