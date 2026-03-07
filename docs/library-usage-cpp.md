# Usando ztail como Biblioteca C/C++

O ztail é construído internamente como uma biblioteca (`ztail_lib`) que pode ser
reutilizada em outros projetos C/C++. Este guia mostra como integrar os componentes
de descompressão do ztail no seu código.

## Integração via CMake

### Opção 1: Submódulo Git

Adicione o ztail como submódulo do seu projeto:

```bash
git submodule add https://github.com/acpguedes/ztail.git vendor/ztail
```

No seu `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(meu_projeto LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Adicionar ztail como subdirectory
add_subdirectory(vendor/ztail)

# Seu executável
add_executable(minha_app src/main.cpp)
target_link_libraries(minha_app PRIVATE ztail_lib)
```

### Opção 2: FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    ztail
    GIT_REPOSITORY https://github.com/acpguedes/ztail.git
    GIT_TAG main
)
FetchContent_MakeAvailable(ztail)

add_executable(minha_app src/main.cpp)
target_link_libraries(minha_app PRIVATE ztail_lib)
```

### Opção 3: Instalação do Sistema

Compile e instale o ztail:

```bash
cd ztail
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install
```

Depois use `find_package` ou adicione os headers e a lib manualmente.

## Headers Principais

| Header | Descrição |
|--------|-----------|
| `icompressor.h` | Interface abstrata `ICompressor` |
| `compression_type.h` | Enum `CompressionType` e funções de detecção |
| `compressor_factory.h` | Factory para criação de compressores |
| `cli.h` | Struct `CLIOptions` com parâmetros de configuração |
| `parser.h` | `Parser` que divide byte stream em linhas |
| `circular_buffer.h` | Ring buffer para armazenar as últimas N linhas |
| `file_ptr.h` | `FilePtr` — wrapper RAII para `FILE*` |

## Exemplo Completo: Ler Últimas N Linhas

```cpp
#include "compression_type.h"
#include "compressor_factory.h"
#include "parser.h"
#include "circular_buffer.h"
#include "cli.h"

#include <iostream>
#include <vector>
#include <string>

int main() {
    const std::string filename = "dados.gz";
    const int n_linhas = 20;

    // 1. Detectar o formato de compressão
    DetectionResult det = detectCompressionType(filename);
    if (det.type == CompressionType::NONE) {
        std::cerr << "Formato não suportado ou arquivo não comprimido\n";
        return 1;
    }
    std::cout << "Formato detectado: " << compressionTypeName(det.type) << "\n";

    // 2. Configurar opções (valores padrão são adequados para a maioria dos casos)
    CLIOptions opts;

    // 3. Criar o compressor via factory
    auto factories = buildCompressorFactoryMap();
    auto it = factories.find(det.type);
    if (it == factories.end()) {
        std::cerr << "Formato não suportado: " << compressionTypeName(det.type) << "\n";
        return 1;
    }
    auto compressor = it->second(std::move(det.file), filename, opts);

    // 4. Criar buffer circular (retém as últimas N linhas)
    CircularBuffer buffer(n_linhas, opts.lineCapacity);

    // 5. Criar parser que alimenta o buffer
    Parser parser(buffer, opts.lineCapacity);

    // 6. Loop de descompressão
    std::vector<char> chunk(opts.readBufferSize);
    size_t bytesDecompressed = 0;

    while (compressor->decompress(chunk, bytesDecompressed)) {
        parser.parse(chunk.data(), bytesDecompressed);
    }
    parser.finalize();  // Processar última linha parcial, se houver

    // 7. Imprimir resultado
    buffer.print(opts.printAggregationThreshold);

    return 0;
}
```

## Exemplo: Apenas Descomprimir (sem parsing de linhas)

Se você precisa apenas dos bytes descomprimidos (sem dividir em linhas):

```cpp
#include "compression_type.h"
#include "compressor_factory.h"
#include "cli.h"

#include <vector>
#include <fstream>
#include <iostream>

int main() {
    const std::string input = "arquivo.xz";
    const std::string output = "arquivo_descomprimido.txt";

    // Detectar e criar compressor
    DetectionResult det = detectCompressionType(input);
    CLIOptions opts;
    auto factories = buildCompressorFactoryMap();
    auto compressor = factories[det.type](std::move(det.file), input, opts);

    // Descomprimir para um arquivo de saída
    std::ofstream out(output, std::ios::binary);
    std::vector<char> buf(1 << 20);  // 1MB buffer
    size_t n = 0;

    while (compressor->decompress(buf, n)) {
        out.write(buf.data(), n);
    }

    std::cout << "Descomprimido com sucesso: " << output << "\n";
    return 0;
}
```

## Exemplo: Detectar Formato sem Descomprimir

```cpp
#include "compression_type.h"
#include <iostream>
#include <cstdio>

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        DetectionResult det = detectCompressionType(argv[i]);
        std::cout << argv[i] << ": "
                  << compressionTypeName(det.type) << "\n";
        // det.file é automaticamente fechado ao sair do escopo (RAII)
    }
    return 0;
}
```

## Exemplo: Implementar um Compressor Personalizado

Você pode criar seu próprio compressor implementando a interface `ICompressor`:

```cpp
#include "icompressor.h"
#include "file_ptr.h"
#include <cstdio>

class MeuCompressor : public ICompressor {
public:
    explicit MeuCompressor(FilePtr&& file)
        : file_(std::move(file)), eof_(false) {}

    [[nodiscard]] bool decompress(std::vector<char>& outBuffer,
                                  size_t& bytesDecompressed) override {
        if (eof_) {
            bytesDecompressed = 0;
            return false;
        }

        // Sua lógica de descompressão aqui
        size_t read = std::fread(outBuffer.data(), 1, outBuffer.size(),
                                 file_.get());
        if (read == 0) {
            eof_ = true;
            bytesDecompressed = 0;
            return false;
        }

        bytesDecompressed = read;
        return true;
    }

private:
    FilePtr file_;
    bool eof_;
};
```

## Compilando com Linkagem Direta (sem CMake)

Se preferir compilar sem CMake, linke diretamente contra as bibliotecas necessárias:

```bash
g++ -std=c++17 -O2 -I/path/to/ztail/src \
    minha_app.cpp \
    /path/to/ztail/src/compression_type.cpp \
    /path/to/ztail/src/compressor_factory.cpp \
    /path/to/ztail/src/compressor_zlib.cpp \
    /path/to/ztail/src/compressor_bzip2.cpp \
    /path/to/ztail/src/compressor_xz.cpp \
    /path/to/ztail/src/compressor_zip.cpp \
    /path/to/ztail/src/compressor_zstd.cpp \
    /path/to/ztail/src/parser.cpp \
    /path/to/ztail/src/circular_buffer.cpp \
    /path/to/ztail/src/char_ring_buffer.cpp \
    /path/to/ztail/src/cli.cpp \
    -lz -lbz2 -llzma -lzstd -lzip \
    -DUSE_CHAR_RING_BUFFER \
    -o minha_app
```

## Opções de Configuração (CLIOptions)

A struct `CLIOptions` oferece controle fino sobre o comportamento:

```cpp
CLIOptions opts;
opts.n = 50;                    // Últimas 50 linhas (padrão: 10)
opts.lineCapacity = 1024;       // Pré-alocação por linha (padrão: 512)
opts.bytesBudget = 10'000'000;  // Limite total de memória em bytes (0 = ilimitado)
opts.zlibBufferSize = 2 << 20;  // Buffer do zlib: 2MB (padrão: 1MB)
opts.xzMemLimit = 512 * 1024 * 1024; // Limite de memória XZ: 512MB (padrão: 256MB)
opts.readBufferSize = 2 << 20;  // Buffer de leitura: 2MB (padrão: 1MB)
opts.zipEntry = "data.csv";     // Entrada específica dentro de .zip
```

## Tratamento de Erros

O ztail usa exceções (`std::runtime_error`) para reportar erros:

```cpp
try {
    DetectionResult det = detectCompressionType("arquivo.gz");
    auto factories = buildCompressorFactoryMap();
    auto comp = factories[det.type](std::move(det.file), "arquivo.gz", opts);

    std::vector<char> buf(1 << 20);
    size_t n;
    while (comp->decompress(buf, n)) {
        // processar dados
    }
} catch (const std::runtime_error& e) {
    std::cerr << "Erro: " << e.what() << "\n";
}
```

Erros comuns:
- Arquivo não encontrado ou sem permissão de leitura
- Dados corrompidos / formato inválido
- Memória insuficiente (especialmente com XZ)
- Entrada inexistente em arquivo ZIP/TAR
