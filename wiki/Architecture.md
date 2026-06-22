# Project Architecture

## Directory Structure

```
H264_JM/
├── cmake/              # CMake build modules
├── cfg/                # Configuration files
│   ├── encoder.cfg         # Default encoder config
│   ├── encoder_baseline.cfg
│   ├── encoder_main.cfg
│   ├── decoder.cfg         # Default decoder config
│   └── HM-like/           # HM-compatible configs
├── doc/                # Documentation
├── source/
│   ├── app/
│   │   ├── lencod/     # Encoder source code
│   │   ├── ldecod/     # Decoder source code
│   │   ├── rtpdump/    # RTP dump tool
│   │   └── rtploss/    # RTP loss tool
│   └── lib/
│       └── lcommon/    # Common library
├── CMakeLists.txt      # Top-level CMake config
└── Makefile            # Alternative build
```

## Module Overview

### Encoder (lencod/)

The encoder implements the full H.264/AVC encoding pipeline:

| Module | Files | Description |
|--------|-------|-------------|
| Main | `lencod.c` | Entry point, main encoding loop |
| Motion Estimation | `me_epzs*.c`, `me_umhex*.c`, `me_hme.c`, `me_fullsearch.c` | Motion search algorithms |
| Mode Decision | `mode_decision*.c`, `rdopt.c` | Macroblock mode selection |
| Intra Prediction | `intra4x4.c`, `intra8x8.c`, `intra16x16.c` | Intra prediction modes |
| Transform | `transform8x8.c` | Integer transform |
| Quantization | `quant4x4*.c`, `quant8x8*.c`, `quantChroma*.c` | Quantization |
| Entropy Coding | `cabac.c`, `vlc.c` | CAVLC/CABAC |
| Loop Filter | `loopFilter.c`, `loop_filter_*.c` | Deblocking filter |
| Rate Control | `ratectl.c`, `rc_quadratic.c` | Rate control |
| Weighted Prediction | `wp_*.c` | Weighted prediction |
| SEI | `sei.c` | SEI message generation |
| Slice | `slice.c` | Slice encoding |

### Decoder (ldecod/)

The decoder implements the full H.264/AVC decoding pipeline:

| Module | Files | Description |
|--------|-------|-------------|
| Main | `ldecod.c` | Entry point |
| Inverse Transform | `transform8x8.c` | Inverse integer transform |
| Dequantization | `quant.c` | Dequantization |
| Intra Prediction | `intra*_pred*.c` | Intra prediction |
| Motion Compensation | `mc_prediction.c`, `mc_direct.c` | Motion compensation |
| Entropy Decoding | `read_comp_cabac.c`, `read_comp_cavlc.c` | CAVLC/CABAC decoding |
| Loop Filter | `loopFilter.c`, `loop_filter_*.c` | Deblocking filter |
| Error Concealment | `erc_*.c`, `errorconcealment.c` | Error resilience |

### Common Library (lcommon/)

Shared utilities used by both encoder and decoder:

| Module | Files | Description |
|--------|-------|-------------|
| Image I/O | `img_io.c`, `io_raw.c`, `io_tiff.c` | Image file handling |
| Memory | `memalloc.c` | Memory allocation |
| Transform | `transform.c` | Transform utilities |
| Motion | `mv_prediction.c` | Motion vector prediction |
| NAL | `nalucommon.c` | NAL unit handling |
| Parameters | `parsetcommon.c` | Parameter sets |

## Data Flow

### Encoder Pipeline

```
Input YUV → [Intra/Inter Prediction] → [Transform] → [Quantization] 
    → [Entropy Coding] → [NAL Unit Assembly] → Bitstream
    
    ↓ (reconstruction path)
[Inverse Quantization] → [Inverse Transform] → [Motion Compensation]
    → [Loop Filter] → [Reference Frame Buffer]
```

### Decoder Pipeline

```
Bitstream → [NAL Unit Parsing] → [Parameter Sets] 
    → [Entropy Decoding] → [Dequantization] → [Inverse Transform]
    → [Motion Compensation / Intra Prediction] → [Loop Filter] → Output YUV
```

## Key Data Structures

- `ImageParameters` (`global.h`): Main image/frame data structure
- `InputParameters` (`global.h`): Encoder/decoder configuration
- `Macroblock` (`macroblock.h`): Per-macroblock coding data
- `Slice` (`slice.h`): Slice-level data
- `VideoParameters` (`global.h`): Video sequence parameters
