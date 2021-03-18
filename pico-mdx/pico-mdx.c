/*
 * pico-mdx
 */
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>

#include "gamdx/mxdrvg/mxdrvg.h"

#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#include "pico/stdlib.h"
#include "pico/audio_i2s.h"

#define SAMPLE_FREQ         22050
#define SAMPLES_PER_BUFFER  256

//#define DEBUG
//#define WAIT

void minfo(void)
{
#ifdef DEBUG
  struct mallinfo m = mallinfo();
  printf("free=%d\n", PICO_HEAP_SIZE - m.uordblks);
#endif
}

struct audio_buffer_pool *init_audio() {

    static audio_format_t audio_format = {
            .format = AUDIO_BUFFER_FORMAT_PCM_S16,
            .sample_freq = SAMPLE_FREQ,
            .channel_count = 2,
    };

    static struct audio_buffer_format producer_format = {
            .format = &audio_format,
            .sample_stride = 4
    };

    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 3,
                                                                      SAMPLES_PER_BUFFER); // todo correct size
    bool __unused ok;
    const struct audio_format *output_format;

    struct audio_i2s_config config = {
            .data_pin = PICO_AUDIO_I2S_DATA_PIN,
            .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
            .dma_channel = 0,
            .pio_sm = 0,
    };

    output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format) {
        panic("PicoAudio: Unable to open audio device.\n");
    }

    ok = audio_i2s_connect(producer_pool);
    assert(ok);
    audio_i2s_set_enabled(true);
    return producer_pool;
}

__asm__ (
  ".section .rodata\n"

  ".balign 4\n"
  ".global g_mdx_data\n"
  "g_mdx_data:\n"
  ".incbin \"" MDX_FILE_NAME "\"\n"

  ".balign 4\n"
  ".global sz_mdx_data\n"
  "sz_mdx_data:\n"
  ".word . - g_mdx_data\n"

  ".section .text\n"
);

#ifdef PDX_FILE_NAME
__asm__ (
  ".section .rodata\n"

  ".balign 4\n"
  ".global g_pdx_data\n"
  "g_pdx_data:\n"
  ".byte 0x00\n"
  ".byte 0x00\n"
  ".byte 0x00\n"
  ".byte 0x00\n"
  ".byte 0x00\n"
  ".byte 0x0a\n"
  ".byte 0x00\n"
  ".byte 0x02\n"
  ".byte 0x00\n"
  ".byte 0x00\n"
  ".incbin \"" PDX_FILE_NAME "\"\n"

  ".balign 4\n"
  ".global sz_pdx_data\n"
  "sz_pdx_data:\n"
  ".word . - g_pdx_data\n"

  ".section .text\n"
);
#endif

extern unsigned char g_mdx_data, g_pdx_data;
extern int sz_mdx_data, sz_pdx_data;

const int MAGIC_OFFSET = 10;

bool LoadMDX(void)
{
  unsigned char *mdx_buf = &g_mdx_data;
  int mdx_size = (int)sz_mdx_data;
#ifdef PDX_FILE_NAME
  unsigned char *pdx_buf = &g_pdx_data;
  int pdx_size = (int)sz_pdx_data;
#else
  unsigned char *pdx_buf = 0;
  int pdx_size = 0;
#endif

  int pos = 0;

  printf("load MDX 0x%p size=%d\n", mdx_buf, mdx_size);

  while (pos < mdx_size) {
    unsigned char c = mdx_buf[pos++];
    if (c == 0) break;
  }

  // Convert mdx to MXDRVG readable structure.

  unsigned char *mdx_fixed_buf;
  int mdx_fixed_size = mdx_size - pos + MAGIC_OFFSET;

  mdx_fixed_buf = malloc(mdx_fixed_size);
  memcpy(&mdx_fixed_buf[MAGIC_OFFSET], &mdx_buf[pos], mdx_size - pos);

  mdx_fixed_buf[0] = 0x00;
  mdx_fixed_buf[1] = 0x00;
  mdx_fixed_buf[2] = (pdx_buf ? 0 : 0xff);
  mdx_fixed_buf[3] = (pdx_buf ? 0 : 0xff);
  mdx_fixed_buf[4] = 0;
  mdx_fixed_buf[5] = 0x0a;
  mdx_fixed_buf[6] = 0x00;
  mdx_fixed_buf[7] = 0x08;
  mdx_fixed_buf[8] = 0x00;
  mdx_fixed_buf[9] = 0x00;

  MXDRVG_SetData(mdx_fixed_buf, mdx_fixed_size, pdx_buf, pdx_size + MAGIC_OFFSET);

  return true;
}

int main(int argc, char **argv) 
{
  int MDX_BUF_SIZE = 16;   // The buffer is no longer used.
  int PDX_BUF_SIZE = 16;   // The buffer is no longer used.
  int SAMPLE_RATE = SAMPLE_FREQ;
  int filter_mode = 0;

  int loop = 2;
  int fadeout = 0;

  int AUDIO_BUF_SAMPLES = SAMPLE_RATE / 100; // 10ms

  stdio_init_all();

#ifdef WAIT
    while (1) {
      int c = getchar_timeout_us(0);
      if (c >= 0) {
        break;
      }
    }
#endif

  minfo();

  struct audio_buffer_pool *ap = init_audio();

//    MXDRVG_SetEmulationType(MXDRVG_YM2151TYPE_FMGEN);
    MXDRVG_SetEmulationType(MXDRVG_YM2151TYPE_MAME);

  minfo();

  MXDRVG_Start(SAMPLE_RATE, filter_mode, MDX_BUF_SIZE, PDX_BUF_SIZE);

  minfo();

  MXDRVG_TotalVolume(MDX_VOLUME);

  LoadMDX();

  float song_duration = MXDRVG_MeasurePlayTime(loop, fadeout) / 1000.0f;
  // Warning: MXDRVG_MeasurePlayTime calls MXDRVG_End internaly,
  //          thus we need to call MXDRVG_PlayAt due to reset playing status.
  MXDRVG_PlayAt(0, loop, fadeout);

  minfo();

  printf("start\n");

  while (true) {
    if (MXDRVG_GetTerminated()) {
      break;
    }

    struct audio_buffer *buffer = take_audio_buffer(ap, true);
    int16_t *samples = (int16_t *) buffer->buffer->bytes;

    int len = MXDRVG_GetPCM(samples, buffer->max_sample_count);
    if (len <= 0) {
      break;
    }

    buffer->sample_count = len;
    give_audio_buffer(ap, buffer);
  }

  MXDRVG_End();

  return 0;
}
