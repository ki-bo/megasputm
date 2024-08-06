/* MEGASPUTM - Graphic Adventure Engine for the MEGA65
 *
 * Copyright (C) 2023-2024 Robert Steffens
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "sound.h"
#include "dma.h"
#include "index.h"
#include "io.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "util.h"
#include <mega65.h>
#include <stdint.h>

#define DMA_TIMER(f) \
    ((uint16_t)((double)(3579545.0 / (f) * 16777215.0 / 40500000.0) + 0.5))

#define DMA_TIMER_RT(f) \
    ((uint16_t)((uint32_t)(3579545ULL * 16777215ULL / 40500000ULL) / f));

#define DMA_VOL(v) \
    ((uint8_t)((double)(v) / 63.0 * 96.0 + 0.5))

#define DMA_VOL_RT(v) \
    ((uint16_t)((((uint32_t)(v * 256) * (uint32_t)(96.0 / 63.0 * 256.0)) + 32768) / 65536));
 
#define BYTE_SWAP16(x) \
    ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))

#define NUM_SOUND_SLOTS 4

#pragma clang section data="data_sound" rodata="cdata_sound" bss="bss_sound"

enum
{
  SOUND_TYPE_NONE,
  SOUND_TYPE_SAMPLE,
  SOUND_TYPE_DUAL_SAMPLE_TIMED_LOOP,
  SOUND_TYPE_MUSIC
};

struct sound_header {
  uint16_t chunk_size;
  uint8_t  unused1[4];
  uint16_t sample_size;
  uint16_t code_size;
};

struct params_sample {
  uint16_t timer;
  uint8_t  vol;
  uint8_t  loop;
};

struct priv_sample {
  uint8_t ch;
};

struct params_dual_sample_timed_loop {
  uint16_t timer1;
  uint16_t timer2;
  uint8_t  vol1;
  uint8_t  vol2;
  uint16_t frames;
};

struct priv_dual_sample_timed_loop {
  uint8_t ch[2];
  uint8_t num_frames;
};

struct params_music {
  uint16_t instoff;
  uint16_t voloff;
  uint16_t chan1off;
  uint16_t chan2off;
  uint16_t chan3off;
  uint16_t chan4off;
  uint16_t sampoff;
  uint8_t  loop;
};

struct music_channel_state {
  int8_t   __far *dataptr_i;
  int8_t   __far *dataptr;
  uint16_t __far *volbase;
  uint8_t         volptr;
  uint16_t        dur;
  uint16_t        ticks;
};

struct priv_music {
  struct params_music *params;
  struct music_channel_state ch[4];
  int8_t __far *data;
};

struct sound_params {
  uint8_t type;
  union {
    struct params_sample sample;
    struct params_dual_sample_timed_loop dual_sample_timed_loop;
    struct params_music music;
  };
};

struct sound_slot {
  uint8_t id;
  uint8_t type;
  uint8_t finished;
  
  void (*update)(struct sound_slot *slot);
  void (*stop)(struct sound_slot *slot);

  union {
    struct priv_sample sample;
    struct priv_dual_sample_timed_loop dual_sample_timed_loop;
    struct priv_music music;
  };
} sound_slots[NUM_SOUND_SLOTS];

uint8_t channel_use[4] = {0xff, 0xff, 0xff, 0xff};
struct sound_params sounds[70] = {
  [54] = {.type = SOUND_TYPE_DUAL_SAMPLE_TIMED_LOOP, .dual_sample_timed_loop = {.timer1 = DMA_TIMER(0x7a), .timer2 = DMA_TIMER(0x7b), .vol1 = DMA_VOL(0x3f), .vol2 = DMA_VOL(0x3f), .frames = 0x000a}},
  [56] = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x01c2), .vol = DMA_VOL(0x1e), .loop = 1}},
  [57] = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x1fc), .vol = DMA_VOL(0x3f), .loop = 0}},
  [58] = {.type = SOUND_TYPE_MUSIC, .music = {.instoff = 0x0032, .voloff = 0x0132, .chan1off = 0x0932, .chan2off = 0x1802, .chan3off = 0x23d2, .chan4off = 0x3ea2, .sampoff = 0x4f04, .loop = 0}}
};
  
// private functions
static uint8_t get_free_sound_slot(void);
static uint8_t get_free_channel(void);
static void stop_slot(struct sound_slot *slot);
static uint16_t read_be16(void __far *ptr);
static uint8_t read_be16_lsb(void __far *ptr);
static uint16_t freq_to_timer(uint16_t freq);
static uint8_t alloc_and_start_channel(uint8_t slot_id, int8_t __far *data, uint16_t size, uint8_t flags, uint16_t timer, uint8_t vol);
static void start_channel(uint8_t ch, int8_t __far *data, uint16_t size, uint8_t flags, uint16_t timer, uint8_t vol);
static void start_channel_looped(uint8_t ch, int8_t __far *data, uint16_t loop_offset, uint16_t size, uint16_t timer, uint8_t vol);
static void start_sample(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params);
static void update_sample(struct sound_slot *slot);
static void stop_sample(struct sound_slot *slot);
static void start_dual_sample_timed_loop(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params);
static void update_dual_sample_timed_loop(struct sound_slot *slot);
static void stop_dual_sample_timed_loop(struct sound_slot *slot);
static void start_music(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params);
static void update_music(struct sound_slot *slot);
static void stop_music(struct sound_slot *slot);
static void print_slots(void);

/**
  * @defgroup sound_init Sound Init Functions
  * @{
  */
#pragma clang section text="code_init" data="data_init" rodata="cdata_init" bss="bss_init"
void sound_init(void)
{
  for (uint8_t ch = 0; ch < 4; ++ch) {
    DMA.aud_ch[ch].ctrl = 0;
    DMA.aud_ch_pan_vol[ch] = 0;
  }

  DMA.aud_ctrl |= 0x80; // Enable audio
}

/// @} // sound_init

/**
  * @defgroup sound_public Sound Public Functions
  * @{
  */
#pragma clang section text="code_sound" data="data_sound" rodata="cdata_sound" bss="bss_sound"

void sound_play(uint8_t sound_id)
{
  //debug_out("sound play %d", sound_id);
  if (sounds[sound_id].type == SOUND_TYPE_NONE) {
    fatal_error(ERR_UNIMPLEMENTED_SOUND);
  }

  uint8_t slot = get_free_sound_slot();
  if (slot == 0xff) {
    return;
  }

  sound_slots[slot].id = sound_id;

  struct sound_header __far *data;
  if (res_is_music(sound_id)) {
    data = (struct sound_header __far *)MUSIC_DATA;
  }
  else {
    uint8_t res_page = res_provide(RES_TYPE_SOUND, sound_id, 0);
    res_activate_slot(res_page);
    data = (struct sound_header __far *)res_get_huge_ptr(res_page);
  }

  uint16_t size          = BYTE_SWAP16(data->sample_size);
  uint16_t sample_offset = BYTE_SWAP16(data->code_size) + sizeof(struct sound_header);
  __auto_type sample_ptr = (int8_t __far *)(data) + sample_offset;
  __auto_type params     = &sounds[sound_id];

  switch (params->type) {
    case SOUND_TYPE_SAMPLE:
      start_sample(slot, sample_ptr, size, params);
      break;
    case SOUND_TYPE_DUAL_SAMPLE_TIMED_LOOP:
      start_dual_sample_timed_loop(slot, sample_ptr, size, params);
      break;
    case SOUND_TYPE_MUSIC:
      start_music(slot, (int8_t __far *)data, size, params);
      break;
  }
}

void sound_stop(uint8_t sound_id)
{
  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    if (sound_slots[i].id == sound_id) {
      sound_slots[i].stop(&sound_slots[i]);
    }
  }
}

void sound_stop_all(void)
{
  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    if (sound_slots[i].type != SOUND_TYPE_NONE && sound_slots[i].id != 0) {
      debug_out("stop slot %d id %d", i, sound_slots[i].id);
      sound_slots[i].stop(&sound_slots[i]);
    }
  }
}

uint8_t sound_is_playing(uint8_t sound_id)
{
  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    if (sound_slots[i].id == sound_id) {
      return 1;
    }
  }
  return 0;
}

void sound_stop_finished_slots(void)
{
  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    if (sound_slots[i].finished) {
      debug_out("stop finished slot %d id %d", i, sound_slots[i].id);
      sound_slots[i].stop(&sound_slots[i]);
    }
  }
}

void sound_process(void)
{
  __auto_type slot = &sound_slots[0];
  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i, ++slot) {
    if (slot->type != SOUND_TYPE_NONE && slot->update) {
      slot->update(slot);
    }
  }
}

/// @} // sound_public

/**
  * @defgroup sound_private Sound Private Functions
  * @{
  */

static uint8_t get_free_sound_slot(void)
{
  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    if (sound_slots[i].id == 0) {
      return i;
    }
  }
  return 0xff;
}

static uint8_t get_free_channel(void)
{
  for (uint8_t ch = 0; ch < 4; ++ch) {
    if (channel_use[ch] == 0xff) {
      return ch;
    }
  }
  return 0xff;
}

static void stop_slot(struct sound_slot *slot)
{
  print_slots();
  slot->type     = SOUND_TYPE_NONE; // will prevent irq from calling update()
  res_deactivate(RES_TYPE_SOUND, slot->id, 0);
  slot->id       = 0;
  slot->finished = 0;
}

static uint16_t read_be16(void __far *ptr)
{
  uint8_t __far *p8 = ptr;
  uint8_t msb = p8[0];
  uint8_t lsb = p8[1];
  return (msb << 8) | lsb;
}

static uint8_t read_be16_lsb(void __far *ptr)
{
  uint8_t __far *p8 = ptr;
  return p8[1];
}

static uint16_t freq_to_timer(uint16_t freq)
{
  static const uint32_t base = (uint32_t)(3579545ULL * 16777215ULL / 40500000ULL);
  *(volatile uint32_t *)0xd770 = base;
  *(volatile uint32_t *)0xd774 = freq;
  VICIV.bordercol = 1;
  while (PEEK(0xd70f) & 0x80) continue;
  VICIV.bordercol = 0;
  return *(volatile uint16_t *)0xd76c;
}

static uint8_t alloc_and_start_channel(uint8_t slot_id, int8_t __far *data, uint16_t size, uint8_t flags, uint16_t timer, uint8_t vol)
{
  uint8_t ch = get_free_channel();
  if (ch != 0xff) {
    channel_use[ch] = slot_id;
    start_channel(ch, data, size, flags, timer, vol);
    //debug_out("start channel %d base %lx, size %d, timer %d, vol %d, flags %02x taddr %x\n", ch, (uint32_t)data, size, timer, vol, flags, DMA.aud_ch[ch].top_addr);
  }
  return ch;
}

static void start_channel(uint8_t ch, int8_t __far *data, uint16_t size, uint8_t flags, uint16_t timer, uint8_t vol)
{
  DMA.aud_ch[ch].ctrl                = 0;
  DMA.aud_ch[ch].freq.lsb16          = timer;
  DMA.aud_ch[ch].freq.msb            = 0;
  DMA.aud_ch[ch].base_addr.addr16    = LSB16(data);
  DMA.aud_ch[ch].current_addr.addr16 = LSB16(data);
  DMA.aud_ch[ch].base_addr.bank      = BANK(data);
  DMA.aud_ch[ch].current_addr.bank   = BANK(data);
  DMA.aud_ch[ch].top_addr            = LSB16(data) + size - 1;
  DMA.aud_ch[ch].volume              = vol;
  DMA.aud_ch[ch].ctrl                = ADMA_CHEN_MASK | ADMA_SBITS_8 | flags;

  //debug_out("start channel2 %d base %lx, size %d, timer %d, vol %d, flags %02x taddr %x\n", ch, (uint32_t)data, size, timer, vol, flags, DMA.aud_ch[ch].top_addr);
}

static void start_channel_looped(uint8_t ch, int8_t __far *data, uint16_t loop_offset, uint16_t size, uint16_t timer, uint8_t vol)
{
  __auto_type loop_address = data + loop_offset;

  DMA.aud_ch[ch].ctrl                = 0;
  DMA.aud_ch[ch].freq.lsb16          = timer;
  DMA.aud_ch[ch].freq.msb            = 0;
  DMA.aud_ch[ch].base_addr.addr16    = LSB16(loop_address);
  DMA.aud_ch[ch].current_addr.addr16 = LSB16(data);
  DMA.aud_ch[ch].base_addr.bank      = BANK(loop_address);
  DMA.aud_ch[ch].current_addr.bank   = BANK(data);
  DMA.aud_ch[ch].top_addr            = LSB16(data) + size - 1;
  DMA.aud_ch[ch].volume              = vol;
  DMA.aud_ch[ch].ctrl                = ADMA_CHEN_MASK | ADMA_SBITS_8 | ADMA_CHLOOP_MASK;

  //debug_out("start channel looped %d base %lx, loop_offset %d, size %d, timer %d, vol %d, taddr %x\n", ch, (uint32_t)data, loop_offset, size, timer, vol, DMA.aud_ch[ch].top_addr);
}

static void start_sample(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params)
{
  __auto_type slot = &sound_slots[slot_id];
  __auto_type priv = &slot->sample;

  slot->update  = update_sample;
  slot->stop    = stop_sample;
  uint8_t flags = params->sample.loop ? ADMA_CHLOOP_MASK : 0;

  //debug_out("start sample %d, %d, %d\n", slot_id, size, params->sample.timer);
  priv->ch = alloc_and_start_channel(slot_id, data, size, flags, params->sample.timer, params->sample.vol);
  //debug_out("playing ch %d", priv->ch);
}

static void update_sample(struct sound_slot *slot)
{
  uint8_t ch = slot->sample.ch;
  if (DMA.aud_ch[ch].ctrl & ADMA_CHSTP_MASK) {
    slot->finished = 1;
  }
}

static void stop_sample(struct sound_slot *slot)
{
  stop_slot(slot);

  uint8_t ch = slot->sample.ch;
  if (ch != 0xff) {
    DMA.aud_ch[ch].ctrl &= 0;
    channel_use[ch] = 0xff;
  }
}

static void start_dual_sample_timed_loop(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params)
{
  __auto_type slot = &sound_slots[slot_id];
  __auto_type priv = &slot->dual_sample_timed_loop;

  slot->update     = update_dual_sample_timed_loop;
  slot->stop       = stop_dual_sample_timed_loop;

  priv->num_frames = params->dual_sample_timed_loop.frames;
  priv->ch[0]      = alloc_and_start_channel(slot_id, data, size, ADMA_CHLOOP_MASK, params->dual_sample_timed_loop.timer1, params->dual_sample_timed_loop.vol1);
  priv->ch[1]      = alloc_and_start_channel(slot_id, data, size, ADMA_CHLOOP_MASK, params->dual_sample_timed_loop.timer2, params->dual_sample_timed_loop.vol2);

  // setting the type will enable updates from within the irq, so we only set it once everything else is done
  slot->type = SOUND_TYPE_DUAL_SAMPLE_TIMED_LOOP;
}

static void update_dual_sample_timed_loop(struct sound_slot *slot)
{
  __auto_type priv = &slot->dual_sample_timed_loop;

  if (priv->num_frames > 0) {
    --priv->num_frames;
    if (priv->num_frames == 0) {
      for (uint8_t i = 0; i < 2; ++i) {
        uint8_t ch = priv->ch[i];
        if (ch != 0xff) {
          DMA.aud_ch[ch].ctrl &= ~ADMA_CHLOOP_MASK; // stop loop
        }
      }
      slot->finished = 1;
    }
  }
}

static void stop_dual_sample_timed_loop(struct sound_slot *slot)
{
  stop_slot(slot);

  for (uint8_t i = 0; i < 2; ++i) {
    uint8_t ch = slot->dual_sample_timed_loop.ch[i];
    if (ch != 0xff) {
      DMA.aud_ch[ch].ctrl = 0;
      channel_use[ch]     = 0xff;
    }
  }
}

static void start_music(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params)
{
  __auto_type slot = &sound_slots[slot_id];
  __auto_type priv = &slot->music;

  // music playback is exclusive, so stop all other sounds
  sound_stop_all();

  slot->update     = update_music;
  slot->stop       = stop_music;

  __auto_type p         = &params->music;
  priv->params          = p;
  priv->data            = data;
  priv->ch[0].dataptr_i = data + p->chan1off;
  priv->ch[1].dataptr_i = data + p->chan2off;
  priv->ch[2].dataptr_i = data + p->chan3off;
  priv->ch[3].dataptr_i = data + p->chan4off;
  for (uint8_t i = 0; i < 4; ++i) {
    __auto_type chan = &priv->ch[i];
    chan->dataptr = chan->dataptr_i;
    chan->volbase = 0;
    chan->volptr  = 0;
    //priv->chan    = 0;
    chan->dur     = 0;
    chan->ticks   = 0;
  }

  // setting the type will enable updates from within the irq, so we only set it once everything else is done
  slot->type = SOUND_TYPE_MUSIC;
}

static void update_music(struct sound_slot *slot)
{
  __auto_type priv = &slot->music;
  uint8_t channels_finished = 0;

  for (uint8_t i = 0; i < 4; ++i) {
    __auto_type chan = &priv->ch[i];

    if (chan->dur) {
      if (!--chan->dur) {
        DMA.aud_ch[i].ctrl = 0;
      }
      else {
        uint8_t  vol         = read_be16_lsb(chan->volbase + chan->volptr);
        DMA.aud_ch[i].volume = DMA_VOL_RT(vol);

        if (++chan->volptr == 0) {
          DMA.aud_ch[i].ctrl = 0;
          chan->dur          = 0;
        }
      }
    }

    if (!chan->dataptr) {
      ++channels_finished;
      continue;
    }

    if (read_be16(chan->dataptr) <= chan->ticks) {
      uint16_t freq = read_be16(chan->dataptr + 2);
      if (freq == 0xffff) {
        if (priv->params->loop) {
          chan->dataptr = chan->dataptr_i;
          chan->ticks   = 0;
          if (read_be16(chan->dataptr) > 0) {
            ++chan->ticks;
            continue;
          }
          freq = read_be16(chan->dataptr + 2);
        }
        else {
          chan->dataptr = NULL;
          ++channels_finished;
          continue;
        }
      }

      uint16_t timer      = freq_to_timer(freq);
      uint16_t inst       = read_be16(chan->dataptr + 8);
      __auto_type instptr = priv->data + priv->params->instoff + (inst << 5);
      chan->volbase       = (uint16_t __far *)(priv->data + priv->params->voloff + (read_be16(instptr) << 9));
      chan->volptr        = 0;
      uint8_t ch          = U8(read_be16(chan->dataptr + 6)) & U8(0x03);
      if (ch != i) {
        fatal_error(ERR_MUSIC_CHANNEL_MISMATCH);
      }
      if (chan->dur) {
        DMA.aud_ch[i].ctrl = 0;
      }
      chan->dur = read_be16(chan->dataptr + 4);
      
      uint8_t vol = read_be16_lsb(chan->volbase + chan->volptr);
      ++chan->volptr;
      vol = DMA_VOL_RT(vol);

      uint16_t offset      = read_be16(instptr + 0x14);
      uint16_t len         = read_be16(instptr + 0x18);
      uint16_t loop_offset = read_be16(instptr + 0x16);
      uint16_t loop_len    = read_be16(instptr + 0x10);

      if (loop_len > 100) {
        debug_out("sample offset %d len %d loop_offset %d loop_len %d vol %d", offset, len, loop_offset, loop_len, vol);
      }

      if (loop_offset == 0 || loop_len < 10) {
        loop_len = 0;
        loop_offset = offset + len;
        //debug_out("loop_len < 10");
      }

      uint16_t size       = len;// + loop_len;

      // if (offset + len != loop_offset) {
      //   fatal_error(ERR_NON_CONTIGUOUS_LOOP_SAMPLE);
      // }

      __auto_type sample_data = priv->data + priv->params->sampoff + offset;

      // if (loop_len) {
      //   fatal_error(1);
      //   //start_channel_looped(i, sample_data, loop_offset, size, timer, vol);
      // }
      // else {
        //fatal_error(2);
        start_channel(i, sample_data, size, 0, timer, vol);
      // }

      chan->dataptr += 16;
    }
    ++chan->ticks;
  }

  if (channels_finished == 4) {
    slot->finished = 1;
  }
}

static void stop_music(struct sound_slot *slot)
{
  stop_slot(slot);

  for (uint8_t i = 0; i < 4; ++i) {
    DMA.aud_ch[i].ctrl = 0;
    channel_use[i]     = 0xff;
  }
}

static void print_slots(void)
{
  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    debug_out("slot %d: id %d, type %d, finished %d\n", i, sound_slots[i].id, sound_slots[i].type, sound_slots[i].finished);
  }
}


/// @} // sound_private