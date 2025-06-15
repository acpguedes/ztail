#ifndef TAIL_PLAIN_H
#define TAIL_PLAIN_H

#include <string>
#include "parser.h"

void tailPlainFile(const std::string& filename, Parser& parser, size_t n, size_t bufferSize);

#endif
