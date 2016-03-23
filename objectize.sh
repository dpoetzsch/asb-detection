#!/bin/sh

ARGS=""

while [[ -n "$1" && "$1" != *.c ]]; do
    ARGS="$ARGS $1"
    shift
done

if [ -z "$1" ]; then
    echo "Usage: objectize.sh [compilation arguments] <c-file> [object-file]"
    exit 1
fi

SRC="$1"
SRC_BASE=`basename "$SRC" .c`

if [ -z "$2" ]; then
    DEST=`dirname "$SRC"`/"$SRC_BASE".o
else
    DEST="$2"
fi

# generate random id
#RID=`cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 4 | head -n 1`
#BASE="$SRC_BASE_$RID"
BASE="$SRC_BASE"

BC1="/tmp/$BASE.bc"
BC2="/tmp/${BASE}_instr.bc"

echo "clang -emit-llvm -c $ARGS -o \"$BC1\" \"$SRC\""
clang -emit-llvm -c $ARGS -o "$BC1" "$SRC"

if [ $? = 0 ]; then
    echo "opt -S -load $(dirname $0)/ASBDetection/libLLVMasbDetection.so -asb_detection -asb-log-level 0 -asb_detection_instr_only < \"$BC1\" > \"$BC2\""
    opt -S -load $(dirname $0)/ASBDetection/libLLVMasbDetection.so -asb_detection -asb-log-level 0 -asb_detection_instr_only < "$BC1" > "$BC2"

    if [ $? = 0 ]; then
        echo "clang -g -O0 -c -o \"$DEST\" \"$BC2\""
        clang -g -O0 -c -o "$DEST" "$BC2"
    fi
fi
