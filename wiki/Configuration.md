# Configuration Files

## Overview

The JM software uses configuration files to specify encoding/decoding parameters. These are text files with key-value pairs.

## Encoder Configuration

### Basic Structure

```
# Comment
ParameterName Value

######### Profile/Level #########
ProfileIDC                 100
LevelIDC                   30

######### Video Source #########
SourceWidth                1920
SourceHeight               1080
FrameRate                  30.0
```

### Key Parameters

#### Video Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `SourceWidth` | int | Frame width in pixels |
| `SourceHeight` | int | Frame height in pixels |
| `FrameRate` | float | Frame rate in fps |
| `FramesToBeEncoded` | int | Number of frames to encode |
| `ProfileIDC` | int | H.264 profile (66/77/88/100/110/122/244) |
| `LevelIDC` | int | H.264 level (10-51) |

#### GOP Structure

| Parameter | Type | Description |
|-----------|------|-------------|
| `IntraPeriod` | int | I-frame interval (0=all intra, -1=only first) |
| `NumBFrames` | int | Number of B-frames between P/I frames |
| `GOPSize` | int | GOP size (if explicit GOP used) |

#### Rate Control

| Parameter | Type | Description |
|-----------|------|-------------|
| `RateControlEnable` | int | 0: off, 1: on |
| `Bitrate` | int | Target bitrate (bps) |
| `QPFirstFrame` | int | Initial QP (0-51) |
| `QPISlice` | int | QP for I-slices |
| `QPPSlice` | int | QP for P-slices |
| `QPBSlice` | int | QP for B-slices |

#### Motion Estimation

| Parameter | Type | Description |
|-----------|------|-------------|
| `SearchMode` | int | 0: Full, 1: EPZS, 2: UMHex, 3: UMHexSMP, 4: HME |
| `SearchRange` | int | Search range in pixels |
| `NumberReferenceFrames` | int | Max reference frames |
| `MEMethod` | int | ME refinement method |

#### Encoding Tools

| Parameter | Type | Description |
|-----------|------|-------------|
| `SymbolModeEntropyCoding` | int | 0: CAVLC, 1: CABAC |
| `Transform8x8Mode` | int | 8x8 transform (0: off, 1: on, 2: adaptive) |
| `DeblockingFilterControl` | int | Deblocking filter control |
| `WeightedPrediction` | int | 0: off, 1: explicit, 2: implicit |

### Sample Encoder Configurations

#### Baseline Profile (Low Complexity)
```bash
./lencod -d encoder.cfg \
  -p ProfileIDC=66 \
  -p LevelIDC=30 \
  -p SymbolModeEntropyCoding=0 \
  -p Transform8x8Mode=0 \
  -p IntraPeriod=16 \
  -p NumBFrames=0
```

#### Main Profile
```bash
./lencod -d encoder.cfg \
  -p ProfileIDC=77 \
  -p LevelIDC=30 \
  -p SymbolModeEntropyCoding=1 \
  -p IntraPeriod=16 \
  -p NumBFrames=3
```

#### High Profile
```bash
./lencod -d encoder.cfg \
  -p ProfileIDC=100 \
  -p LevelIDC=40 \
  -p SymbolModeEntropyCoding=1 \
  -p Transform8x8Mode=2 \
  -p IntraPeriod=16 \
  -p NumBFrames=3
```

## Decoder Configuration

### Key Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `InputFile` | string | Input bitstream file |
| `OutputFile` | string | Output YUV file |
| `RefFileMode` | int | Reference file mode |
| `DecodeAllFrames` | int | Decode all frames |
| `ErrorConcealment` | int | Error concealment mode |

## Configuration File Formats

### HM-compatible Configs

Located in `cfg/HM-like/`:
- `encoder_intra_main.cfg` - All intra
- `encoder_lowdelay_main.cfg` - Low delay P
- `encoder_lowdelay_P_main.cfg` - Low delay P
- `encoder_randomaccess_main.cfg` - Random access

### Test Sequences

Common test sequences used with JM:
- `foreman` - QCIF (176x144)
- `container` - QCIF
- `mobile` - CIF (352x288)
- `tempet` - CIF
- `crew` - 4CIF (704x576)
- `harbour` - 4CIF
