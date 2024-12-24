#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>   
#include <cstring>   
#include <stdexcept> 
#include <algorithm> 
#include <lz4.h>     
#include <zip.h>
#include <memory>    

static const size_t READ_BUFFER_SIZE = 1 << 20; 

static const size_t MAX_OUTPUT_SIZE = 4 * 1024 * 1024; 

void usage(const char* progName) {
    std::cerr << "Uso: " << progName << " [-n N] <file.lz4 | file.zip>\n"
              << "       " << progName << " [-n N]\n"
              << "    -n N : show last N lines (default = 10)\n"
              << "    Read standart input if any file is passed (stdin).\n";
}


struct CircularBuffer {
    std::vector<std::string> buffer; 
    size_t capacity;                 
    size_t next;                     
    size_t count;                    

    CircularBuffer(size_t cap) : buffer(cap), capacity(cap), next(0), count(0) {
        
        for(auto &s : buffer) {
            s.reserve(1024); 
        }
    }

    
    void add(std::string&& line) {
        if (capacity == 0) return; 
        buffer[next].assign(std::move(line));
        next = (next + 1) % capacity;
        if (count < capacity) {
            count++;
        }
    }

    void print() const {
        for (size_t i = 0; i < count; ++i) {
            size_t idx = (next + i) % capacity;
            std::cout << buffer[idx] << "\n";
        }
    }
};

void readLz4File(const std::string &filename, CircularBuffer &cb, int n) {
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        throw std::runtime_error("Erro ao abrir arquivo LZ4: " + filename);
    }

    
    std::vector<char> compressedBuffer(READ_BUFFER_SIZE);
    
    std::vector<char> decompressedBuffer(MAX_OUTPUT_SIZE);

    std::string partial;
    partial.reserve(1024); 

    size_t bytesRead = 0;
    while ((bytesRead = fread(compressedBuffer.data(), 1, compressedBuffer.size(), file)) > 0) {
        
        int decompressedSize = LZ4_decompress_safe(compressedBuffer.data(), decompressedBuffer.data(),
                                                  bytesRead, decompressedBuffer.size());
        if (decompressedSize < 0) {
            fclose(file);
            throw std::runtime_error("Erro ao descomprimir bloco LZ4 no arquivo: " + filename);
        }

        size_t pos = 0;
        while (pos < static_cast<size_t>(decompressedSize)) {
            
            char* start_ptr = decompressedBuffer.data() + pos;
            size_t remaining = decompressedSize - pos;
            char* newline_ptr = static_cast<char*>(memchr(start_ptr, '\n', remaining));

            if (newline_ptr) {
                size_t line_length = newline_ptr - start_ptr;
                partial.append(start_ptr, line_length);
                cb.add(std::move(partial));
                partial.clear();
                pos += line_length + 1; 
            } else {
 
                partial.append(start_ptr, remaining);
                break;
            }
        }
    }

    if (ferror(file)) {
        fclose(file);
        throw std::runtime_error("Erro ao ler arquivo LZ4: " + filename);
    }

 
    if (!partial.empty()) {
        cb.add(std::move(partial));
    }

    fclose(file);
}


void readZipFile(const std::string &filename, CircularBuffer &cb, int n) {
    int zipError = 0;
    zip_t* za = zip_open(filename.c_str(), ZIP_RDONLY, &zipError);
    if (!za) {
        throw std::runtime_error("Erro ao abrir arquivo zip: " + filename);
    }

    zip_int64_t numEntries = zip_get_num_entries(za, 0);
    if (numEntries == 0) {
        zip_close(za);
        throw std::runtime_error("ZIP vazio: " + filename);
    }

    zip_file_t* zf = zip_fopen_index(za, 0, 0);
    if (!zf) {
        zip_close(za);
        throw std::runtime_error("Falha ao abrir primeiro arquivo dentro de " + filename);
    }

    std::string partial;
    partial.reserve(1024); 

    std::vector<char> buffer(READ_BUFFER_SIZE);
    zip_int64_t bytesRead = 0;

    while ((bytesRead = zip_fread(zf, buffer.data(), buffer.size())) > 0) {
        size_t pos = 0;
        while (pos < static_cast<size_t>(bytesRead)) {
            
            char* start_ptr = buffer.data() + pos;
            size_t remaining = bytesRead - pos;
            char* newline_ptr = static_cast<char*>(memchr(start_ptr, '\n', remaining));

            if (newline_ptr) {
                size_t line_length = newline_ptr - start_ptr;
                partial.append(start_ptr, line_length);
                cb.add(std::move(partial));
                partial.clear();
                pos += line_length + 1; 
            } else {
                
                partial.append(start_ptr, remaining);
                break;
            }
        }
    }

    if (bytesRead < 0) {
        zip_fclose(zf);
        zip_close(za);
        throw std::runtime_error("Erro ao ler dados de " + filename);
    }

    
    if (!partial.empty()) {
        cb.add(std::move(partial));
    }

    zip_fclose(zf);
    zip_close(za);
}


void readStdin(CircularBuffer &cb, int n) {
    std::string partial;
    partial.reserve(1024); 

    std::vector<char> buffer(READ_BUFFER_SIZE);

    while (true) {
        size_t bytesRead = fread(buffer.data(), 1, buffer.size(), stdin);
        if (bytesRead == 0) break;

        size_t pos = 0;
        while (pos < bytesRead) {
            
            char* start_ptr = buffer.data() + pos;
            size_t remaining = bytesRead - pos;
            char* newline_ptr = static_cast<char*>(memchr(start_ptr, '\n', remaining));

            if (newline_ptr) {
                size_t line_length = newline_ptr - start_ptr;
                partial.append(start_ptr, line_length);
                cb.add(std::move(partial));
                partial.clear();
                pos += line_length + 1; 
            } else {
                
                partial.append(start_ptr, remaining);
                break;
            }
        }
    }

    
    if (!partial.empty()) {
        cb.add(std::move(partial));
    }
}

int main(int argc, char* argv[]) {
    
    int n = 10;

    
    if (argc < 1) { 
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int fileIndex = -1;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-n") == 0) {
            if (i + 1 < argc) {
                n = std::atoi(argv[i + 1]);
                if (n < 0) {
                    std::cerr << "Erro: -n requer um número positivo.\n";
                    usage(argv[0]);
                    return EXIT_FAILURE;
                }
                i++;
            } else {
                std::cerr << "Erro: -n requer um número.\n";
                usage(argv[0]);
                return EXIT_FAILURE;
            }
        } else {
            if (fileIndex == -1) {
                fileIndex = i;
            } else {
                std::cerr << "Erro: múltiplos arquivos fornecidos. Apenas um arquivo é suportado.\n";
                usage(argv[0]);
                return EXIT_FAILURE;
            }
        }
    }

    
    CircularBuffer cb(n);

    try {
        if (fileIndex != -1) {
            
            std::string filename = argv[fileIndex];

            
            bool isLz4 = false;
            bool isZip = false;

            if (filename.size() >= 4 &&
                (filename.compare(filename.size() - 4, 4, ".lz4") == 0)) {
                isLz4 = true;
            }
            else if (filename.size() >= 4 &&
                     (filename.compare(filename.size() - 4, 4, ".zip") == 0)) {
                isZip = true;
            }
            else {
                std::cerr << "Extensão não reconhecida em \"" << filename << "\".\n"
                          << "Suporte apenas para .lz4 e .zip.\n";
                return EXIT_FAILURE;
            }

            if (isLz4) {
                readLz4File(filename, cb, n);
            } else if (isZip) {
                readZipFile(filename, cb, n);
            }
        } else {
            
            readStdin(cb, n);
        }
    } catch (const std::exception &ex) {
        std::cerr << "ERRO: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    
    cb.print();

    return EXIT_SUCCESS;
}
