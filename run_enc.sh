#!/bin/bash

CASE=akiyo_64x64
FRAMES=1

ALL_CASES="akiyo_64x64 akiyo_cif city_256x256 city_352x288 football_128x128"

usage() {
    echo "Usage: $0 [-c case] [-n frames] [-h]"
    echo "  -c case    Test case: akiyo_64x64 (default), akiyo_cif, city_256x256, city_352x288, football_128x128, all"
    echo "  -n frames  Number of frames to encode (default: 1)"
    echo "  -h         Show this help"
    exit 0
}

while getopts "c:n:h" opt; do
    case "$opt" in
        c) CASE=$OPTARG ;;
        n) FRAMES=$OPTARG ;;
        h) usage ;;
        *) usage ;;
    esac
done

setup_case() {
    local c=$1
    case "$c" in
        akiyo_64x64)
            INPUT_FILE=./sequences/akiyo_64x64.yuv
            SOURCE_WIDTH=64
            SOURCE_HEIGHT=64
            BITSTREAM_FILE=./stream_akiyo_64x64.264
            ;;
        akiyo_cif)
            INPUT_FILE=./sequences/akiyo_cif.yuv
            SOURCE_WIDTH=352
            SOURCE_HEIGHT=288
            BITSTREAM_FILE=./stream_akiyo_cif.264
            ;;
        city_256x256)
            INPUT_FILE=./sequences/city_256x256_10f_420p.yuv
            SOURCE_WIDTH=256
            SOURCE_HEIGHT=256
            BITSTREAM_FILE=./stream_city_256x256.264
            ;;
        city_352x288)
            INPUT_FILE=./sequences/city_352x288_10f_420p.yuv
            SOURCE_WIDTH=352
            SOURCE_HEIGHT=288
            BITSTREAM_FILE=./stream_city_352x288.264
            ;;
        football_128x128)
            INPUT_FILE=./sequences/football_128x128_420p.yuv
            SOURCE_WIDTH=128
            SOURCE_HEIGHT=128
            BITSTREAM_FILE=./stream_football_128x128.264
            ;;
        *)
            echo "Unknown case: $c"
            echo "Available cases: ${ALL_CASES} all"
            return 1
            ;;
    esac
}

export JM_INTRA_DUMP_DIR=dump_output
ENCODER_BIN=./bin/umake/gcc-13.3/x86_64/debug/lencod

run_encode() {
    local c=$1
    setup_case "$c" || return 1
    local encoder_bin=${ENCODER_BIN}
    if [ ! -x "${encoder_bin}" ]; then
        encoder_bin=./bin/lencod_static
    fi

    echo "Running test case: $c (${SOURCE_WIDTH}x${SOURCE_HEIGHT})"    

    rm -fr "${JM_INTRA_DUMP_DIR}"
    mkdir -p "${JM_INTRA_DUMP_DIR}"

    "${encoder_bin}" -d cfg/encoder.cfg \
        -p InputFile=${INPUT_FILE} \
        -p SourceWidth=${SOURCE_WIDTH} \
        -p SourceHeight=${SOURCE_HEIGHT} \
        -p FrameToBeEncoded=${FRAMES} \
        -p IntraPeriod=1 \
        -p OutputFile=${BITSTREAM_FILE}

    md5sum ${BITSTREAM_FILE}

    local CASE_DIR=dump_output_cases/${c}
    mkdir -p dump_output_cases
    rm -fr "${CASE_DIR}"
    mv "${JM_INTRA_DUMP_DIR}" "${CASE_DIR}"

    local BITSTREAM_BASENAME
    BITSTREAM_BASENAME=$(basename "${BITSTREAM_FILE}")
    if [ -f "${BITSTREAM_FILE}" ]; then
        mv "${BITSTREAM_FILE}" "${CASE_DIR}/${BITSTREAM_BASENAME}"
    fi

    local EXTRA_FILE
    for EXTRA_FILE in log.dat stats.dat test_rec.yuv data.txt; do
        if [ -f "${EXTRA_FILE}" ]; then
            mv "${EXTRA_FILE}" "${CASE_DIR}/"
        fi
    done

    if [ -f "${INPUT_FILE}" ]; then
        local INPUT_BASENAME
        INPUT_BASENAME=$(basename "${INPUT_FILE}")
        cp -f "${INPUT_FILE}" "${CASE_DIR}/${INPUT_BASENAME}"
    fi

    python3 tools/verify_dump.py --pixels ${CASE_DIR}/intra_dump.bin > ${CASE_DIR}/dump_log.txt
    python3 tools/verify_dump.py --pixels ${CASE_DIR}/intra_dump_postorder.bin > ${CASE_DIR}/dump_postorder_log.txt
    cat ${CASE_DIR}/dump_log.txt
    cat ${CASE_DIR}/dump_postorder_log.txt
}

#if [ ! -f ./bin/lencod ]; then
    echo "Building lencod..."
    mkdir -p build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    make -j
    cd ..
#fi

if [ "$CASE" = "all" ]; then
    for c in ${ALL_CASES}; do
        run_encode "$c"
    done
else
    run_encode "$CASE"
fi
