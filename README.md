# pico-mdx - MDX file player for Raspberry Pi Pico

## Introduction

- MDX file player for Raspberry Pi Pico
  - "MDX" is the music file format for Sharp X68000.
  - See: https://en.wikipedia.org/wiki/X68000%27s_MDX
- Pico Audio Pack (https://shop.pimoroni.com/products/pico-audio-pack) is needed for audio output.
- mdx2wav (https://github.com/mitsuman/mdx2wav) is used for MDX to straight PCM conversion.

## How to make

```
$ git clone --recursive https://github.com/yunkya2/pico-mdx.git
$ cd pico-mdx
$ make
```

The UF2 binary file is created in `pico-mdx/build/pico-mdx/pico-mdx.uf2`. 

The MDX file to play is described in `pico-mdx/pico-mdx/CMakeLists.txt`. Set the full path of MDX file to `MDX_FILE_NAME`.

```
target_compile_definitions(pico-mdx PRIVATE
                    :
       #
       MDX_FILE_NAME="${CMAKE_CURRENT_LIST_DIR}/test.mdx"
)
```

## Limitation

- The player can play only one MDX file embedded in the binary.
- PCM sampling rate is 22.05kHz.
- PDX (ADPCM sampling data) playback is not supported.


## License

- mdx2wav
  - Apache license
- All other files
  - BSD 3-Clause (same as Pico SDK)
