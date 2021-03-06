#!/bin/bash

CLEANUP=1
ARGS=""

while [[ -n "$1" && "$1" != *.c && "$1" != *.cpp ]]; do
    if [ "$1" = "--no-cleanup" ]; then
        CLEANUP=0
    else
        ARGS="$ARGS $1"
    fi
    shift
done

if [ -z "$1" ]; then
    echo "Usage: objectize.sh [args] <c-file> [object-file]"
    echo "Args:"
    echo "  --no-cleanup    Don't clean up afterwards leaving the tmp files in /tmp"
    echo "  All other arguments are directly passed to the initial compilation of the c source"
    exit 1
fi

SRC="$1"

if [ -z "$2" ]; then
    BN_SRC=$(basename "$SRC")
    DEST="${BN_SRC%.*}".o
else
    DEST="$2"
fi

DEST_BASE=/tmp/`basename "$DEST" .o`

if [ $CLEANUP = 1 ]; then
    # generate unique id
    BASE="${DEST_BASE}_$$"
else
    BASE="$DEST_BASE"
fi

BC1="$BASE"
BC2="${BASE}.instr"

# if we don't clean up generate human readable llvm IR instead of bitcode
if [ $CLEANUP = 1 ]; then
    CLANG_LLVM_FORMAT_ARG="-c"
    OPT_LLVM_FORMAT_ARG=""

    BC1="$BC1.bc"
    BC2="$BC2.bc"
else
    CLANG_LLVM_FORMAT_ARG="-S"
    OPT_LLVM_FORMAT_ARG="-S"

    BC1="$BC1.ll"
    BC2="$BC2.ll"
fi

clang -g -emit-llvm $CLANG_LLVM_FORMAT_ARG $ARGS -o "$BC1" "$SRC" && \
    opt $OPT_LLVM_FORMAT_ARG -load $(dirname $BASH_SOURCE)/ASBDetection/libLLVMasbDetection.so -asb-detection -asb-log-level 0 < "$BC1" > "$BC2" && \
    clang -g -O0 -c -o "$DEST" "$BC2"

if [ $CLEANUP = 1 ]; then
    rm -f "$BC1" "$BC2"
fi
