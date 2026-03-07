# Guia do Desenvolvedor - ztail

Este documento descreve como contribuir com o projeto ztail, incluindo a arquitetura interna,
padrões de código e como adicionar novas funcionalidades.

## Arquitetura Geral

O ztail segue um pipeline de processamento em etapas:

```
Arquivo comprimido
    │
    ▼
Detecção de formato (magic bytes + extensão)
    │
    ▼
Factory cria o ICompressor adequado
    │
    ▼
ICompressor::decompress() → chunks de bytes
    │
    ▼
Parser divide por linhas (\n)
    │
    ▼
CircularBuffer retém as últimas N linhas
    │
    ▼
Saída agregada para stdout
```

Quando threading está habilitado (`ZTAIL_USE_THREADS`), a descompressão (producer) e
o parsing (consumer) rodam em threads separadas com double-buffering.

## Estrutura do Projeto

```
src/
├── main.cpp              # Ponto de entrada, pipeline de streaming, threading
├── cli.h/cpp             # Parsing de argumentos (getopt_long)
├── parser.h/cpp          # Divide stream de bytes em linhas
├── circular_buffer.h/cpp # Ring buffer baseado em std::string (modo debug)
├── char_ring_buffer.h/cpp# Ring buffer zero-copy por bytes (produção)
├── compression_type.h/cpp# Detecção de formato (magic bytes + extensão)
├── compressor_factory.h/cpp # Factory pattern para criação de compressores
├── icompressor.h         # Interface ICompressor (descompressão polimórfica)
├── compressor_*.h/cpp    # Implementações individuais de cada formato
├── tail_plain.h/cpp      # Scan reverso eficiente para arquivos texto
└── file_ptr.h            # Wrapper RAII para FILE*
tests/
└── test_*.cpp            # Testes unitários com Google Test
```

## Configurando o Ambiente de Desenvolvimento

### Dependências

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake pkg-config \
    zlib1g-dev libbz2-dev liblzma-dev libzstd-dev libzip-dev \
    libarchive-dev liblz4-dev
```

### Build para Desenvolvimento

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
make -j$(nproc)
```

### Executar Testes

```bash
cd build
ctest --output-on-failure

# Filtrar testes específicos
./ztail_tests --gtest_filter="*Zlib*"
```

## Como Adicionar um Novo Formato de Compressão

Este é o cenário mais comum de extensão do ztail. Siga os passos abaixo para adicionar
suporte a um novo formato (ex: `snappy`).

### Passo 1: Criar o Compressor

Crie `src/compressor_snappy.h`:

```cpp
#ifndef COMPRESSOR_SNAPPY_H
#define COMPRESSOR_SNAPPY_H

#include "icompressor.h"
#include "file_ptr.h"
#include <string>

class CompressorSnappy : public ICompressor {
public:
    explicit CompressorSnappy(FilePtr&& file, const std::string& filename,
                              size_t bufferSize = 1 << 20);
    [[nodiscard]] bool decompress(std::vector<char>& outBuffer,
                                  size_t& bytesDecompressed) override;
private:
    FilePtr file_;
    std::string filename_;
    bool eof_ = false;
    // ... contexto de descompressão específico do formato
};

#endif // COMPRESSOR_SNAPPY_H
```

Crie `src/compressor_snappy.cpp`:

```cpp
#include "compressor_snappy.h"
#include <stdexcept>

CompressorSnappy::CompressorSnappy(FilePtr&& file, const std::string& filename,
                                   size_t bufferSize)
    : file_(std::move(file)), filename_(filename)
{
    if (!file_) {
        throw std::runtime_error("CompressorSnappy: null file for " + filename);
    }
    // Inicializar o contexto de descompressão aqui
}

bool CompressorSnappy::decompress(std::vector<char>& outBuffer,
                                  size_t& bytesDecompressed) {
    if (eof_) {
        bytesDecompressed = 0;
        return false;
    }

    // Ler dados do arquivo e descomprimir em outBuffer
    // Setar bytesDecompressed com o número de bytes produzidos
    // Retornar false quando atingir EOF

    return true;
}
```

**Contrato da interface `ICompressor::decompress()`:**

- Preenche `outBuffer` com dados descomprimidos
- Seta `bytesDecompressed` com o número de bytes escritos
- Retorna `true` enquanto houver dados
- Retorna `false` no EOF, com `bytesDecompressed = 0`

### Passo 2: Registrar o Tipo de Compressão

Em `src/compression_type.h`, adicione o enum:

```cpp
enum class CompressionType {
    NONE, GZIP, BZIP2, XZ, ZIP, ZSTD, LZ4, TAR,
    SNAPPY  // ← novo
};
```

Em `src/compression_type.cpp`:

1. Adicione os magic bytes na função `detectCompressionTypeFromBytes()`:
```cpp
// Snappy framing format: 0xff 0x06 0x00 0x00
if (len >= 4 && bytes[0] == 0xFF && bytes[1] == 0x06
    && bytes[2] == 0x00 && bytes[3] == 0x00) {
    return CompressionType::SNAPPY;
}
```

2. Adicione a extensão no fallback `fallbackByExtension()`:
```cpp
if (ext == ".snappy" || ext == ".snz") return CompressionType::SNAPPY;
```

3. Adicione o nome em `compressionTypeName()`:
```cpp
case CompressionType::SNAPPY: return "snappy";
```

### Passo 3: Registrar no Factory

Em `src/compressor_factory.cpp`:

```cpp
#include "compressor_snappy.h"

// Dentro de buildCompressorFactoryMap():
factories[CompressionType::SNAPPY] = [](FilePtr&& file, const std::string& fn,
                                        const CLIOptions& opts) {
    return std::make_unique<CompressorSnappy>(std::move(file), fn);
};
```

### Passo 4: Atualizar o CLI

Em `src/cli.cpp`, adicione o formato no parser de `--format`/`-f`:

```cpp
if (fmt == "snappy" || fmt == "snz") return CompressionType::SNAPPY;
```

### Passo 5: Atualizar o CMakeLists.txt

Se o formato for **obrigatório**:
```cmake
# Adicione em ZTAIL_LIB_SOURCES
set(ZTAIL_LIB_SOURCES
    ...
    src/compressor_snappy.cpp
)

# Adicione a dependência
find_package(Snappy REQUIRED)  # ou pkg_check_modules
target_link_libraries(ztail_lib PUBLIC ${SNAPPY_LIBRARIES})
```

Se o formato for **opcional** (recomendado para dependências menos comuns):
```cmake
# Detecção opcional
pkg_check_modules(LIBSNAPPY snappy)

if(LIBSNAPPY_FOUND)
    message(STATUS "Snappy found: ${LIBSNAPPY_INCLUDE_DIRS}")
    include_directories(${LIBSNAPPY_INCLUDE_DIRS})
    list(APPEND ZTAIL_LIB_SOURCES src/compressor_snappy.cpp)
    target_link_libraries(ztail_lib PUBLIC ${LIBSNAPPY_LIBRARIES})
    target_compile_definitions(ztail_lib PUBLIC ZTAIL_HAS_SNAPPY)
else()
    message(STATUS "Snappy not found — Snappy support disabled")
endif()
```

Para formatos opcionais, proteja as inclusões com `#ifdef`:

```cpp
// compression_type.cpp
#ifdef ZTAIL_HAS_SNAPPY
    if (ext == ".snappy") return CompressionType::SNAPPY;
#endif

// compressor_factory.cpp
#ifdef ZTAIL_HAS_SNAPPY
#include "compressor_snappy.h"
#endif
```

### Passo 6: Escrever Testes

Crie `tests/test_compressor_snappy.cpp`:

```cpp
#include <gtest/gtest.h>
#include "compressor_snappy.h"
#include "file_ptr.h"
#include <cstdio>
#include <vector>

// Helper: cria arquivo temporário com dados comprimidos
static std::string createTempSnappyFile(const std::string& content) {
    // Comprimir 'content' no formato snappy e salvar em tmpfile
    // Retornar o path do arquivo
}

TEST(CompressorSnappy, DecompressValidData) {
    auto path = createTempSnappyFile("Hello, World!\n");
    FilePtr fp(std::fopen(path.c_str(), "rb"));
    ASSERT_NE(fp.get(), nullptr);

    CompressorSnappy comp(std::move(fp), path);
    std::vector<char> buf(4096);
    size_t n = 0;

    ASSERT_TRUE(comp.decompress(buf, n));
    EXPECT_GT(n, 0u);

    std::string result(buf.data(), n);
    EXPECT_EQ(result, "Hello, World!\n");

    // Próxima chamada deve retornar EOF
    ASSERT_FALSE(comp.decompress(buf, n));
    EXPECT_EQ(n, 0u);

    std::remove(path.c_str());
}

TEST(CompressorSnappy, InvalidDataThrows) {
    // Testar com dados inválidos/corrompidos
    // O compressor deve lançar std::runtime_error
}

TEST(CompressorSnappy, EmptyFile) {
    // Testar comportamento com arquivo vazio
}
```

Adicione o arquivo de teste no `CMakeLists.txt`:

```cmake
# Dentro da seção de testes, condicionalmente se for opcional:
if(LIBSNAPPY_FOUND)
    target_sources(ztail_tests PRIVATE tests/test_compressor_snappy.cpp)
endif()
```

### Passo 7: Verificar

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
make -j$(nproc)
ctest --output-on-failure
./ztail arquivo.snappy  # teste manual
```

## Padrões de Código

### Estilo Geral

- **C++17** estrito, sem extensões do compilador
- **RAII** para todos os recursos (FILE*, contextos de compressão, etc.)
- **`[[nodiscard]]`** em funções cujo retorno deve ser verificado
- **Exceções** (`std::runtime_error`) para tratamento de erros
- **Smart pointers** — nunca use `new`/`delete` diretamente
- Templates para caminhos críticos de performance

### Convenções de Nomes

- Classes: `PascalCase` (ex: `CompressorZlib`, `CharRingBuffer`)
- Métodos/Funções: `camelCase` (ex: `decompress()`, `detectCompressionType()`)
- Variáveis membro: `camelCase` ou `snake_case_` com sufixo underscore
- Constantes/Enums: `UPPER_CASE` para valores de enum, `PascalCase` para tipos
- Arquivos: `snake_case.cpp/.h` (ex: `compressor_zlib.cpp`)

### Compilação Condicional

Use `#ifdef` para funcionalidades opcionais:

```cpp
#ifdef ZTAIL_HAS_LZ4
#include "compressor_lz4.h"
#endif
```

### Invariantes Importantes

- `Parser.residual` é `std::vector<char>` — nunca trunca linhas parciais
- `CharRingBuffer`: `count <= capacity`, `start` e `end` são índices válidos
- `ICompressor::decompress()` retorna `false` somente no EOF
- Em threading, buffers são redimensionados apenas quando não estão em uso pelo consumer

## Debugging

### Build de Debug

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DUSE_CHAR_RING_BUFFER=OFF -DBUILD_TESTING=ON
```

Usar `USE_CHAR_RING_BUFFER=OFF` ativa o backend `CircularBuffer` baseado em `std::string`,
que é mais fácil de depurar mas mais lento.

### Desabilitar Threading

```bash
cmake .. -DZTAIL_USE_THREADS=OFF
```

Ou em runtime com `--no-threads`.
