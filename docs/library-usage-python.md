# Usando ztail com Python

O ztail é uma ferramenta C++ que pode ser integrada a projetos Python de diversas formas:
via `subprocess`, via bindings C com `ctypes`/`cffi`, ou via extensão nativa com `pybind11`.

## Opção 1: Usando via subprocess (mais simples)

A forma mais rápida de usar o ztail em Python é chamando o binário diretamente:

```python
import subprocess


def ztail(filename: str, n: int = 10, format: str | None = None) -> str:
    """Retorna as últimas N linhas de um arquivo comprimido."""
    cmd = ["ztail", "-n", str(n)]
    if format:
        cmd.extend(["-f", format])
    cmd.append(filename)

    result = subprocess.run(cmd, capture_output=True, text=True, check=True)
    return result.stdout


# Uso
linhas = ztail("logs/app.log.gz", n=50)
print(linhas)

# Com formato forçado
linhas = ztail("dados.bin", n=20, format="zstd")
```

### Processando linha a linha

```python
import subprocess


def ztail_lines(filename: str, n: int = 10) -> list[str]:
    """Retorna as últimas N linhas como lista."""
    result = subprocess.run(
        ["ztail", "-n", str(n), filename],
        capture_output=True, text=True, check=True
    )
    return result.stdout.splitlines()


for linha in ztail_lines("server.log.bz2", n=100):
    if "ERROR" in linha:
        print(linha)
```

### Múltiplos arquivos

```python
import subprocess
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor


def ztail_file(path: Path, n: int = 10) -> tuple[Path, str]:
    result = subprocess.run(
        ["ztail", "-n", str(n), str(path)],
        capture_output=True, text=True, check=True
    )
    return path, result.stdout


# Processar vários arquivos em paralelo
logs = list(Path("logs/").glob("*.gz"))
with ThreadPoolExecutor(max_workers=4) as pool:
    for path, output in pool.map(lambda p: ztail_file(p, 20), logs):
        print(f"--- {path} ---")
        print(output)
```

### Listar entradas de ZIP/TAR

```python
import subprocess


def listar_entradas(arquivo: str) -> list[str]:
    """Lista as entradas dentro de um ZIP ou TAR."""
    result = subprocess.run(
        ["ztail", "--list-entries", arquivo],
        capture_output=True, text=True, check=True
    )
    return result.stdout.splitlines()


def ztail_entrada(arquivo: str, entrada: str, n: int = 10) -> str:
    """Lê as últimas N linhas de uma entrada específica."""
    result = subprocess.run(
        ["ztail", "-n", str(n), "-e", entrada, arquivo],
        capture_output=True, text=True, check=True
    )
    return result.stdout


# Exemplo
entradas = listar_entradas("dados.tar.gz")
for e in entradas:
    print(f"Entrada: {e}")
    print(ztail_entrada("dados.tar.gz", e, n=5))
```

## Opção 2: Wrapper C para ctypes/cffi

Para integração mais performática sem depender de subprocess, você pode criar
um wrapper C sobre a biblioteca `ztail_lib` e usá-lo com `ctypes`.

### Passo 1: Criar o wrapper C

Crie `ztail_python_wrapper.cpp`:

```cpp
#include "compression_type.h"
#include "compressor_factory.h"
#include "parser.h"
#include "circular_buffer.h"
#include "cli.h"

#include <cstring>
#include <sstream>
#include <vector>
#include <string>

extern "C" {

// Struct opaca para manter o estado
struct ZtailResult {
    std::string output;
};

// Obter as últimas N linhas de um arquivo comprimido
// Retorna ponteiro para string alocada (caller deve chamar ztail_free)
char* ztail_tail(const char* filename, int n_lines) {
    try {
        DetectionResult det = detectCompressionType(filename);
        if (det.type == CompressionType::NONE) {
            return nullptr;
        }

        CLIOptions opts;
        auto factories = buildCompressorFactoryMap();
        auto it = factories.find(det.type);
        if (it == factories.end()) {
            return nullptr;
        }

        auto compressor = it->second(
            std::move(det.file), filename, opts
        );

        CircularBuffer buffer(n_lines, opts.lineCapacity);
        Parser parser(buffer, opts.lineCapacity);

        std::vector<char> chunk(opts.readBufferSize);
        size_t bytesDecompressed = 0;

        while (compressor->decompress(chunk, bytesDecompressed)) {
            parser.parse(chunk.data(), bytesDecompressed);
        }
        parser.finalize();

        // Capturar saída em string
        // Nota: isto requer redirecionar stdout ou modificar
        // CircularBuffer para retornar string.
        // Aqui usamos um pipe interno:
        int pipefd[2];
        pipe(pipefd);

        int saved_stdout = dup(STDOUT_FILENO);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        buffer.print(opts.printAggregationThreshold);
        fflush(stdout);

        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);

        // Ler do pipe
        std::string result;
        char buf[4096];
        ssize_t r;
        while ((r = read(pipefd[0], buf, sizeof(buf))) > 0) {
            result.append(buf, r);
        }
        close(pipefd[0]);

        char* out = static_cast<char*>(malloc(result.size() + 1));
        if (out) {
            memcpy(out, result.c_str(), result.size() + 1);
        }
        return out;

    } catch (...) {
        return nullptr;
    }
}

// Detectar tipo de compressão
const char* ztail_detect_format(const char* filename) {
    try {
        DetectionResult det = detectCompressionType(filename);
        return compressionTypeName(det.type);
    } catch (...) {
        return "unknown";
    }
}

// Liberar memória alocada por ztail_tail
void ztail_free(char* ptr) {
    free(ptr);
}

}  // extern "C"
```

### Passo 2: Compilar como shared library

```bash
cd ztail/build
cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=ON
make -j$(nproc)

# Compilar o wrapper como .so
g++ -std=c++17 -shared -fPIC -O2 \
    -I../src \
    ../ztail_python_wrapper.cpp \
    -L. -lztail_lib \
    -lz -lbz2 -llzma -lzstd -lzip \
    -o libztail_python.so
```

### Passo 3: Usar com ctypes em Python

```python
import ctypes
from pathlib import Path

# Carregar a biblioteca
lib = ctypes.CDLL("./libztail_python.so")

# Definir tipos de retorno
lib.ztail_tail.restype = ctypes.c_char_p
lib.ztail_tail.argtypes = [ctypes.c_char_p, ctypes.c_int]

lib.ztail_detect_format.restype = ctypes.c_char_p
lib.ztail_detect_format.argtypes = [ctypes.c_char_p]

lib.ztail_free.restype = None
lib.ztail_free.argtypes = [ctypes.c_char_p]


def ztail(filename: str, n: int = 10) -> str | None:
    """Retorna as últimas N linhas de um arquivo comprimido."""
    result = lib.ztail_tail(filename.encode(), n)
    if result is None:
        return None
    try:
        return result.decode("utf-8")
    finally:
        lib.ztail_free(result)


def detect_format(filename: str) -> str:
    """Detecta o formato de compressão de um arquivo."""
    result = lib.ztail_detect_format(filename.encode())
    return result.decode("utf-8")


# Uso
fmt = detect_format("dados.gz")
print(f"Formato: {fmt}")  # "gzip"

linhas = ztail("dados.gz", 20)
if linhas:
    print(linhas)
```

## Opção 3: Extensão Nativa com pybind11

Para uma integração mais elegante e Pythonic, use `pybind11`.

### Passo 1: Instalar pybind11

```bash
pip install pybind11
```

### Passo 2: Criar o módulo de binding

Crie `ztail_pybind.cpp`:

```cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "compression_type.h"
#include "compressor_factory.h"
#include "parser.h"
#include "circular_buffer.h"
#include "cli.h"

#include <sstream>
#include <vector>

namespace py = pybind11;

// Classe wrapper para uso em Python
class ZtailReader {
public:
    explicit ZtailReader(const std::string& filename)
        : filename_(filename)
    {
        DetectionResult det = detectCompressionType(filename);
        format_ = det.type;

        auto factories = buildCompressorFactoryMap();
        auto it = factories.find(det.type);
        if (it == factories.end()) {
            throw std::runtime_error(
                "Formato não suportado: " +
                std::string(compressionTypeName(det.type))
            );
        }

        CLIOptions opts;
        compressor_ = it->second(std::move(det.file), filename, opts);
    }

    std::string format_name() const {
        return compressionTypeName(format_);
    }

    // Descomprimir e retornar todas as últimas N linhas
    std::vector<std::string> tail(int n) {
        CLIOptions opts;
        CircularBuffer buffer(n, opts.lineCapacity);
        Parser parser(buffer, opts.lineCapacity);

        std::vector<char> chunk(opts.readBufferSize);
        size_t bytesDecompressed = 0;

        while (compressor_->decompress(chunk, bytesDecompressed)) {
            parser.parse(chunk.data(), bytesDecompressed);
        }
        parser.finalize();

        // Capturar saída
        // Nota: idealmente CircularBuffer teria um método
        // para retornar as linhas como vector<string>
        std::vector<std::string> lines;
        // ... implementação depende da API do CircularBuffer
        return lines;
    }

private:
    std::string filename_;
    CompressionType format_;
    std::unique_ptr<ICompressor> compressor_;
};

// Função simples para detectar formato
std::string detect_format(const std::string& filename) {
    DetectionResult det = detectCompressionType(filename);
    return compressionTypeName(det.type);
}

PYBIND11_MODULE(ztail, m) {
    m.doc() = "Bindings Python para ztail - tail de arquivos comprimidos";

    // Enum de tipos de compressão
    py::enum_<CompressionType>(m, "CompressionType")
        .value("NONE", CompressionType::NONE)
        .value("GZIP", CompressionType::GZIP)
        .value("BZIP2", CompressionType::BZIP2)
        .value("XZ", CompressionType::XZ)
        .value("ZIP", CompressionType::ZIP)
        .value("ZSTD", CompressionType::ZSTD)
        .value("LZ4", CompressionType::LZ4)
        .value("TAR", CompressionType::TAR);

    // Função de detecção
    m.def("detect_format", &detect_format,
          "Detecta o formato de compressão de um arquivo",
          py::arg("filename"));

    // Classe reader
    py::class_<ZtailReader>(m, "ZtailReader")
        .def(py::init<const std::string&>(), py::arg("filename"))
        .def("format_name", &ZtailReader::format_name)
        .def("tail", &ZtailReader::tail, py::arg("n") = 10);
}
```

### Passo 3: Compilar com CMake

Crie um `CMakeLists.txt` para o binding:

```cmake
cmake_minimum_required(VERSION 3.16)
project(ztail_python LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(pybind11 REQUIRED)

# Adicionar ztail como subdirectory
add_subdirectory(vendor/ztail)

pybind11_add_module(ztail ztail_pybind.cpp)
target_link_libraries(ztail PRIVATE ztail_lib)
```

Ou compile diretamente:

```bash
c++ -O2 -shared -std=c++17 -fPIC \
    $(python3 -m pybind11 --includes) \
    -I/path/to/ztail/src \
    ztail_pybind.cpp \
    -L/path/to/ztail/build -lztail_lib \
    -lz -lbz2 -llzma -lzstd -lzip \
    -o ztail$(python3-config --extension-suffix)
```

### Passo 4: Usar em Python

```python
import ztail

# Detectar formato
fmt = ztail.detect_format("dados.gz")
print(f"Formato: {fmt}")

# Criar reader e ler últimas linhas
reader = ztail.ZtailReader("logs/app.log.zst")
print(f"Formato: {reader.format_name()}")

linhas = reader.tail(50)
for linha in linhas:
    print(linha)
```

## Comparação das Abordagens

| Aspecto | subprocess | ctypes | pybind11 |
|---------|-----------|--------|----------|
| Complexidade | Baixa | Média | Alta |
| Performance | Boa (I/O bound) | Ótima | Ótima |
| Dependências | Apenas ztail instalado | Shared lib compilada | pybind11 + compilação |
| Instalação | `apt install` ou build | Build customizado | Build customizado |
| API Pythonic | Não | Parcial | Sim |
| Tratamento de erros | Via exit code | Via retorno null | Exceções Python |

### Recomendação

- **Para scripts simples e automação**: use `subprocess` — é a abordagem mais simples
  e não requer compilação adicional.
- **Para uso em pipelines de dados de alta performance**: use `ctypes` ou `pybind11`,
  dependendo do nível de integração desejado.
- **Para distribuição como pacote Python**: use `pybind11` com `setuptools` para
  criar um pacote instalável via `pip`.
