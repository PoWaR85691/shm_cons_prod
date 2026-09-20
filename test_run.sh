#!/bin/bash

export WORKERS_COUNT=16

# none
# zstd,<level>
# zfp,<accuracy>
export COMPRESSION_ALGORITHM=zfp,0.1

INPUT_FILE=bin/source_data.bin
OUTPUT_FILE=/tmp/out.bin
BUILD_DIR=build

[ -n $WORKERS_COUNT ]         && echo "WORKERS_COUNT=$WORKERS_COUNT"
[ -n $COMPRESSION_ALGORITHM ] && echo "COMPRESSION_ALGORITHM=$COMPRESSION_ALGORITHM"

$BUILD_DIR/producer $INPUT_FILE &
$BUILD_DIR/consumer $OUTPUT_FILE

./loss_percent.py $INPUT_FILE $OUTPUT_FILE
