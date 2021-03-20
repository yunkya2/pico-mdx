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

The UF2 binary file will be created in `pico-mdx/build/pico-mdx/pico-mdx.uf2`. 

The file `pico-mdx/pico-mdx/mdxlist.txt` contains the list of MDX files to play.

```
#   mdxlist.txt
#

#   PDX file search path
%.

#   MDX file names
test.mdx
```

Describe the file name of the MDX file, one on each line.
The PDX file corresponding to the MDX file is searched from the search path which is described the line beginning "%".

## User Interface

Push the BOOTSEL button once to start playback.
The following actions are accepted during playback.

| Action |               | Operation      |
|--------|---------------|----------------|
| *     | (single click) | Next track     |
| * *   | (double click) | Previous track |
| * * * | (triple click) | Stop playback  |
| ==    | (hold down)    | Volume down    |
| * ==  | (single click and hold down) | Volume up |

## Limitation

- PCM sampling rate is 22.05kHz.

## License

- mdx2wav
  - Apache license
- All other files
  - BSD 3-Clause (same as Pico SDK)
