# Build Instructions

## Prerequisites

- CMake 3.10 or later
- C compiler supporting C11 (GCC, Clang, MSVC)
- Optional: Python, GnuWin32 (Windows)

## Build with CMake (Recommended)

### Linux

**Release Build:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

**Debug Build:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j
```

### Windows (Visual Studio)

**Visual Studio 2019:**
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019"
```
Open the generated `.sln` file in Visual Studio.

**Visual Studio 2017:**
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 15 2017 Win64"
```

### macOS

**Using Xcode:**
```bash
mkdir build && cd build
cmake .. -G "Xcode"
```

**Using Makefile with Homebrew GCC:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-11 -DCMAKE_CXX_COMPILER=g++-11
make -j
```

## Build with Make

```bash
make all
```

For MSYS2/MinGW:
```bash
make all toolset=gcc
```

## CMake Options

| Option | Description | Default |
|--------|-------------|---------|
| `CMAKE_BUILD_TYPE` | Build type (Release/Debug/RelWithDebInfo/MinSizeRel) | Release |
| `USE_ADDRESS_SANITIZER` | Enable AddressSanitizer (Linux GCC only) | OFF |
| `ENABLE_SPLIT_PARALLELISM` | Enable split parallelism | OFF |
| `ENABLE_WPP_PARALLELISM` | Enable WPP parallelism | OFF |

## Build Outputs

After building, the following executables are generated:

| Executable | Description |
|------------|-------------|
| `lencod` | H.264/AVC encoder |
| `ldecod` | H.264/AVC decoder |
| `rtpdump` | RTP packet dump tool |
| `rtploss` | RTP packet loss simulator |

## Troubleshooting

### AddressSanitizer (Linux)

To enable AddressSanitizer for debugging:
```bash
cmake .. -DUSE_ADDRESS_SANITIZER=ON
make -j
```

### Static Build

```bash
cmake .. -DBUILD_STATIC=ON
make -j
```
