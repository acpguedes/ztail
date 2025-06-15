# ztail

**ztail** is an efficient command-line tool to display the last N lines of compressed `.gz`, `.bz2`, and `.zip` files. Designed for high performance, **ztail** is ideal for working with large compressed text files.

## 🛠️ **Features**

- **Supports `.gz`, `.bz2`, and `.zip` Files:** Decompresses and processes these compressed file formats efficiently.
- **High Performance:** Optimized for large files, avoiding unnecessary full decompressions.
- **Intuitive Command-Line Interface:** Simple usage with flexible options.

## 📦 **Installation**

### 📋 **Prerequisites**

- **C++17** or higher.
- **CMake** (version 3.10 or higher).
- **zlib Library:** For decompressing `.gz` and `.bgz` files.
- **bzip2 Library:** For handling `.bz2` files.
- **libzip Library:** For handling `.zip` files.

On Ubuntu/Debian the required packages can typically be installed with:

```bash
sudo apt install zlib1g-dev libbz2-dev libzip-dev
```

Library names may vary on other operating systems.

### 🔧 **Build Steps**

1. **Clone the Repository:**

   ```bash
   git clone https://github.com/acpguedes/ztail.git
   cd ztail
   ```

2. **Create a Build Directory and Navigate Into It:**

   ```bash
   mkdir build
   cd build
   ```

3. **Configure the Project with CMake:**

   ```bash
   cmake .. -DBUILD_TESTING=ON
   ```

4. **Build the Project:**

   ```bash
   make
   ```

   This will generate the `ztail` executable inside the `build` directory.

## 🚀 **Usage**

### **Display the Last N Lines of a `.gz`, `.bz2`, or `.zip` File**

```bash
./ztail -n 2 file.gz
./ztail -n 2 file.bz2
./ztail -n 2 --entry second.txt file.zip
```

- **`-n N`**: Display the last N lines (default = 10).
- **`--entry name`**: For zip files, read `name` instead of the first entry.
- **`file.gz`, `file.bz2`, or `file.zip`**: Name of the compressed file.

## 🧪 **Tests**

Unit tests are located in the `tests/` directory. To run them:

1. **Ensure the project was configured with testing enabled:**

   ```bash
   cmake .. -DBUILD_TESTING=ON
   ```

2. **Navigate to the Build Directory:**

   ```bash
   cd build
   ```

3. **Execute the Tests:**

   ```bash
   ctest --output-on-failure
   ```

## 📝 **Contributing**

Contributions are welcome! To contribute:

1. **Fork the Repository.**
2. **Create a Branch for Your Feature or Bug Fix:**

   ```bash
   git checkout -b feature/new-feature
   ```

3. **Make Your Changes and Commit:**

   ```bash
   git commit -m "feat: Add new feature"
   ```

   Recomendation: Use [Conventional commits](https://www.conventionalcommits.org/en/v1.0.0/).

4. **Push to Your Fork:**

   ```bash
   git push origin feature/new-feature
   ```

5. **Open a Pull Request on the Original Repository.**

Please ensure that your contributions adhere to the project's coding standards and pass all tests.

## 📜 **License**

This project is licensed under the [Apache License 2.0](LICENSE) - see the [LICENSE](LICENSE) file for details.

## 🛡️ **Security Considerations**

- **Input Validation:** Ensure file paths and inputs are validated to prevent vulnerabilities such as path traversal.
- **Error Handling:** Robust error handling to manage unexpected scenarios gracefully.
- **Dependency Management:** Keep dependencies up-to-date to mitigate potential security risks.

---

Feel free to reach out if you have any questions or need further assistance!