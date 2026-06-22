# H.264 `ut_wd_intra` Support

This directory contains the JM/H.264 additions for the HM `ut_wd_intra` directed test flow.

The H.264 dump reader is separate from HM's HEVC dump reader because the JM dump wire format is MB-based and has different TLV payloads. Symbols are kept under `H264IntraDump` or `h264_directed_*` so the code can be included with the existing HM directed implementation without name conflicts.

## Files

- `H264IntraDumpReader.h` / `H264IntraDumpReader.cc`: JM intra dump TLV reader.
- `ut_intra_directed_h264.cc`: H.264 directed-mode bridge used by `JVET_HM/ut_wd_intra/main.cc`.

## Quick Checks

Build the reader with warnings as errors:

```bash
make -C ut_wd_intra h264-reader-check
```

Run a reader smoke test on an existing JM post-order dump:

```bash
make -C ut_wd_intra h264-reader-smoke DUMP=../dump_output_cases/akiyo/intra_dump_postorder.bin
```

## Full HM UT Build

`JVET_HM/ut_wd_intra/main.cc` now dispatches directed mode by codec:

- `--codec hevc`: existing HM `directed_*` path.
- `--codec h264`: new `h264_directed_*` path.

The full HM UT executable still depends on the external VCE emulator headers and libraries, such as `vce/emul/vceemul.h`, which are not stored in this JM repository.