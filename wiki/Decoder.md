# H.264/AVC Decoder

## Overview

The JM decoder (`ldecod`) implements the complete H.264/AVC decoding process, supporting all profiles and features.

## Usage

```bash
./ldecod -d <config_file> [-p <parameter>=<value>] ...
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `-d <file>` | Configuration file |
| `-p <param>=<value>` | Override configuration parameter |
| `-h` | Display help |

### Example

```bash
./ldecod -d decoder.cfg -p InputFile=input.264 -p OutputFile=output.yuv
```

## Key Decoding Parameters

### Input/Output

| Parameter | Description | Default |
|-----------|-------------|---------|
| `InputFile` | Input bitstream file | - |
| `OutputFile` | Output YUV file | - |
| `RefFileMode` | Reference file mode (0-2) | 0 |

### Decoding Options

| Parameter | Description | Default |
|-----------|-------------|---------|
| `DecodeAllFrames` | Decode all frames | 0 |
| `ErrorConcealment` | Error concealment mode (0-3) | 0 |

## Error Concealment Modes

| Mode | Description |
|------|-------------|
| 0 | No error concealment |
| 1 | Slice concealment |
| 2 | Frame concealment |
| 3 | Advanced concealment |

## Features

- Full H.264/AVC Baseline, Main, and High profile decoding
- CABAC and CAVLC entropy decoding
- Deblocking filter
- Multiple reference frame support
- B-slice support
- Interlace (MBAFF) support
- MVC (Multiview Video Coding) support
- Error concealment

## Output Formats

| Format | Description |
|--------|-------------|
| YUV 4:2:0 | Default output |
| YUV 4:2:2 | With appropriate config |
| YUV 4:4:4 | High profile output |

## Configuration Files

| File | Description |
|------|-------------|
| `decoder.cfg` | Default decoder configuration |
| `decoder_stereo.cfg` | Stereo/MVC decoding |

## Statistics Output

The decoder can output decoding statistics including:
- Bits per frame
- Decode time
- Error statistics
- MB type distribution

Enable with:
```bash
./ldecod -d decoder.cfg -p WriteStat=1
```
