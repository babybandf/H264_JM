# H.264/AVC Encoder

## Overview

The JM encoder (`lencod`) implements the complete H.264/AVC encoding process, supporting Baseline, Main, and High profiles.

## Usage

```bash
./lencod -d <config_file> [-p <parameter>=<value>] ...
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `-d <file>` | Configuration file |
| `-p <param>=<value>` | Override configuration parameter |
| `-h` | Display help |

### Example

```bash
./lencod -d encoder.cfg -p FrameRate=30 -p OutputFile=output.264
```

## Profiles Supported

| Profile | Features |
|---------|----------|
| Baseline | CAVLC, no B-slices, no CABAC |
| Main | CABAC, B-slices, interlace |
| High | 8x8 transform, custom quant matrices |

## Key Encoding Parameters

### Video Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `SourceWidth` | Source width in pixels | - |
| `SourceHeight` | Source height in pixels | - |
| `FrameRate` | Frame rate (fps) | 30 |
| `FramesToBeEncoded` | Number of frames to encode | - |
| `ProfileIDC` | Profile indicator | 100 |
| `LevelIDC` | Level indicator | 30 |

### Rate Control

| Parameter | Description | Default |
|-----------|-------------|---------|
| `RateControlEnable` | Enable rate control | 0 |
| `Bitrate` | Target bitrate (bps) | 100000 |
| `QPFirstFrame` | QP for first frame | 24 |
| `QPISlice` | QP for I-slices | 28 |
| `QPPSlice` | QP for P-slices | 30 |

### Motion Estimation

| Parameter | Description | Default |
|-----------|-------------|---------|
| `SearchMode` | ME algorithm (0: full, 1: EPZS, 2: UMHex, 3: UMHexSMP) | 1 |
| `SearchRange` | Search range in pixels | 16 |
| `NumberReferenceFrames` | Number of reference frames | 1 |

### Encoding Tools

| Parameter | Description | Default |
|-----------|-------------|---------|
| `SymbolModeEntropyCoding` | 0: CAVLC, 1: CABAC | 0 |
| `Transform8x8Mode` | 8x8 transform enable | 0 |
| `DeblockingFilterControl` | Deblocking filter control | 1 |

## Encoding Modes

### All Intra (AI)
```bash
# Set IntraPeriod=1 in config
./lencod -d encoder.cfg -p IntraPeriod=1
```

### Low Delay B (LDB)
```bash
# Set IntraPeriod and NumBFrames
./lencod -d encoder.cfg -p IntraPeriod=16 -p NumBFrames=3
```

### Random Access (RA)
```bash
# Configure GOP structure
./lencod -d encoder.cfg -p IntraPeriod=16 -p NumBFrames=3
```

## Configuration Files

| File | Description |
|------|-------------|
| `encoder.cfg` | Default configuration |
| `encoder_baseline.cfg` | Baseline profile |
| `encoder_main.cfg` | Main profile |
| `encoder_extended.cfg` | Extended profile |
| `encoder_stereo.cfg` | Stereo/MVC encoding |

## Output

The encoder produces:
- H.264/AVC bitstream (`.264` or `.26l`)
- Reconstruction file (optional, `.yuv`)
- Statistics file (optional)
