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
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
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
  ".include \"mdxdata.inc\"\n"

  ".section .text\n"
);

struct mdxdata {
  unsigned char *mdx;
  int size;
  unsigned char *pdx;
  int psize;
};

extern struct mdxdata g_mdx_data;
struct mdxdata *gd = &g_mdx_data;

int NumMDX(void)
{
  int i;

  for (i = 0; gd[i].mdx != NULL; i++)
    ;
  return i;
}

unsigned char mdxbuf[65536];

bool LoadMDX(int index)
{
  memcpy(mdxbuf, gd[index].mdx, gd[index].size);
  MXDRVG_SetData(mdxbuf, gd[index].size, gd[index].pdx, gd[index].psize);
  return true;
}

/* From pico-examples/picoboard/button/button.c */

bool __no_inline_not_in_flash_func(get_bootsel_button)() {
    const uint CS_PIN_INDEX = 1;

    // Must disable interrupts, as interrupt handlers may be in flash, and we
    // are about to temporarily disable flash access!
    uint32_t flags = save_and_disable_interrupts();

    // Set chip select to Hi-Z
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    // Note we can't call into any sleep functions in flash right now
    for (volatile int i = 0; i < 1000; ++i);

    // The HI GPIO registers in SIO can observe and control the 6 QSPI pins.
    // Note the button pulls the pin *low* when pressed.
    bool button_state = !(sio_hw->gpio_hi_in & (1u << CS_PIN_INDEX));

    // Need to restore the state of chip select, else we are going to have a
    // bad time when we return to code in flash!
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

    restore_interrupts(flags);

    return button_state;
}

#define BUTTON_THRES  (3 * (SAMPLE_FREQ / 10))

int buttoncode(int samples)
{
  static int prevbtn = 0;
  static int duration = 0;
  static int count = 0;
  static int press = 0;
  int res = 0;

  int btn = get_bootsel_button();
  if (btn != prevbtn) {
    duration = 0;
    prevbtn = btn;
    if (btn) {
      count++;
    }
  }

  duration += samples;
  if (btn) {
    if (duration > BUTTON_THRES) {
      res = -count;
      press = 1;
    }
  } else {
    if (press) {
      press = 0;
      count = 0;
    }
    if (duration > BUTTON_THRES) {
      res = count;
      count = 0;
    }
  }

  return res;
}

void waitbutton(void)
{
  while (!get_bootsel_button())
    ;
  while (get_bootsel_button())
    ;
}

int main(int argc, char **argv) 
{
  int MDX_BUF_SIZE = 16;   // The buffer is no longer used.
  int PDX_BUF_SIZE = 16;   // The buffer is no longer used.
  int SAMPLE_RATE = SAMPLE_FREQ;
  int filter_mode = 0;

  int loop = 2;
  int fadeout = 1;

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
  int vol = MDX_VOLUME;

  printf("start\n");

  waitbutton();

  int index = 0;
  int max = NumMDX();

  while (true) {
    printf("track %d\n", index);

    minfo();

//    MXDRVG_SetEmulationType(MXDRVG_YM2151TYPE_FMGEN);
    MXDRVG_SetEmulationType(MXDRVG_YM2151TYPE_MAME);
    MXDRVG_Start(SAMPLE_RATE, filter_mode, MDX_BUF_SIZE, PDX_BUF_SIZE);
    MXDRVG_TotalVolume(vol);

    LoadMDX(index);
    MXDRVG_PlayAt(0, loop, fadeout);

    int duration = 0;

    while (true) {
      if (MXDRVG_GetTerminated()) {
        break;
      }

      struct audio_buffer *buffer = take_audio_buffer(ap, true);
      int16_t *samples = (int16_t *) buffer->buffer->bytes;
      int max_sample_count = buffer->max_sample_count;
      int len = MXDRVG_GetPCM(samples, max_sample_count);
      if (len <= 0) {
        break;
      }

      buffer->sample_count = len;
      duration += len;
      give_audio_buffer(ap, buffer);

      int key = buttoncode(max_sample_count);
      if (key == 1) {
        break;
      } else if (key == 2) {
        if (duration >= SAMPLE_FREQ * 2) {
          MXDRVG_PlayAt(0, loop, fadeout);
          duration = 0;
        } else {
          index -= 2;
          if (index < -1)
            index = max - 2;
          break;
        }
      } else if (key == 3) {
        waitbutton();
      } else if (key < 0) {
          if (key == -1)
            vol = vol > 0 ? vol - 1 : 0;
          if (key == -2)
            vol = vol < 256 ? vol + 1 : 256;
          MXDRVG_TotalVolume(vol);
          printf("vol=%d\n", vol);
      }
    }
    MXDRVG_End();

    index++;
    if (index >= max)
      index = 0;
  }

  return 0;
}
