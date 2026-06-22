# AGENTS.md

## What This Is

H.264/AVC reference software (JM) — the official reference implementation of the H.264 video coding standard. Written in C (C11).

## Build

**Quick build (CMake, recommended):**
```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

**All variants at once (via Make wrapper):**
```bash
make all
```

**Single target with variant suffix** (a=all, r=release, d=debug, p=relwithdebinfo):
```bash
make lencod-r    # release encoder only
make ldecod-d    # debug decoder only
```

**CI builds with Ninja** (for reference — use Make or raw CMake locally):
```bash
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -S . -B build/debug
cmake --build build/debug --parallel $(nproc)
```

## Build Outputs

Four executables: `lencod` (encoder), `ldecod` (decoder), `rtpdump`, `rtploss`.

## CMake Options Worth Knowing

| Option | Description |
|---|---|
| `USE_ADDRESS_SANITIZER=ON` | ASan (Linux GCC only) |
| `ENABLE_TRACING=ON` | Tracing support |
| `BUILD_STATIC=ON` | Static linking |

## Critical: Warnings Are Errors

GCC, Clang, and MSVC all compile with `-Werror` (warnings-as-errors). Any code change must compile cleanly with the project's warning flags. If you see a build failure on CI that you can't reproduce locally, check your compiler version — warning flags are version-dependent in `CMakeLists.txt`.

## Architecture

```
source/app/lencod/   # Encoder (~213 files)
source/app/ldecod/   # Decoder (~92 files)
source/app/rtpdump/  # RTP dump tool
source/app/rtploss/  # RTP loss simulator
source/lib/lcommon/  # Shared library (image I/O, memory, transforms, NAL)
cfg/                 # Encoder/decoder configuration files (.cfg)
```

Entry points: `source/app/lencod/lencod.c`, `source/app/ldecod/ldecod.c`.

Key data structures (`global.h` in both lencod/ and ldecod/): `ImageParameters`, `InputParameters`, `VideoParameters`.

## What's Not Here

- **No test suite.** `decoder_test.c` is the decoder's main, not a test.
- **No linter/formatter.** Follow existing code style.
- **No type checking toolchain.** This is pure C.
