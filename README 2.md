# Image Explorer

Image Explorer is an interactive C11 command-line program for inspecting the
stored channel values in RGB/RGBA PNG and OpenEXR images. Every loaded sample is
held in memory as an IEEE-754 32-bit `float`.

## Supported input

- 8-bit RGB and RGBA PNG
- 16-bit RGB and RGBA PNG
- Single-part, non-deep OpenEXR with 32-bit `FLOAT` R/G/B channels and an
  optional 32-bit `FLOAT` A channel
- Common EXR scanline/tiled compression such as NONE, RLE, ZIP/ZIPS, PIZ,
  PXR24, B44/B44A, and HTJ2K (DWA and the nonstandard ZSTD extension are not
  enabled)

PNG integers are normalized (`value / 255` or `value / 65535`) so different PNG
bit depths share the `[0,1]` range. No gamma, transfer-function, ICC-profile, or
premultiplication conversion is applied. EXR float bits are copied unchanged.

## Build and run

```sh
make
./imgexplorer
```

The root Makefile defaults to `-O2 -g`. Other useful forms are:

```sh
make release                 # clean -O3 build without -g
make debug                   # clean -O0 -g3 build
make clean && make OPT=-O3 DEBUG=-g  # explicit custom combination
make test                    # automated format/menu/export checks
make sanitize                # AddressSanitizer + UndefinedBehaviorSanitizer
make clean
```

Both relative paths (for example `images/a.png`) and absolute paths are accepted
at every input/output prompt.

## Comparison behavior

Choose menu item 1 or 2 to load/replace images A and B, then item 3 to compare.
Images must have identical dimensions. RGB and RGBA can be compared together;
the RGB image receives an implicit alpha of `1.0` for that comparison.

The report includes:

- The percentage of float channel values whose raw 32-bit representations differ
- The percentage of pixels containing at least one difference
- Per-channel difference counts
- Mean and maximum absolute finite deltas
- Example coordinates, values, raw hexadecimal float bits, and signed `B - A`
  deltas

Raw-bit comparison intentionally distinguishes values such as `-0.0` and
`+0.0`, and can reveal different NaN payloads.

## Text export format

Menu items 5 and 6 export all float values for A or B. Menu item 4 exports the
full signed difference grid (`B - A`). Output is data-only and uses enough
decimal digits to round-trip a float32:

```text
R,G,B[,A]\tR,G,B[,A]\t...\n
R,G,B[,A]\tR,G,B[,A]\t...\n
```

Commas separate channels, tabs separate pixels, and each image row ends with a
newline. Diff export uses RGBA when either source has alpha; an absent alpha is
treated as `1.0`.

## Third-party code

The application and integration layer are C11. PNG decoding uses the C
`stb_image` implementation, and EXR decoding uses the pure-C11 TinyEXR v3 core.
Pinned revisions and licenses are recorded in
[`third_party/README.md`](third_party/README.md).
