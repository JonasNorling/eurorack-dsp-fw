#!/usr/bin/env python
#
# Create C code for lookup tables. Output file as first argument.
#
import sys
import math

def sin_lut(length=1024) -> list:
    # Create a sine lookup table of the specified length.
    # We go full floating point from 0..2pi here because we're not tight on flash
    # space and we're going to use this in a float context anyway.
    return [math.sin(2 * math.pi * i/length) for i in range(length)]

def format_lut(data: list, name) -> str:
    s = f"const float {name}[{len(data)}] = {{\n"
    for i, v in enumerate(data):
        if i % 8 == 0:
            s += "   "
        s += f" {v:.20},"
        if i % 8 == 7:
            s += "\n"
    s += "\n};"
    return s

def main():
    outfile = sys.argv[1]
    with open(outfile, "w") as f:
        print('#include "dsp/lut_data.h"', file=f)
        print(file=f)
        print(format_lut(sin_lut(), "lut_sin_table"), file=f)

if __name__ == "__main__":
    main()
