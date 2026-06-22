# H.264/AVC JM Reference Software

Welcome to the H.264/AVC Joint Model (JM) Reference Software Wiki.

## Overview

The JM software is the official reference software for the H.264/AVC video coding standard. It provides a complete implementation of the encoder and decoder, serving as a reference for conformance testing and algorithm development.

## Wiki Contents

| Page | Description |
|------|-------------|
| [Build Instructions](Build.md) | How to compile and build the software |
| [Architecture](Architecture.md) | Project structure and code organization |
| [Encoder](Encoder.md) | H.264/AVC encoder documentation |
| [Decoder](Decoder.md) | H.264/AVC decoder documentation |
| [Configuration](Configuration.md) | Configuration file parameters |

## Quick Start

### Build (Linux)
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

### Run Encoder
```bash
./lencod -d encoder.cfg
```

### Run Decoder
```bash
./ldecod -d decoder.cfg
```

## Project Information

- **Version**: JM 18.4 (FRExt)
- **Standard**: H.264/AVC (ITU-T H.264, ISO/IEC 14496-10)
- **Language**: C (C11 standard)
- **Build System**: CMake 3.10+

## Links

- [JM Software Homepage](http://iphome.hhi.de/suehring/tml)
- [Bug Reporting](https://ipbt.hhi.fraunhofer.de)
- [H.264/AVC Standard](https://www.itu.int/rec/T-REC-H.264)
