#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>   
#include <cstring>   
#include <stdexcept> 
#include <zlib.h>
#include <zip.h>
#include <algorithm> 


static const size_t READ_BUFFER_SIZE = 1 << 20; 


void usage(const char* progName) {
    std::cerr << "Uso: " << progName << " [-n N] <arquivo.gz | arquivo.bgz | arquivo.zip>\n"
              << "       " << progName << " [-n N]\n"
              << "    -n N : exibir as últimas N linhas (padrão = 10)\n"
              << "    Se nenhum arquivo for fornecido, o programa lê da entrada padrão (stdin).\n";
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


void readGzFile(const std::string &filename, CircularBuffer &cb, int n) {
    gzFile gz = gzopen(filename.c_str(), "rb");
    if (!gz) {
        throw std::runtime_error("Erro ao abrir arquivo gz/bgz: " + filename);
    }

    std::string partial;
    partial.reserve(1024); 

    std::vector<char> buffer(READ_BUFFER_SIZE);
    int bytesRead = 0;

    while ((bytesRead = gzread(gz, buffer.data(), buffer.size())) > 0) {
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
        gzclose(gz);
        throw std::runtime_error("Erro ao ler arquivo gz/bgz: " + filename);
    }

    
    if (!partial.empty()) {
        cb.add(std::move(partial));
    }

    gzclose(gz);
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

            
            bool isGz  = false;
            bool isBgz = false;
            bool isZip = false;

            if (filename.size() >= 3 &&
                (filename.compare(filename.size() - 3, 3, ".gz") == 0)) {
                isGz = true;
            }
            else if (filename.size() >= 4 &&
                     (filename.compare(filename.size() - 4, 4, ".bgz") == 0)) {
                isBgz = true;
            }
            else if (filename.size() >= 4 &&
                     (filename.compare(filename.size() - 4, 4, ".zip") == 0)) {
                isZip = true;
            }
            else {
                std::cerr << "Extensão não reconhecida em \"" << filename << "\".\n"
                          << "Suporte apenas para .gz, .bgz e .zip.\n";
                return EXIT_FAILURE;
            }

            if (isGz || isBgz) {
                readGzFile(filename, cb, n);
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
