project(pico-mdx)

set(CMAKE_C_FLAGS_RELEASE "-Wall -O3")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O3")
set(CMAKE_C_FLAGS_DEBUG "-Wall -g")
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g")

add_executable(pico-mdx
	pico-mdx.c
	mdx2wav/gamdx/downsample/downsample.cpp
	mdx2wav/gamdx/fmgen/fmgen.cpp
	mdx2wav/gamdx/fmgen/fmtimer.cpp
	mdx2wav/gamdx/fmgen/opm.cpp
	mdx2wav/gamdx/mxdrvg/so.cpp
	mdx2wav/gamdx/mxdrvg/opm_delegate.cpp
	mdx2wav/gamdx/pcm8/pcm8.cpp
	mdx2wav/gamdx/pcm8/x68pcm8.cpp
	mdx2wav/gamdx/mame/ym2151.c
)

target_include_directories(pico-mdx PUBLIC ${CMAKE_CURRENT_LIST_DIR}/mdx2wav)

target_link_libraries(pico-mdx PRIVATE
    pico_stdlib
    pico_audio_i2s
)

target_compile_definitions(pico-mdx PRIVATE
    # compile time configuration of I2S
    PICO_AUDIO_I2S_MONO_INPUT=0
    PICO_AUDIO_I2S_MONO_OUTPUT=0
    # for Pico Audio Pack
    PICO_AUDIO_I2S_DATA_PIN=9
    PICO_AUDIO_I2S_CLOCK_PIN_BASE=10
    # define for our example code
    USE_AUDIO_I2S=1
	#
	PICO_HEAP_SIZE=0x30000
	#
	MDX_VOLUME=256
	#
	MDX_FILE_NAME="${CMAKE_CURRENT_LIST_DIR}/test.mdx"
#	PDX_FILE_NAME=""
)

pico_enable_stdio_usb(pico-mdx 0)
pico_enable_stdio_uart(pico-mdx 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(pico-mdx)
