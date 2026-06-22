#!/usr/bin/env python3
"""
verify_dump.py - Parse and verify the JM intra dump binary file.
Checks structural integrity of TLV records and prints a summary.
"""

import struct
import sys
from collections import defaultdict

TAGS = {
    0x0001: 'SEQ_HEADER',
    0x0010: 'MB_BEGIN',
    0x0011: 'MB_END',
    0x0020: 'BLOCK_BEGIN',
    0x0021: 'BLOCK_END',
    0x0030: 'REF_SAMPLES',
    0x0031: 'RECON_PELS',
    0x0032: 'PRED_PELS',
    0x0040: 'MODE_METRIC',
    0x0041: 'FINAL_MODE',
}

MODE_KINDS = {
    0: 'dc', 1: 'hor', 2: 'ver', 3: 'plane',
    4: 'd4', 5: 'd4r', 6: 'vr', 7: 'hd', 8: 'vl', 9: 'hu'
}

def parse_seq_header(data):
    fields = struct.unpack_from('<IIIIIIIIx', data, 0)
    return {
        'picW': fields[0], 'picH': fields[1], 'mbSize': fields[2],
        'chromaFmt': fields[3], 'bitDepthL': fields[4], 'bitDepthC': fields[5],
        'baseQp': fields[6], 'useDqp': fields[7]
    }

def parse_mb_key(data):
    fields = struct.unpack_from('<IIIIIbxxx', data, 0)
    return {
        'picIdx': fields[0], 'mbAddrX': fields[1],
        'mbPelX': fields[2], 'mbPelY': fields[3],
        'sliceType': fields[4], 'sliceQp': fields[5]
    }

def parse_block_key(data):
    fields = struct.unpack_from('<IIIIIIIII', data, 0)
    extra = data[36:44]
    return {
        'blockUid': fields[0], 'parentUid': fields[1],
        'mbAddrX': fields[2], 'mbPelX': fields[3], 'mbPelY': fields[4],
        'blkPelXInMb': fields[5], 'blkPelYInMb': fields[6],
        'width': fields[7], 'height': fields[8],
        'compID': extra[0] if len(extra) > 0 else 0,
        'mbMode': extra[1] if len(extra) > 1 else 0,
        'intraMode': extra[2] if len(extra) > 2 else 0,
        'partIdx': extra[3] if len(extra) > 3 else 0,
    }

def parse_mode_metric(data):
    fields = struct.unpack_from('<IBBxxQ', data, 0)
    return {
        'modeId': fields[0], 'modeKind': fields[1],
        'mbMode': fields[2], 'distortionSatd': fields[3]
    }

def parse_final_mode(data):
    fields = struct.unpack_from('<IBBxxQ', data, 0)
    return {
        'modeId': fields[0], 'modeKind': fields[1],
        'mbMode': fields[2], 'distortionSatd': fields[3]
    }

def parse_pred_pels(data):
    hdr_size = 4 + 1 + 4 + 4 + 8
    mode_id = struct.unpack_from('<I', data, 0)[0]
    mode_kind = struct.unpack_from('<B', data, 4)[0]
    w = struct.unpack_from('<I', data, 5)[0]
    h = struct.unpack_from('<I', data, 9)[0]
    satd = struct.unpack_from('<Q', data, 13)[0]
    return mode_id, mode_kind, w, h, satd, hdr_size

def main():
    import argparse
    parser = argparse.ArgumentParser(description='Verify JM intra dump file')
    parser.add_argument('dump_file', help='Path to intra_dump.bin')
    parser.add_argument('--quiet', '-q', action='store_true')
    parser.add_argument('--max-blocks', type=int, default=0,
                        help='Stop after printing N blocks (0=unlimited)')
    parser.add_argument('--pixels', '-p', action='store_true',
                        help='Print pixel values for REF_SAMPLES, RECON_PELS and PRED_PELS')
    args = parser.parse_args()

    with open(args.dump_file, 'rb') as f:
        data = f.read()

    pos = 0
    tag_counts = defaultdict(int)
    errors = []
    mb_count = 0
    block_nesting = 0
    printed_blocks = 0
    max_blocks = args.max_blocks
    show_pixels = args.pixels

    while pos < len(data):
        if pos + 8 > len(data):
            errors.append(f"Truncated TLV header at offset {pos}")
            break

        tag, payload_len = struct.unpack_from('<II', data, pos)
        pos += 8

        if pos + payload_len > len(data):
            errors.append(f"Truncated payload at offset {pos-8}")
            break

        payload = data[pos:pos+payload_len]
        pos += payload_len

        tag_name = TAGS.get(tag, f'UNKNOWN(0x{tag:04x})')
        tag_counts[tag_name] += 1

        if tag == 0x0010:
            mb = parse_mb_key(payload)
            mb_count += 1
            if not args.quiet:
                print(f"[MB_BEGIN] pic={mb['picIdx']}, addr={mb['mbAddrX']}, "
                      f"pos=({mb['mbPelX']},{mb['mbPelY']}), QP={mb['sliceQp']}")

        elif tag == 0x0020:
            blk = parse_block_key(payload)
            block_nesting += 1
            printed_blocks += 1
            if not args.quiet and (max_blocks == 0 or printed_blocks <= max_blocks):
                abs_x = blk['mbPelX'] + blk['blkPelXInMb']
                abs_y = blk['mbPelY'] + blk['blkPelYInMb']
                print(f"  [BLOCK uid={blk['blockUid']}] {blk['width']}x{blk['height']} "
                      f"pos=({abs_x},{abs_y}) inMb=({blk['blkPelXInMb']},{blk['blkPelYInMb']}) "
                      f"comp={blk['compID']} mbMode={blk['mbMode']}")

        elif tag == 0x0021:
            block_nesting -= 1
            if block_nesting < 0:
                errors.append("BLOCK_END without matching BLOCK_BEGIN")

        elif tag == 0x0030:
            filtered = struct.unpack_from('<B', payload, 0)[0]
            num_pels = (len(payload) - 1) // 2
            if not args.quiet and (max_blocks == 0 or printed_blocks <= max_blocks):
                print(f"    [REF_SAMPLES] filtered={filtered}, pels={num_pels}")
                if show_pixels and num_pels > 0:
                    pels = struct.unpack_from(f'<{num_pels}h', payload, 1)
                    print(f"      pels: {list(pels)}")

        elif tag == 0x0031:
            w, h = struct.unpack_from('<II', payload, 0)
            if not args.quiet and (max_blocks == 0 or printed_blocks <= max_blocks):
                print(f"    [RECON_PELS] {w}x{h}")
                if show_pixels and w > 0 and h > 0:
                    pels = struct.unpack_from(f'<{w*h}h', payload, 8)
                    for row in range(h):
                        print(f"      row{row:2d}: {list(pels[row*w:(row+1)*w])}")

        elif tag == 0x0032:
            modeId, modeKind, w, h, satd, hdr_size = parse_pred_pels(payload)
            kind_str = MODE_KINDS.get(modeKind, f'?{modeKind}')
            if not args.quiet and (max_blocks == 0 or printed_blocks <= max_blocks):
                print(f"    [PRED_PELS] mode={modeId} kind={kind_str} {w}x{h} satd={satd}")
                if show_pixels and w > 0 and h > 0:
                    pels = struct.unpack_from(f'<{w*h}h', payload, hdr_size)
                    for row in range(h):
                        print(f"      row{row:2d}: {list(pels[row*w:(row+1)*w])}")

        elif tag == 0x0040:
            m = parse_mode_metric(payload)
            if not args.quiet and (max_blocks == 0 or printed_blocks <= max_blocks):
                kind = MODE_KINDS.get(m['modeKind'], f'?{m["modeKind"]}')
                print(f"    [MODE_METRIC] mode={m['modeId']} kind={kind} "
                      f"satd={m['distortionSatd']}")

        elif tag == 0x0041:
            fm = parse_final_mode(payload)
            if not args.quiet and (max_blocks == 0 or printed_blocks <= max_blocks):
                kind = MODE_KINDS.get(fm['modeKind'], f'?{fm["modeKind"]}')
                print(f"    [FINAL_MODE] mode={fm['modeId']} kind={kind} "
                      f"satd={fm['distortionSatd']}")

    print("\n" + "=" * 50)
    print("SUMMARY")
    print("=" * 50)
    print(f"Total bytes: {pos}/{len(data)}")
    print(f"MBs: {mb_count}")
    print(f"\nTag counts:")
    for name, count in sorted(tag_counts.items()):
        print(f"  {name}: {count}")

    if block_nesting != 0:
        errors.append(f"Unmatched block nesting: {block_nesting}")

    if errors:
        print(f"\nERRORS ({len(errors)}):")
        for e in errors:
            print(f"  !! {e}")
        return 1
    else:
        print("\nAll checks PASSED.")
        return 0

if __name__ == '__main__':
    sys.exit(main())
