#!/usr/bin/env python3
"""Mean relative error between the source and the restored float files.

The metric is the one from the assignment:

    e[i]         = abs(out[i] - in[i]) / abs(in[i])
    loss_percent = 100 * mean(e[i])
"""

import argparse
import array
import sys

VALUE_SIZE = 4
SEPARATOR_SIZE = 1
RECORD_SIZE = VALUE_SIZE + SEPARATOR_SIZE
SEPARATOR = 0x0A


def load_values(path):
    with open(path, "rb") as stream:
        raw = bytearray(stream.read())

    if len(raw) % RECORD_SIZE != 0:
        raise SystemExit(
            f"{path}: size {len(raw)} is not a multiple of {RECORD_SIZE} bytes per record"
        )

    count = len(raw) // RECORD_SIZE
    if raw[VALUE_SIZE::RECORD_SIZE].count(SEPARATOR) != count:
        raise SystemExit(
            f"{path}: not a sequence of float32 records separated by newline"
        )

    del raw[VALUE_SIZE::RECORD_SIZE]
    values = array.array("f")
    if values.itemsize != VALUE_SIZE:
        raise SystemExit("this platform does not use 32-bit floats")
    values.frombytes(raw)
    if sys.byteorder != "little":
        values.byteswap()
    return values


def loss_percent(source, restored):
    total = 0.0
    count = 0

    for expected, actual in zip(source, restored):
        if expected == 0.0:
            continue

        error = abs(actual - expected) / abs(expected)
        total += error
        count += 1

    if count != 0:
        mean_error = total / count
    else:
        mean_error = 0.0

    return mean_error


def main():
    parser = argparse.ArgumentParser(
        description="Loss percent between the source and the restored float files"
    )
    parser.add_argument("source", help="input file read by the producer")
    parser.add_argument("restored", help="output file written by the consumer")
    arguments = parser.parse_args()

    source = load_values(arguments.source)
    restored = load_values(arguments.restored)

    if len(source) != len(restored):
        print(
            f"warning: {len(source)} values in {arguments.source}, "
            f"{len(restored)} values in {arguments.restored}",
            file=sys.stderr,
        )

    mean_error = loss_percent(source, restored)

    print(f"loss_percent={100 * mean_error:.6f}")


if __name__ == "__main__":
    main()
