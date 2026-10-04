# Vendored dependencies

These files are vendored so the default build requires only a C11 compiler,
`make`, `ar`, and the system math library.

## stb_image

- Project: <https://github.com/nothings/stb>
- File: `stb/stb_image.h`
- Revision: `2c980bb59875b0d32144a71867fbdebb2f77cd20`
- Version reported by the header: 2.30
- License: public domain or MIT, at the user's choice (license text is included
  at the end of `stb_image.h`)

The program compiles it with `STBI_ONLY_PNG`.

## TinyEXR v3

- Project: <https://github.com/syoyo/tinyexr>
- Directories: `include/` and `src/`
- Revision: `a41d995ee8b019514c9cbb62176a3824db07b27b`
- License: BSD-3-Clause; see `tinyexr/LICENSE` and `tinyexr/NOTICE`

The build uses the pure-C11 v3 core with its in-tree DEFLATE implementation.
CUDA, Vulkan, freestanding, and ZSTD translation units are intentionally omitted.
