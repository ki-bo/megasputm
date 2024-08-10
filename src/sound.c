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

#define MAX_VOL 63

#define DMA_TIMER(f) \
    ((uint16_t)((double)(3579545.0 / (f) * 16777215.0 / 40500000.0) + 0.5))

#define DMA_TIMER_RT(f) \
    ((uint16_t)((uint32_t)(3579545ULL * 16777215ULL / 40500000ULL) / f))

#define DMA_VOL(v) \
    ((uint8_t)((double)(v) / 63.0 * (double)MAX_VOL + 0.5))

#define DMA_VOL_RT(v) \
    ((uint16_t)((((uint32_t)(v * 256) * (uint32_t)((double)MAX_VOL / 63.0 * 256.0)) + 32768) / 65536));
 
#define BYTE_SWAP16(x) \
    ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))

#define NUM_SOUND_SLOTS 4

#pragma clang section data="data_sound" rodata="cdata_sound" bss="bss_sound"

enum
{
  SOUND_TYPE_NONE,
  SOUND_TYPE_IGNORED,
  SOUND_TYPE_SAMPLE,
  SOUND_TYPE_DUAL_SAMPLE_TIMED_LOOP,
  SOUND_TYPE_ALARM,
  SOUND_TYPE_MICROWAVE_DING,
  SOUND_TYPE_TENTACLE,
  SOUND_TYPE_EXPLOSION,
  SOUND_TYPE_MUSIC
};

enum
{
  PAN_LEFT,
  PAN_CENTER,
  PAN_RIGHT
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
  uint16_t loop_offset;
  uint16_t loop_len;
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

struct priv_alarm {
  uint16_t freq1;
  uint16_t freq2;
  int8_t   step1;
  uint8_t  int_ctr;
  uint8_t  ch1;
  uint8_t  ch2;
};

struct priv_microwave_ding {
  uint8_t fade_in_step;
  uint8_t fade_out_step;
  uint8_t ch;
};

struct params_tentacle {
  uint8_t step;
};

struct priv_tentacle {
  uint8_t  ch;
  uint16_t freq;
  uint8_t  step;
};

struct priv_explosion {
  uint8_t  ch;
  uint16_t freq;
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
    struct params_tentacle tentacle;
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
    struct priv_alarm alarm;
    struct priv_microwave_ding microwave_ding;
    struct priv_tentacle tentacle;
    struct priv_explosion explosion;
  };
} sound_slots[NUM_SOUND_SLOTS];

uint8_t channel_use[4] = {0xff, 0xff, 0xff, 0xff};
struct sound_params sounds[71] = {
  [8]  = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x01ac), .vol = DMA_VOL(0x3f), .loop = 0}},
  [9]  = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x01ac), .vol = DMA_VOL(0x3f), .loop = 0}},
  [10] = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x01fc), .vol = DMA_VOL(0x3f), .loop = 0}},
  [12] = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x017c), .vol = DMA_VOL(0x3f), .loop = 0}},
  [14] = {.type = SOUND_TYPE_MICROWAVE_DING},
  [15] = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x016e), .vol = DMA_VOL(0x3f), .loop = 1, .loop_offset = 7124, .loop_len = 0}},
  [16] = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x016e), .vol = DMA_VOL(0x3f), .loop = 1, .loop_offset = 0, .loop_len = 0}},
  [17] = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x016e), .vol = DMA_VOL(0x3f), .loop = 1, .loop_offset = 0, .loop_len = 0}},
  [19] = {.type = SOUND_TYPE_DUAL_SAMPLE_TIMED_LOOP, .dual_sample_timed_loop = {.timer1 = DMA_TIMER(0xf8), .timer2 = DMA_TIMER(0xf7), .vol1 = DMA_VOL(0x3f), .vol2 = DMA_VOL(0x3f), .frames = 0x000a}},
  [28] = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x0078), .vol = DMA_VOL(0x28), .loop = 0}},
  [32] = {.type = SOUND_TYPE_ALARM},
  [33] = {.type = SOUND_TYPE_ALARM},
  [36] = {.type = SOUND_TYPE_TENTACLE, .tentacle = {.step = 7}},
  [38] = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x01c2), .vol = DMA_VOL(0x1e), .loop = 1, .loop_offset = 0, .loop_len = 0}},
  [39] = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x017c), .vol = DMA_VOL(0x39), .loop = 0}},
  [42] = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x01f8), .vol = DMA_VOL(0x3f), .loop = 0}},
  [50] = {.type = SOUND_TYPE_MUSIC, .music = {.instoff = 0x0032, .voloff = 0x00b2, .chan1off = 0x08b2, .chan2off = 0x1222, .chan3off = 0x1a52, .chan4off = 0x23c2, .sampoff = 0x3074, .loop = 0}},
  [54] = {.type = SOUND_TYPE_DUAL_SAMPLE_TIMED_LOOP, .dual_sample_timed_loop = {.timer1 = DMA_TIMER(0x7c), .timer2 = DMA_TIMER(0x7b), .vol1 = DMA_VOL(0x3f), .vol2 = DMA_VOL(0x3f), .frames = 0x000a}},
  [56] = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x01c2), .vol = DMA_VOL(0x1e), .loop = 1, .loop_offset = 0, .loop_len = 0}},
  [57] = {.type = SOUND_TYPE_SAMPLE, .sample = {.timer = DMA_TIMER(0x01fc), .vol = DMA_VOL(0x3f), .loop = 0}},
  [58] = {.type = SOUND_TYPE_MUSIC, .music = {.instoff = 0x0032, .voloff = 0x0132, .chan1off = 0x0932, .chan2off = 0x1802, .chan3off = 0x23d2, .chan4off = 0x3ea2, .sampoff = 0x4f04, .loop = 0}},
  [69] = {.type = SOUND_TYPE_EXPLOSION}
};
  
// private functions
static uint8_t get_free_sound_slot(void);
static uint8_t get_free_channel(void);
static void stop_slot(struct sound_slot *slot);
static uint16_t read_be16(void __far *ptr);
static uint8_t read_be16_lsb(void __far *ptr);
static uint16_t freq_to_timer(uint16_t freq);
static uint8_t alloc_and_start_channel(uint8_t sound_id, int8_t __far *data, uint16_t size, uint16_t loop_offset, uint8_t flags, uint16_t timer, uint8_t vol, uint8_t pan);
static void start_channel(uint8_t ch, int8_t __far *data, uint16_t size, uint16_t loop_offset, uint8_t flags, uint16_t timer, uint8_t vol, uint8_t pan);
static void set_channel_vol_and_pan(uint8_t ch, uint8_t vol, uint8_t pan_left);
static void start_sample(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params);
static void update_sample(struct sound_slot *slot);
static void start_dual_sample_timed_loop(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params);
static void update_dual_sample_timed_loop(struct sound_slot *slot);
static void start_music(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params);
static void update_music(struct sound_slot *slot);
static void start_alarm(uint8_t slot_id, int8_t __far *data);
static void update_alarm(struct sound_slot *slot);
static void start_microwave_ding(uint8_t slot_id, int8_t __far *data, uint16_t size);
static void update_microwave_ding(struct sound_slot *slot);
static void start_tentacle(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params);
static void update_tentacle(struct sound_slot *slot);
static void start_explosion(uint8_t slot_id, int8_t __far *data, uint16_t size);
static void update_explosion(struct sound_slot *slot);
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

  sound_slots[slot].id       = sound_id;
  sound_slots[slot].finished = 0;

  struct sound_header __far *data;
  uint8_t res_page = res_provide(RES_TYPE_SOUND, sound_id, 0);
  if (res_is_music(sound_id)) {
    data = (struct sound_header __far *)MUSIC_DATA;
  }
  else {
    res_activate_slot(res_page);
    data = (struct sound_header __far *)res_get_huge_ptr(res_page);
  }

  uint16_t size          = BYTE_SWAP16(data->sample_size);
  uint16_t sample_offset = BYTE_SWAP16(data->code_size) + sizeof(struct sound_header);
  __auto_type sample_ptr = (int8_t __far *)(data) + sample_offset;
  __auto_type params     = &sounds[sound_id];

  sound_stop(sound_id);

  switch (params->type) {
    case SOUND_TYPE_IGNORED:
      sound_slots[slot].id = 0;
      res_deactivate_slot(res_page);
      break;
    case SOUND_TYPE_SAMPLE:
      start_sample(slot, sample_ptr, size, params);
      break;
    case SOUND_TYPE_DUAL_SAMPLE_TIMED_LOOP:
      start_dual_sample_timed_loop(slot, sample_ptr, size, params);
      break;
    case SOUND_TYPE_ALARM:
      start_alarm(slot, sample_ptr);
      break;
    case SOUND_TYPE_MICROWAVE_DING:
      start_microwave_ding(slot, sample_ptr, size);
      break;
    case SOUND_TYPE_TENTACLE:
      start_tentacle(slot, sample_ptr, size, params);
      break;
    case SOUND_TYPE_EXPLOSION:
      start_explosion(slot, sample_ptr, size);
      break;
    case SOUND_TYPE_MUSIC:
      start_music(slot, (int8_t __far *)data, size, params);
      break;
  }
}

void sound_stop(uint8_t sound_id)
{
  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    if (sound_slots[i].type != SOUND_TYPE_NONE && sound_slots[i].id == sound_id) {
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
    if (sound_slots[i].type != SOUND_TYPE_NONE && sound_slots[i].id == sound_id) {
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
  debug_out("stop slot spund id %d", slot->id);
  if (slot->type == SOUND_TYPE_NONE || slot->type == SOUND_TYPE_IGNORED) {
    fatal_error(ERR_STOPPING_EMPTY_SOUND_SLOT);
  }
  //print_slots();
  uint8_t is_music = slot->type == SOUND_TYPE_MUSIC;
  slot->type       = SOUND_TYPE_NONE; // will prevent irq from calling update()
  for (uint8_t i = 0; i < 4; ++i) {
    if (channel_use[i] == slot->id) {
      DMA.aud_ch[i].ctrl = 0;
      channel_use[i]     = 0xff;
    }
  }
  if (!is_music) {
    res_deactivate(RES_TYPE_SOUND, slot->id, 0);
  }
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
  while (PEEK(0xd70f) & 0x80) continue;
  return *(volatile uint16_t *)0xd76c;
}

static uint8_t alloc_and_start_channel(uint8_t sound_id, int8_t __far *data, uint16_t size, uint16_t loop_offset, uint8_t flags, uint16_t timer, uint8_t vol, uint8_t pan)
{
  uint8_t ch = get_free_channel();
  if (ch != 0xff) {
    channel_use[ch] = sound_id;
    start_channel(ch, data, size, loop_offset, flags, timer, vol, pan);
    //debug_out("start channel %d base %lx, size %d, timer %d, vol %d, flags %02x taddr %x\n", ch, (uint32_t)data, size, timer, vol, flags, DMA.aud_ch[ch].top_addr);
  }
  return ch;
}

static void start_channel(uint8_t ch, int8_t __far *data, uint16_t size, uint16_t loop_offset, uint8_t flags, uint16_t timer, uint8_t vol, uint8_t pan)
{
  __auto_type loop_address = (flags & ADMA_CHLOOP_MASK) ? data + loop_offset : data;

  //debug_out("loop sample %lx, %lx, %lx, %d", (uint32_t)data, (uint32_t)loop_address, (uint32_t)(data+size), loop_offset);

  DMA.aud_ch[ch].ctrl                = 0;
  DMA.aud_ch[ch].freq.lsb16          = timer;
  DMA.aud_ch[ch].freq.msb            = 0;
  DMA.aud_ch[ch].base_addr.addr16    = LSB16(loop_address);
  DMA.aud_ch[ch].base_addr.bank      = BANK(loop_address);
  DMA.aud_ch[ch].current_addr.addr16 = LSB16(data);
  DMA.aud_ch[ch].current_addr.bank   = BANK(data);
  DMA.aud_ch[ch].top_addr            = LSB16(data) + size - 1;

  set_channel_vol_and_pan(ch, vol, pan);

  DMA.aud_ch[ch].ctrl                = ADMA_CHEN_MASK | ADMA_SBITS_8 | flags;

  //debug_out("start channel looped %d base %lx, loop_offset %d, size %d, timer %d, vol %d, taddr %x\n", ch, (uint32_t)data, loop_offset, size, timer, vol, DMA.aud_ch[ch].top_addr);
}

static void set_channel_vol_and_pan(uint8_t ch, uint8_t vol, uint8_t pan)
{
  if (pan == PAN_CENTER) {
    DMA.aud_ch_pan_vol[ch] = vol;
    DMA.aud_ch[ch].volume  = vol;
  }
  else if ((ch < 2 && pan == PAN_LEFT) || (ch >= 2 && pan == PAN_RIGHT)) {
    DMA.aud_ch_pan_vol[ch] = 0;
    DMA.aud_ch[ch].volume  = vol;
  }
  else {
    DMA.aud_ch_pan_vol[ch] = vol;
    DMA.aud_ch[ch].volume  = 0;
  }
}

static void start_sample(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params)
{
  __auto_type slot = &sound_slots[slot_id];
  __auto_type priv = &slot->sample;

  slot->update  = update_sample;
  slot->stop    = stop_slot;

  uint8_t flags = params->sample.loop ? ADMA_CHLOOP_MASK : 0;

  //debug_out("start sample %d, %d, %d\n", slot_id, size, params->sample.timer);
  priv->ch = alloc_and_start_channel(slot->id, data, size, params->sample.loop_offset, flags, params->sample.timer, params->sample.vol, PAN_CENTER);
  //debug_out("playing ch %d", priv->ch);
  slot->type = SOUND_TYPE_SAMPLE;
}

static void update_sample(struct sound_slot *slot)
{
  uint8_t ch = slot->sample.ch;
  if (DMA.aud_ch[ch].ctrl & ADMA_CHSTP_MASK) {
    slot->finished = 1;
  }
}

static void start_dual_sample_timed_loop(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params)
{
  __auto_type slot = &sound_slots[slot_id];
  __auto_type priv = &slot->dual_sample_timed_loop;

  slot->update     = update_dual_sample_timed_loop;
  slot->stop       = stop_slot;

  priv->num_frames = params->dual_sample_timed_loop.frames;
  priv->ch[0]      = alloc_and_start_channel(slot->id, data, size, 0, ADMA_CHLOOP_MASK, params->dual_sample_timed_loop.timer1, params->dual_sample_timed_loop.vol1, PAN_LEFT);
  priv->ch[1]      = alloc_and_start_channel(slot->id, data, size, 0, ADMA_CHLOOP_MASK, params->dual_sample_timed_loop.timer2, params->dual_sample_timed_loop.vol2, PAN_RIGHT);

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

static void start_music(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params)
{
  __auto_type slot = &sound_slots[slot_id];
  __auto_type priv = &slot->music;

  // music playback is exclusive, so stop all other sounds
  sound_stop_all();

  slot->update     = update_music;
  slot->stop       = stop_slot;

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

  for (uint8_t i = 0; i < 4; ++i) {
    channel_use[i] = slot_id;
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
        //debug_out("sample offset %d len %d loop_offset %d loop_len %d vol %d", offset, len, loop_offset, loop_len, vol);
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
        start_channel(i, sample_data, size, 0, 0, timer, vol, i % 1 ? PAN_LEFT : PAN_RIGHT);
      // }

      chan->dataptr += 16;
    }
    ++chan->ticks;
  }

  if (channels_finished == 4) {
    slot->finished = 1;
  }
}

static void start_alarm(uint8_t slot_id, int8_t __far *data)
{
  __auto_type slot = &sound_slots[slot_id];
  __auto_type priv = &slot->alarm;

  slot->update = update_alarm;
  slot->stop   = stop_slot;
  
  priv->freq1 = 0x00fa;
  priv->step1 = -10;
  priv->freq2 = 0x0060;

  priv->ch1 = alloc_and_start_channel(slot->id, data,      32, 0, ADMA_CHLOOP_MASK, freq_to_timer(priv->freq1), 0x3f, PAN_LEFT);
  priv->ch2 = alloc_and_start_channel(slot->id, data + 28, 40, 0, ADMA_CHLOOP_MASK, freq_to_timer(priv->freq2), 0x3f, PAN_RIGHT);

  priv->int_ctr = 0;

  slot->type = SOUND_TYPE_ALARM;
}

static void update_alarm(struct sound_slot *slot)
{
  __auto_type priv = &slot->alarm;

  // freq1
  const uint16_t freq1_min = 0x00aa;
  const uint16_t freq1_max = 0x00fa;
  priv->freq1 += priv->step1;
  if (priv->freq1 <= freq1_min) {
    priv->freq1 = freq1_min;
    priv->step1 = -priv->step1;
  }
  else if (priv->freq1 >= freq1_max) {
    priv->freq1 = freq1_max;
    priv->step1 = -priv->step1;
  }
  if (priv->ch1 != 0xff) {
    DMA.aud_ch[priv->ch1].freq.lsb16 = freq_to_timer(priv->freq1);
  }

  // freq2 is updated every 9th frame
  if (++priv->int_ctr == 9) {
    priv->int_ctr = 0;
    if (priv->freq2 == 0xffff) {
      priv->freq2 = 0x0060;
    }
    else {
      priv->freq2 = 0xffff;
    }
    if (priv->ch2 != 0xff) {
      DMA.aud_ch[priv->ch2].freq.lsb16 = freq_to_timer(priv->freq2);
    }
  }
}

static void start_microwave_ding(uint8_t slot_id, int8_t __far *data, uint16_t size)
{
  __auto_type slot = &sound_slots[slot_id];

  __auto_type priv = &slot->microwave_ding;
  priv->fade_in_step  = 16;
  priv->fade_out_step = 2;

  slot->update = update_microwave_ding;
  slot->stop   = stop_slot;
  
  priv->ch = alloc_and_start_channel(slot->id, data, size, 0, ADMA_CHLOOP_MASK, DMA_TIMER(0x00c8), 1, PAN_CENTER);

  slot->type = SOUND_TYPE_MICROWAVE_DING;
}

static void update_microwave_ding(struct sound_slot *slot)
{
  __auto_type priv = &slot->microwave_ding;

  if (priv->ch == 0xff) {
    slot->finished = 1;
    return;
  }

  uint8_t step    = priv->fade_in_step;
  int8_t  cur_vol = DMA.aud_ch[priv->ch].volume;

  if (step) {
    cur_vol += step;
    if (cur_vol > 0x3f) {
      cur_vol = 0x3f;
      priv->fade_in_step = 0;
    }
  }
  else {
    step = priv->fade_out_step;
    cur_vol -= step;
    if (cur_vol < 1) {
      cur_vol = 0;
      slot->finished = 1;
    }
  }

  set_channel_vol_and_pan(priv->ch, cur_vol, PAN_CENTER);
}

static void start_tentacle(uint8_t slot_id, int8_t __far *data, uint16_t size, struct sound_params *params)
{
  __auto_type slot = &sound_slots[slot_id];

  __auto_type priv = &slot->tentacle;
  priv->freq = 0x007c;
  priv->step = params->tentacle.step;

  slot->update = update_tentacle;
  slot->stop   = stop_slot;
  
  priv->ch = alloc_and_start_channel(slot->id, data, size, 0, ADMA_CHLOOP_MASK, freq_to_timer(priv->freq), 0x3f, PAN_CENTER);

  slot->type = SOUND_TYPE_TENTACLE;
}

static void update_tentacle(struct sound_slot *slot)
{
  __auto_type priv = &slot->tentacle;

  uint8_t ch = priv->ch;
  if (ch == 0xff) {
    slot->finished = 1;
    return;
  }

  static const uint16_t target_freq = 0x016d;

  int8_t cur_vol = DMA.aud_ch[ch].volume;

  if (priv->freq > target_freq) {
    cur_vol = 0x3f - (priv->freq - target_freq);
    if (cur_vol < 1) {
      slot->finished = 1;
      return;
    }
    set_channel_vol_and_pan(ch, cur_vol, PAN_CENTER);  
  }
  priv->freq += priv->step;
  DMA.aud_ch[ch].freq.lsb16 = freq_to_timer(priv->freq);
}

static void start_explosion(uint8_t slot_id, int8_t __far *data, uint16_t size)
{
  __auto_type slot = &sound_slots[slot_id];

  __auto_type priv = &slot->explosion;
  priv->freq = 0x0190;

  slot->update = update_explosion;
  slot->stop   = stop_slot;
  
  priv->ch = alloc_and_start_channel(slot->id, data, size, 0, ADMA_CHLOOP_MASK, freq_to_timer(priv->freq), 0x3f >> 1, PAN_CENTER);

  slot->type = SOUND_TYPE_EXPLOSION;
}

static void update_explosion(struct sound_slot *slot)
{
  __auto_type priv = &slot->tentacle;

  uint8_t ch = priv->ch;
  if (ch == 0xff) {
    slot->finished = 1;
    return;
  }

  priv->freq += 2;
  DMA.aud_ch[ch].freq.lsb16 = freq_to_timer(priv->freq);

  uint8_t cur_vol = DMA.aud_ch[ch].volume;
  --cur_vol;
  if (cur_vol == 0) {
    slot->finished = 1;
    return;
  }
  set_channel_vol_and_pan(ch, cur_vol >> 1, PAN_CENTER);  
}

static void print_slots(void)
{
  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    debug_out("slot %d: id %d, type %d, finished %d\n", i, sound_slots[i].id, sound_slots[i].type, sound_slots[i].finished);
  }
}


/// @} // sound_private