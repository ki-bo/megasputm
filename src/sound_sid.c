#include "sound_sid.h"
#include "map.h"
#include "memory.h"
#include "resource.h"
#include "util.h"


#define ZEROMEM(a) memset(a, 0, sizeof(a))

#define READ_LE_UINT16(a) READ_UINT16(a)

#define NUM_SOUND_SLOTS 6

#define NUM_RES_SLOTS 9

// zdata variables
#pragma clang section bss="zdata"
uint8_t sound_triggers[NUM_SOUND_SLOTS];

// sound_sid data/cdata/bss variables
#pragma clang section data="data_sound_sid" rodata="cdata_sound_sid" bss="bss_sound_sid"

struct {
  uint8_t id[NUM_RES_SLOTS];
  uint8_t page[NUM_RES_SLOTS];
  uint8_t count[NUM_RES_SLOTS];
} res_data;

static const uint8_t BITMASK[7] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40
};
static const uint8_t BITMASK_INV[7] = {
  0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF
};

static const uint8_t SID_REG_OFFSET[7] = {
  0, 7, 14, 21, 2, 9, 16
};

// NTSC frequency table (also used for PAL versions).
// FREQ_TBL[i] = tone_freq[i] * 2^24 / clockFreq
static const uint16_t FREQ_TBL[97] = {
  0x0000, 0x010C, 0x011C, 0x012D, 0x013E, 0x0151, 0x0166, 0x017B,
  0x0191, 0x01A9, 0x01C3, 0x01DD, 0x01FA, 0x0218, 0x0238, 0x025A,
  0x027D, 0x02A3, 0x02CC, 0x02F6, 0x0323, 0x0353, 0x0386, 0x03BB,
  0x03F4, 0x0430, 0x0470, 0x04B4, 0x04FB, 0x0547, 0x0598, 0x05ED,
  0x0647, 0x06A7, 0x070C, 0x0777, 0x07E9, 0x0861, 0x08E1, 0x0968,
  0x09F7, 0x0A8F, 0x0B30, 0x0BDA, 0x0C8F, 0x0D4E, 0x0E18, 0x0EEF,
  0x0FD2, 0x10C3, 0x11C3, 0x12D1, 0x13EF, 0x151F, 0x1660, 0x17B5,
  0x191E, 0x1A9C, 0x1C31, 0x1DDF, 0x1FA5, 0x2187, 0x2386, 0x25A2,
  0x27DF, 0x2A3E, 0x2CC1, 0x2F6B, 0x323C, 0x3539, 0x3863, 0x3BBE,
  0x3F4B, 0x430F, 0x470C, 0x4B45, 0x4FBF, 0x547D, 0x5983, 0x5ED6,
  0x6479, 0x6A73, 0x70C7, 0x777C, 0x7E97, 0x861E, 0x8E18, 0x968B,
  0x9F7E, 0xA8FA, 0xB306, 0xBDAC, 0xC8F3, 0xD4E6, 0xE18F, 0xEEF8,
  0xFD2E
};

static const uint8_t SONG_CHANNEL_OFFSET[3] = { 6, 8, 10 };
static const uint8_t RES_ID_CHANNEL[3] = { 3, 4, 5 };

#define LOBYTE_(a) ((a) & 0xFF)
#define HIBYTE_(a) (((a) >> 8) & 0xFF)

#define GETBIT(var, pos) ((var) & (1<<(pos)))


uint8_t chanBuffer[3][45] = {
    {
      0x00,0x00,0x00,0x00,0x7f,0x01,0x19,0x00,
      0x00,0x00,0x2d,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0xf0,0x40,0x10,0x04,0x00,0x00,
      0x00,0x04,0x27,0x03,0xff,0xff,0x01,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00
    },
    {
      0x00,0x00,0x00,0x00,0x7f,0x01,0x19,0x00,
      0x00,0x00,0x2d,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0xf0,0x20,0x10,0x04,0x00,0x00,
      0x00,0x04,0x27,0x03,0xff,0xff,0x02,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00
    },
    {
      0x00,0x00,0x00,0x00,0x7f,0x01,0x19,0x00,
      0x00,0x00,0x2d,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0xf0,0x20,0x10,0x04,0x00,0x00,
      0x00,0x04,0x27,0x03,0xff,0xff,0x02,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00
    }
  };

uint8_t is_irq;

int8_t resID_song;
uint8_t res_page_music;
uint8_t act_res_page_music;

// statusBits1A/1B are always equal
uint8_t statusBits1A;
uint8_t statusBits1B;

uint8_t busyChannelBits;

uint8_t SIDReg23;
uint8_t SIDReg23Stuff;
uint8_t SIDReg24;

uint8_t *chanFileData[3];
uint16_t chanDataOffset[3];
uint8_t *songPosPtr[7];

// 0..2: freq value voice1/2/3
// 3:    filter freq
// 4..6: pulse width
uint16_t freqReg[7];

// start offset[i] for songFileOrChanBufData to obtain songPosPtr[i]
//  vec6[0..2] = 0x0008;
//  vec6[4..6] = 0x0019;
int16_t vec6[7];

// current offset[i] for songFileOrChanBufData to obtain songPosPtr[i] (starts with vec6[i], increased later)
int16_t songFileOrChanBufOffset[7];

uint16_t freqDelta[7];
int16_t freqDeltaCounter[7];
uint8_t *swapSongPosPtr[3];
uint8_t* swapVec5[3];
int16_t swapVec8[3];
uint16_t swapVec10[3];
uint16_t swapFreqReg[3];
int16_t swapVec11[3];

// never read
//uint8_t* vec5[7];
// never read
//uint8_t vec19[7];
// never read (needed by scumm engine?)
//uint8_t curChannelActive;

uint8_t *vec20[7];

uint8_t *swapVec20[3];

// resource status (never read)
// bit7: some flag
// bit6..0: counter (use-count?), maybe just bit0 as flag (used/unused?)
//uint8_t resStatus[70];

uint8_t *songFileOrChanBufData;

uint16_t stepTbl[33];

uint8_t initializing;
uint8_t _soundInQueue;
uint8_t isVoiceChannel;

uint8_t isMusicPlaying;
uint8_t swapVarLoaded;
uint8_t bgSoundActive;
uint8_t filterUsed;

uint8_t bgSoundResID;
uint8_t freeChannelCount;

// seems to be used for managing the three voices
// bit[0..2]: 0 -> unused, 1 -> already in use
uint8_t usedChannelBits;
uint8_t attackReg[3];
uint8_t sustainReg[3];

// -1/0/1
int8_t var481A;

// bit-array: 00000cba
// a/b/c: channel1/2/3
uint8_t songChannelBits;

uint8_t pulseWidthSwapped;
uint8_t swapPrepared;

// never read
//uint8_t var5163;

uint8_t filterSwapped;
uint8_t SIDReg24_HiNibble;
uint8_t keepSwapVars;

uint8_t phaseBit[3];
uint8_t releasePhase[3];

// values: a resID or -1
// resIDs: 3, 4, 5 or song-number
int8_t _soundQueue[7];

// values: a resID or 0
// resIDs: 3, 4, 5 or song-number
int8_t channelMap[7];
uint8_t chanloop[7];

uint8_t songPosUpdateCounter[7];

// priortity of channel contents
// MM:  1: lowest .. 120: highest (1,2,A,64,6E,73,78)
// Zak: -???: lowest .. 120: highest (5,32,64,65,66,6E,78, A5,A6,AF,D7)
uint8_t chanPrio[7];

// only [0..2] used?
uint8_t waveCtrlReg[7];

uint8_t swapAttack[2];
uint8_t swapSustain[2];
uint8_t swapSongPrio[3];
int8_t swapVec479C[3];
uint8_t swapVec19[3];
uint8_t swapSongPosUpdateCounter[3];
uint8_t swapWaveCtrlReg[3];

uint8_t actFilterHasLowerPrio;
uint8_t chansWithLowerPrioCount;
uint8_t minChanPrio;
uint8_t minChanPrioIndex;



void sid_write(uint8_t reg, uint8_t data);
void init_sid();
uint8_t *get_resource(int8_t resID);

void start_sound(int8_t nr);
void stop_sound(int8_t nr);
int8_t get_sound_status(int8_t nr);
void map_sound_res(uint8_t page);


void init_music(int8_t songResIndex, uint8_t res_page); // $7de6
int8_t init_sound(int8_t soundResID, uint8_t res_page); // $4D0A
void stop_sound_intern(int8_t soundResID, uint8_t flags); // $5093
void stop_music_intern(); // $4CAA

void reset_sid(); // $48D8
void handle_music_buffer();
int8_t setup_song_file_data(); // $36cb
void func_3674(uint8_t channel); // $3674
void reset_player_state(); // $48f7
void process_song_data(uint8_t channel); // $4939
void read_set_sid_filter_and_props(int8_t *offset, uint8_t *dataPtr);  // $49e7
void save_song_pos(int8_t y, uint8_t channel);
void update_freq(uint8_t channel);
void reset_freq_delta(uint8_t channel);
void read_song_chunk(uint8_t channel); // $4a6b
void set_sid_freq_as(uint8_t channel); // $4be6
void set_sid_wave_ctrl_reg(uint8_t channel); // $4C0D
int8_t setup_song_ptr(uint8_t channel); // $4C1C
void unlock_resource(int8_t chanResIndex); // $4CDA
void count_free_channels(); // $4f26
void func_4F45(uint8_t channel); // $4F45
void safe_unlock_resource(int8_t resIndex); // $4FEA
void release_resource(int8_t resIndex, uint8_t flags); // $5031
void release_res_channels(int8_t resIndex); // $5070
void release_resource_unk(int8_t resIndex); // $50A4
void release_channel(uint8_t channel);
void clear_sid_waveform(uint8_t channel);
void stop_channel(uint8_t channel);
void swap_vars(uint8_t channel, int8_t swapIndex); // $51a5
void reset_swap_vars(); // $52d0
void prepare_swap_vars(uint8_t channel); // $52E5
void use_swap_vars(uint8_t channel); // $5342
void lock_resource(int8_t resIndex); // $4ff4
void reserve_channel(uint8_t channel, uint8_t prioValue, int8_t chanResIndex); // $4ffe
//void unlockCodeLocation(); // $513e
//void lockCodeLocation(); // $514f
void func_7eae(uint8_t channel); // $7eae
void func_819b(uint8_t channel); // $819b
void build_step_tbl(uint8_t step); // $82B4
uint8_t reserve_sound_filter(uint8_t value, uint8_t chanResIndex); // $4ED0
uint8_t reserve_sound_voice(uint8_t value, uint8_t chanResIndex); // $4EB8
void find_less_prio_channels(uint8_t soundPrio); // $4ED8
void release_resource_by_sound(int8_t resID); // $5088
void read_vec6_data(int8_t x, int8_t *offset, int8_t chanResID); // $4E99



#pragma clang section text="code_init" data="data_init" rodata="cdata_init" bss="bss_init"

void c64_sound_init(void) 
{
  SAVE_CS_AUTO_RESTORE
  MAP_CS_SOUND

  /*
   * initialize data
   */

  for (int i = 0; i < 7; ++i) {
    _soundQueue[i] = -1;
  };

  //_music_timer = 0;

  //_mixer = mixer;
  //_sampleRate = _mixer->getOutputRate();
  //_vm = scumm;

  // sound speed is slightly different on NTSC and PAL machines
  // as the SID clock depends on the frame rate.
  // ScummVM does not distinguish between NTSC and PAL targets
  // so we use the NTSC timing here as the music was composed for
  // NTSC systems (music on PAL systems is slower).
  //_videoSystem = NTSC;
  //_cpuCyclesLeft = 0;

  init_sid();
  reset_sid();

  //_mixer->playStream(Audio::Mixer::kPlainSoundType, &_soundHandle, this, -1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO, true);
}

void init_sid() 
{
  /*  _sid = new Resid::SID();
    _sid->set_sampling_parameters(
      timingProps[_videoSystem].clockFreq,
      _sampleRate);
    _sid->enable_filter(true);*/
  
  
  
    //_sid->reset();
  
    //Synchronize the waveform generators (must occur after reset)
    sid_write(4, 0x08);
    sid_write(11, 0x08);
    sid_write(18, 0x08);
    sid_write(4, 0x00);
    sid_write(11, 0x00);
    sid_write(18, 0x00);
  }
  
  void reset_sid() // $48D8
  {
    SIDReg24 = 0x0f;
  
    sid_write(4, 0);
    sid_write(11, 0);
    sid_write(18, 0);
    sid_write(23, 0);
    sid_write(21, 0);
    sid_write(22, 0);
    sid_write(24, SIDReg24);
  
    reset_player_state();
  }
  

#pragma clang section text="code_main" data="data_main" rodata="cdata_main" bss="zdata"


void c64_start_sound(uint8_t sound_id) 
{
  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    if (!sound_triggers[i]) {
      sound_triggers[i] = sound_id;
      // we do a first res_provide() for each sound to avoid loading latency in sound_handle_play_triggers() later
      res_provide(RES_TYPE_SOUND_SID, sound_id, 0);
      return;
    }
  }
}

void c64_sound_handle_play_triggers(void)
{
  SAVE_CS_AUTO_RESTORE
  SAVE_DS_AUTO_RESTORE
  MAP_CS_SOUND

  for (uint8_t i = 0; i < NUM_RES_SLOTS; ++i) {
    if (res_data.count[i] == 0 && res_data.id[i] == 0xff) {
      res_deactivate_slot(res_data.page[i]);
      res_data.id[i] = 0;
    }
  }

  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    if (sound_triggers[i]) {
      start_sound((int8_t)sound_triggers[i]);
      sound_triggers[i] = 0;
    }
  }
}

void c64_stop_sound(uint8_t sound_id) 
{
  SAVE_CS_AUTO_RESTORE
  MAP_CS_SOUND

  //return;

  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    if (sound_triggers[i] == sound_id) {
      sound_triggers[i] = 0;
    }
  }
  stop_sound((int8_t)sound_id);
}

void c64_stop_all_sounds() 
{
  SAVE_CS_AUTO_RESTORE
  MAP_CS_SOUND
  
  //Common::StackLock lock(_mutex);
  reset_player_state();
}

uint8_t c64_sound_is_playing(uint8_t sound_id) 
{
  SAVE_CS_AUTO_RESTORE
  MAP_CS_SOUND

  return (uint8_t)get_sound_status((int8_t)sound_id);
}



#pragma clang section text="code_sound_sid" data="data_sound_sid" rodata="cdata_sound_sid" bss="bss_sound_sid"



void c64_sound_update() // $481B
{
  if (initializing)
    return;

  is_irq = 1;

  if (_soundInQueue) {
    for (int8_t i = 6; i >= 0; --i) {
      if (_soundQueue[i] != -1) {
        process_song_data(i);
      }
    }
    _soundInQueue = 0;
  }

  // no sound
  if (busyChannelBits == 0) {
    is_irq = 0;
    return;
  }

  for (int8_t i = 6; i >= 0; --i) {
    if (busyChannelBits & BITMASK[i]) {
      update_freq(i);
    }
  }

  // seems to be used for background (prio=1?) sounds.
  // If a bg sound cannot be played because all SID
  // voices are used by higher priority sounds, the
  // bg sound's state is updated here so it will be at
  // the correct state when a voice is available again.
  if (swapPrepared) {
    swap_vars(0, 0);
    swapVarLoaded = 1;
    update_freq(0);
    swap_vars(0, 0);
    if (pulseWidthSwapped) {
      swap_vars(4, 1);
      update_freq(4);
      swap_vars(4, 1);
    }
    swapVarLoaded = 0;
  }

  for (int8_t i = 6; i >= 0; --i) {
    if (busyChannelBits & BITMASK[i])
      set_sid_wave_ctrl_reg(i);
  };

  if (isMusicPlaying) {
    //while(1) {
      //*(volatile uint8_t *)0xd020 =  *(volatile uint8_t *)0xd020 + 1;
    //}
    handle_music_buffer();
  }

  is_irq = 0;

  return;
}


void SWAP(uint8_t *a, uint8_t *b) 
{ 
  uint8_t tmp = *a; *a = *b; *b = tmp; 
}

void SWAP_int8(int8_t *a, int8_t *b) 
{ 
  int8_t tmp = *a; *a = *b; *b = tmp; 
}

void SWAP_ptr(uint8_t **a, uint8_t **b) 
{ 
  uint8_t *tmp = *a; *a = *b; *b = tmp; 
}

void SWAP_16(uint16_t *a, uint16_t *b) 
{ 
  uint16_t tmp = *a; *a = *b; *b = tmp; 
}

void SWAP_int(int16_t *a, int16_t *b) 
{ 
  int16_t tmp = *a; *a = *b; *b = tmp; 
}

uint16_t READ_UINT16(const void *ptr) 
{
  return *(const uint16_t *)(ptr);
}


void handle_music_buffer() // $33cd
{
  int8_t channel = 2;
  while (channel >= 0) {
    if ((statusBits1A & BITMASK[channel]) == 0 ||
        (busyChannelBits & BITMASK[channel]) != 0) {
      --channel;
      continue;
    }

    if (setup_song_file_data() == 1) {
      while(1) {
        *(volatile uint8_t *)0xd020 = *(volatile uint8_t *)0xd020 + 1;
      }
      return;
    }

    uint8_t *l_chanFileDataPtr = chanFileData[channel];

    uint16_t l_freq = 0;
    uint8_t l_keepFreq = 0;

    int16_t y = 0;
    uint8_t curByte = l_chanFileDataPtr[y++];

    // freq or 0/0xFF
    if (curByte == 0) {
      func_3674(channel);
      if (!isMusicPlaying)
        return;
      continue;
    } else if (curByte == 0xFF) {
      l_keepFreq = 1;
    } else {
      l_freq = FREQ_TBL[curByte];
    }

    uint8_t local1 = 0;
    curByte = l_chanFileDataPtr[y++];
    uint8_t isLastCmdByte = (curByte & 0x80) != 0;
    uint16_t curStepSum = stepTbl[curByte & 0x7f];

    for (uint8_t i = 0; !isLastCmdByte && (i < 2); ++i) {
      curByte = l_chanFileDataPtr[y++];
      isLastCmdByte = (curByte & 0x80) != 0;
      if (curByte & 0x40) {
        
        // note: bit used in zak theme (95) only (not used/handled in MM)
        //_music_timer = curByte & 0x3f;

      } else {
        local1 = curByte & 0x3f;
      }
    }

    chanFileData[channel] += y;
    chanDataOffset[channel] += y;

    uint8_t *l_chanBuf = get_resource(RES_ID_CHANNEL[channel]);

    if (local1 != 0) {
      // TODO: signed or unsigned?
      uint16_t offset = *NEAR_U16_PTR(RES_MAPPED + local1 * 2 + 12); //READ_LE_UINT16(&actSongFileData[local1*2 + 12]);
      l_chanFileDataPtr = NEAR_U8_PTR(RES_MAPPED + offset);

      // next five bytes: freqDelta, attack, sustain and phase bit
      for (uint8_t i = 0; i < 5; ++i) {
        l_chanBuf[15 + i] = l_chanFileDataPtr[i];
      }
      phaseBit[channel] = l_chanFileDataPtr[4];

      for (uint8_t i = 0; i < 17; ++i) {
        l_chanBuf[25 + i] = l_chanFileDataPtr[5 + i];
      }
    }

    if (l_keepFreq) {
      if (!releasePhase[channel]) {
        l_chanBuf[10] &= 0xfe; // release phase
      }
      releasePhase[channel] = 1;
    } else {
      if (releasePhase[channel]) {
        l_chanBuf[19] = phaseBit[channel];
        l_chanBuf[10] |= 0x01; // attack phase
      }
      l_chanBuf[11] = LOBYTE_(l_freq);
      l_chanBuf[12] = HIBYTE_(l_freq);
      releasePhase[channel] = 0;
    }

    // set counter value for frequency update (freqDeltaCounter)
    l_chanBuf[13] = LOBYTE_(curStepSum);
    l_chanBuf[14] = HIBYTE_(curStepSum);

    _soundQueue[channel] = RES_ID_CHANNEL[channel];
    process_song_data(channel);
    _soundQueue[channel+4] = RES_ID_CHANNEL[channel];
    process_song_data(channel+4);
    --channel;
  }
}

int8_t setup_song_file_data() // $36cb
{
  map_sound_res(res_page_music);

  // no new song
  songFileOrChanBufData = NEAR_U8_PTR(RES_MAPPED);

  if (res_page_music == act_res_page_music) {
    return 0;
  }

  // new song selected
  act_res_page_music = res_page_music;
  for (int i = 0; i < 3; ++i) {
    chanFileData[i] = NEAR_U8_PTR(RES_MAPPED + chanDataOffset[i]);
  }

  return -1;
}

void func_3674(uint8_t channel) // $3674
{
  statusBits1B &= BITMASK_INV[channel];
  if (statusBits1B == 0) {
    isMusicPlaying = 0;
    //unlockCodeLocation();
    safe_unlock_resource(resID_song);
    for (int i = 0; i < 3; ++i) {
      safe_unlock_resource(RES_ID_CHANNEL[i]);
    }
  }

  chanPrio[channel] = 2;

  statusBits1A &= BITMASK_INV[channel];
  phaseBit[channel] = 0;

  func_4F45(channel);
}

void reset_player_state() // $48f7
{
  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    sound_triggers[i] = 0;
  }

  for (int8_t i = 6; i >= 0; --i)
    release_channel(i);

  isMusicPlaying = 0;
  //unlockCodeLocation(); // does nothing
  statusBits1B = 0;
  statusBits1A = 0;
  freeChannelCount = 3;
  swapPrepared = 0;
  filterSwapped = 0;
  pulseWidthSwapped = 0;
  //var5163 = 0;
}

// channel: 0..6
void process_song_data(uint8_t channel) // $4939
{  
  // always: _soundQueue[channel] != -1
  // -> channelMap[channel] != -1
  
  //debug_out("spsd %d %d", channel, _soundQueue[channel]);
  channelMap[channel] = _soundQueue[channel];
  _soundQueue[channel] = -1;


  songPosUpdateCounter[channel] = 0;

  isVoiceChannel = (channel < 3);

  songFileOrChanBufOffset[channel] = vec6[channel];

  setup_song_ptr(channel);

  //vec5[channel] = songFileOrChanBufData; // not used

  if (songFileOrChanBufData == NULL) { // chanBuf (4C1C)
    /*
    // TODO: do we need this?
    LOBYTE_(vec20[channel]) = 0;
    LOBYTE_(songPosPtr[channel]) = LOBYTE_(songFileOrChanBufOffset[channel]);
    */
    release_resource_unk(channel);
    return;
  }

  vec20[channel] = songFileOrChanBufData; // chanBuf (4C1C)
  songPosPtr[channel] = songFileOrChanBufData + songFileOrChanBufOffset[channel]; // chanBuf (4C1C)
  uint8_t *ptr1 = songPosPtr[channel];

  int8_t y = -1;
  if (channel < 4) {
    ++y;
    if (channel == 3) {
      read_set_sid_filter_and_props(&y, ptr1);
    } else if (statusBits1A & BITMASK[channel]) {
      ++y;
    } else { // channel = 0/1/2
      waveCtrlReg[channel] = ptr1[y];

      ++y;
      if (ptr1[y] & 0x0f) {
        // filter on for voice channel
        SIDReg23 |= BITMASK[channel];
      } else {
        // filter off for voice channel
        SIDReg23 &= BITMASK_INV[channel];
      }
      sid_write(23, SIDReg23);
    }
  }

  save_song_pos(y, channel);
  busyChannelBits |= BITMASK[channel];
  read_song_chunk(channel);
}

void read_set_sid_filter_and_props(int8_t *offset, uint8_t *dataPtr) // $49e7
{
  SIDReg23 |= dataPtr[*offset];
  sid_write(23, SIDReg23);
  ++*offset;
  SIDReg24 = dataPtr[*offset];
  sid_write(24, SIDReg24);
}

void save_song_pos(int8_t y, uint8_t channel) 
{
  ++y;
  songPosPtr[channel] += y;
  songFileOrChanBufOffset[channel] += y;
}

// channel: 0..6
void update_freq(uint8_t channel) 
{
  isVoiceChannel = (channel < 3);

  --freqDeltaCounter[channel];
  if (freqDeltaCounter[channel] < 0) {
    read_song_chunk(channel);
  } else {
    freqReg[channel] += freqDelta[channel];
  }
  set_sid_freq_as(channel);
}

void reset_freq_delta(uint8_t channel) 
{
  freqDeltaCounter[channel] = 0;
  freqDelta[channel] = 0;
}

void read_song_chunk(uint8_t channel) // $4a6b
{
  while (1) {
    if (setup_song_ptr(channel) == 1) {
      // do something with code resource
      release_resource_unk(1);
      return;
    }

    uint8_t *ptr1 = songPosPtr[channel];

    //curChannelActive = true;

    uint8_t l_cmdByte = ptr1[0];
    if (l_cmdByte == 0) {
      //curChannelActive = false;
      songPosUpdateCounter[channel] = 0;

      var481A = -1;
      release_channel(channel);
      return;
    }

    //vec19[channel] = l_cmdByte;

    // attack (1) / release (0) phase
    if (isVoiceChannel) {
      if (GETBIT(l_cmdByte, 0))
        waveCtrlReg[channel] |= 0x01; // start attack phase
      else
        waveCtrlReg[channel] &= 0xfe; // start release phase
    }

    // channel finished bit
    if (GETBIT(l_cmdByte, 1)) {
      var481A = -1;
      release_channel(channel);
      return;
    }

    int8_t y = 0;

    // frequency
    if (GETBIT(l_cmdByte, 2)) {
      y += 2;
      freqReg[channel] = READ_LE_UINT16(&ptr1[y-1]);
      if (!GETBIT(l_cmdByte, 6)) {
        y += 2;
        freqDeltaCounter[channel] = READ_LE_UINT16(&ptr1[y-1]);
        y += 2;
        freqDelta[channel] = READ_LE_UINT16(&ptr1[y-1]);
      } else {
        reset_freq_delta(channel);
      }
    } else {
      reset_freq_delta(channel);
    }

    // attack / release
    if (isVoiceChannel && GETBIT(l_cmdByte, 3)) {
      // start release phase
      waveCtrlReg[channel] &= 0xfe;
      set_sid_wave_ctrl_reg(channel);

      ++y;
      attackReg[channel] = ptr1[y];
      ++y;
      sustainReg[channel] = ptr1[y];

      // set attack (1) or release (0) phase
      waveCtrlReg[channel]  |= (l_cmdByte & 0x01);
    }

    if (GETBIT(l_cmdByte, 4)) {
      ++y;
      uint8_t curByte = ptr1[y];

      // pulse width
      if (isVoiceChannel && GETBIT(curByte, 0)) {
        int reg = SID_REG_OFFSET[channel+4];

        y += 2;
        sid_write(reg, ptr1[y-1]);
        sid_write(reg+1, ptr1[y]);
      }

      if (GETBIT(curByte, 1)) {
        ++y;
        read_set_sid_filter_and_props(&y, ptr1);

        y += 2;
        sid_write(21, ptr1[y-1]);
        sid_write(22, ptr1[y]);
      }

      if (GETBIT(curByte, 2)) {
        reset_freq_delta(channel);

        y += 2;
        freqDeltaCounter[channel] = READ_LE_UINT16(&ptr1[y-1]);
      }
    }

    // set waveform (?)
    if (GETBIT(l_cmdByte, 5)) {
      ++y;
      waveCtrlReg[channel] = (waveCtrlReg[channel] & 0x0f) | ptr1[y];
    }

    // song position
    if (GETBIT(l_cmdByte, 7)) {
      //debug_out("srsc 7.1 %d", channel);

      if (songPosUpdateCounter[channel] == 1) {
        y += 2;
        --songPosUpdateCounter[channel];
        save_song_pos(y, channel);
      } else {
        chanloop[channel] = 1;

        // looping / skipping / ...
        ++y;
        songPosPtr[channel] -= (int8_t)ptr1[y];
        songFileOrChanBufOffset[channel] -= (int8_t)ptr1[y];

        ++y;
        if (songPosUpdateCounter[channel] == 0) {
          songPosUpdateCounter[channel] = ptr1[y];
        } else {
          --songPosUpdateCounter[channel];
        }
      }
    } else {
      save_song_pos(y, channel);
      return;
    }
  }
}

/**
 * Sets frequency, attack and sustain register
 */
void set_sid_freq_as(uint8_t channel) // $4be6
{
  if (swapVarLoaded)
    return;
  int8_t reg = SID_REG_OFFSET[channel];
  sid_write(reg,   LOBYTE_(freqReg[channel]));   // freq/pulseWidth voice 1/2/3
  sid_write(reg+1, HIBYTE_(freqReg[channel]));
  if (channel < 3) {
    sid_write(reg+5, attackReg[channel]); // attack
    sid_write(reg+6, sustainReg[channel]); // sustain
  }
}

void set_sid_wave_ctrl_reg(uint8_t channel) // $4C0D
{
  if (channel < 3) {
    int8_t reg = SID_REG_OFFSET[channel];
    sid_write(reg+4, waveCtrlReg[channel]);
  }
}

// channel: 0..6
int8_t setup_song_ptr(uint8_t channel) // $4C1C
{
  //resID:5,4,3,songid
  int8_t resID = channelMap[channel];

  __auto_type res_ptr = get_resource(resID);

  // TODO: when does this happen, only if resID == 0?
  if (res_ptr == NULL) {
    release_resource_unk(resID);
    if (resID == bgSoundResID) {
      bgSoundResID = 0;
      bgSoundActive = 0;
      swapPrepared = 0;
      pulseWidthSwapped = 0;
    }
    return 1;
  }

  songFileOrChanBufData = res_ptr; // chanBuf (4C1C)
  if (songFileOrChanBufData == vec20[channel]) {
    return 0;
  } else {
    vec20[channel] = songFileOrChanBufData;
    songPosPtr[channel] = songFileOrChanBufData + songFileOrChanBufOffset[channel];
    return -1;
  }
}

// chanResIndex: 3,4,5 or 58
void unlock_resource(int8_t chanResIndex) // $4CDA
{
  if (chanResIndex > 5) {
    for (uint8_t i = 0; i < NUM_RES_SLOTS; ++i) {
      if (res_data.id[i] == (uint8_t)chanResIndex) {
        res_data.count[i]--;
        if (res_data.count[i] == 0) {
          res_data.id[i] = 0xff; // 0xff = marked as to be deactivated
        };
      }
    }
  }
}

void count_free_channels() // $4f26
{
  freeChannelCount = 0;
  for (uint8_t i = 0; i < 3; ++i) {
    if (GETBIT(usedChannelBits, i) == 0)
      ++freeChannelCount;
  }
}

void func_4F45(uint8_t channel) // $4F45
{
  if (swapVarLoaded) {
    if (channel == 0) {
      swapPrepared = 0;
      reset_swap_vars();
    }
    pulseWidthSwapped = 0;
  } else {
    if (channel == 3) {
      filterUsed = 0;
    }

    if (chanPrio[channel] == 1) {
      if (var481A == 1)
        prepare_swap_vars(channel);
      else if (channel < 3)
        clear_sid_waveform(channel);
    } else if (channel < 3 && bgSoundActive && swapPrepared &&
        !(filterSwapped && filterUsed))
    {
      busyChannelBits |= BITMASK[channel];
      use_swap_vars(channel);
      waveCtrlReg[channel] |= 0x01;
      set_sid_wave_ctrl_reg(channel);

      safe_unlock_resource(channelMap[channel]);
      return;
    }

    chanPrio[channel] = 0;
    usedChannelBits &= BITMASK_INV[channel];
    count_free_channels();
  }

  int8_t resIndex = channelMap[channel];
  channelMap[channel] = 0;
  
  //debug_out("s4f45 %d %d", channel, resIndex);
  
  safe_unlock_resource(resIndex);
}

// chanResIndex: 3,4,5 or 58
void safe_unlock_resource(int8_t resIndex) // $4FEA
{
  if (!isMusicPlaying && resIndex > 5) {
    unlock_resource(resIndex);
  }
}

void release_resource(int8_t resIndex, uint8_t flags) // $5031
{
  release_res_channels(resIndex);
  if (resIndex == bgSoundResID && var481A == -1) {
    if (flags) {
      safe_unlock_resource(resIndex);
    }

    bgSoundResID = 0;
    bgSoundActive = 0;
    swapPrepared = 0;
    pulseWidthSwapped = 0;

    reset_swap_vars();
  }

}

void release_res_channels(int8_t resIndex) // $5070
{
  for (int8_t i = 3; i >= 0; --i) {
    if (resIndex == channelMap[i]) {
      release_channel(i);
    }
  }
}

void stop_sound_intern(int8_t soundResID, uint8_t flags) // $5093
{
  for (uint8_t i = 0; i < 7; ++i) {
    if (soundResID == _soundQueue[i]) {
      _soundQueue[i] = -1;
    }
  }
  var481A = -1;
  release_resource(soundResID, flags);
}

void stop_music_intern() // $4CAA
{
  statusBits1B = 0;
  isMusicPlaying = 0;

  if (resID_song != 0) {
    unlock_resource(resID_song);
  }

  chanPrio[0] = 2;
  chanPrio[1] = 2;
  chanPrio[2] = 2;

  statusBits1A = 0;
  phaseBit[0] = 0;
  phaseBit[1] = 0;
  phaseBit[2] = 0;
}

void release_resource_unk(int8_t resIndex) // $50A4
{
  var481A = -1;
  release_resource(resIndex, 1);
}

// a: 0..6
void release_channel(uint8_t channel) 
{
  stop_channel(channel);
  if (channel >= 4) {
    return;
  }
  if (channel < 3) {
    SIDReg23Stuff = SIDReg23;
    clear_sid_waveform(channel);
  }
  func_4F45(channel);
  if (channel >= 3) {
    return;
  }
  if ((SIDReg23 != SIDReg23Stuff) &&
      (SIDReg23 & 0x07) == 0)
  {
    if (filterUsed) {
      func_4F45(3);
      stop_channel(3);
    }
  }

  stop_channel(channel + 4);
}

void clear_sid_waveform(uint8_t channel) 
{
  if (!isMusicPlaying && var481A == -1) {
    waveCtrlReg[channel] &= 0x0e;
    set_sid_wave_ctrl_reg(channel);
  }
}

void stop_channel(uint8_t channel) 
{
  songPosUpdateCounter[channel] = 0;
  // clear "channel" bit
  busyChannelBits &= BITMASK_INV[channel];
  if (channel >= 4) {
    // pulsewidth = 0
    channelMap[channel] = 0;
  }
}

// channel: 0..6, swapIndex: 0..2
void swap_vars(uint8_t channel, int8_t swapIndex) // $51a5
{
  if (channel < 3) {
    SWAP(&attackReg[channel], &swapAttack[swapIndex]);
    SWAP(&sustainReg[channel], &swapSustain[swapIndex]);
  }
  //SWAP(vec5[channel],  swapVec5[swapIndex]);  // not used
  //SWAP(vec19[channel], swapVec19[swapIndex]); // not used

  SWAP(&chanPrio[channel], &swapSongPrio[swapIndex]);
  SWAP_int8(&channelMap[channel], &swapVec479C[swapIndex]);
  SWAP(&songPosUpdateCounter[channel], &swapSongPosUpdateCounter[swapIndex]);
  SWAP(&waveCtrlReg[channel], &swapWaveCtrlReg[swapIndex]);
  SWAP_ptr(&songPosPtr[channel],  &swapSongPosPtr[swapIndex]);
  SWAP_16(&freqReg[channel],  &swapFreqReg[swapIndex]);
  SWAP_int(&freqDeltaCounter[channel], &swapVec11[swapIndex]);
  SWAP_16(&freqDelta[channel], &swapVec10[swapIndex]);
  SWAP_ptr(&vec20[channel], &swapVec20[swapIndex]);
  SWAP_int(&songFileOrChanBufOffset[channel],  &swapVec8[swapIndex]);
}

void reset_swap_vars() // $52d0
{
  for (int i = 0; i < 2; ++i) {
    swapAttack[i] = 0;
    swapSustain[i] = 0;
  }
  for (int i = 0; i < 3; ++i) {
    swapVec5[i] = NULL;
    swapSongPrio[i] = 0;
    swapVec479C[i] = 0;
    swapVec19[i] = 0;
    swapSongPosUpdateCounter[i] = 0;
    swapWaveCtrlReg[i] = 0;
    swapSongPosPtr[i] = NULL;
    swapFreqReg[i] = 0;
    swapVec11[i] = 0;
    swapVec10[i] = 0;
    swapVec20[i] = NULL;
    swapVec8[i] = 0;
  }
}

void prepare_swap_vars(uint8_t channel) // $52E5
{
  if (channel >= 4)
    return;

  if (channel < 3) {
    if (!keepSwapVars) {
      reset_swap_vars();
    }
    swap_vars(channel, 0);
    if (busyChannelBits & BITMASK[channel+4]) {
      swap_vars(channel+4, 1);
      pulseWidthSwapped = 1;
    }
  } else if (channel == 3) {
    SIDReg24_HiNibble = SIDReg24 & 0x70;
    reset_swap_vars();
    keepSwapVars = 1;
    swap_vars(3, 2);
    filterSwapped = 1;
  }
  swapPrepared = 1;
}

void use_swap_vars(uint8_t channel) // $5342
{
  if (channel >= 3)
    return;

  swap_vars(channel, 0);
  set_sid_freq_as(channel);
  if (pulseWidthSwapped) {
    swap_vars(channel+4, 1);
    set_sid_freq_as(channel+4);
  }
  if (filterSwapped) {
    swap_vars(3, 2);

    // resonating filter freq. or voice-to-filter mapping?
    SIDReg23 = (SIDReg23Stuff & 0xf0) | BITMASK[channel];
    sid_write(23, SIDReg23);

    // filter props
    SIDReg24 = (SIDReg24 & 0x0f) | SIDReg24_HiNibble;
    sid_write(24, SIDReg24);

    // filter freq.
    sid_write(21, LOBYTE_(freqReg[3]));
    sid_write(22, HIBYTE_(freqReg[3]));
  } else {
    SIDReg23 = SIDReg23Stuff & BITMASK_INV[channel];
    sid_write(23, SIDReg23);
  }

  swapPrepared = 0;
  pulseWidthSwapped = 0;
  keepSwapVars = 0;
  SIDReg24_HiNibble = 0;
  filterSwapped = 0;
}

// ignore: no effect
// resIndex: 3,4,5 or 58
void lock_resource(int8_t resIndex) { // $4ff4
  if (!isMusicPlaying && resIndex > 5) {
    //++resStatus[resIndex];
    for (uint8_t i = 0; i < NUM_RES_SLOTS; ++i) {
      if (res_data.id[i] == resIndex) {
        res_data.count[i]++;
        break;
      }
    }
  }
}

void reserve_channel(uint8_t channel, uint8_t prioValue, int8_t chanResIndex) // $4ffe
{
  if (channel == 3) {
    filterUsed = 1;
  } else if (channel < 3) {
    usedChannelBits |= BITMASK[channel];
    count_free_channels();
  }

  chanPrio[channel] = prioValue;

  lock_resource(chanResIndex);
}

// ignore: no effect
/*void unlockCodeLocation() { // $513e
  resStatus[1] &= 0x80;
  resStatus[2] &= 0x80;
}*/

// ignore: no effect
/*void lockCodeLocation() { // $514f
  resStatus[1] |= 0x01;
  resStatus[2] |= 0x01;
}*/

void init_music(int8_t songResIndex, uint8_t res_page) // $7de6
{
  //unlockResource(resID_song);

  resID_song = songResIndex;
  //_music = getResource(resID_song);
  res_page_music = res_page;

  // song base address
  //actSongFileData = _music;

  initializing = 1;
  _soundInQueue = 0;
  isMusicPlaying = 0;

  //unlockCodeLocation();
  reset_player_state();

  //lockResource(resID_song);
  build_step_tbl(*NEAR_U8_PTR(RES_MAPPED + 5));

  // fetch sound
  songChannelBits = *NEAR_U8_PTR(RES_MAPPED + 4);
  for (int8_t i = 2; i >= 0; --i) {
    if ((songChannelBits & BITMASK[i]) != 0) {
      func_7eae(i);
    }
  }

  isMusicPlaying = 1;
  //lockCodeLocation();

  SIDReg23 &= 0xf0;
  sid_write(23, SIDReg23);

  handle_music_buffer();

  initializing = 0;
  _soundInQueue = 1;
}

// params:
//   channel: channel 0..2
void func_7eae(uint8_t channel) 
{
  uint8_t pos = SONG_CHANNEL_OFFSET[channel];
  chanDataOffset[channel] = *NEAR_U16_PTR(RES_MAPPED + pos); // READ_LE_UINT16(&songFileDataPtr[pos]);
  chanFileData[channel] = NEAR_U8_PTR(RES_MAPPED + chanDataOffset[channel]);

  //vec5[channel+4] = vec5[channel] = CHANNEL_BUFFER_ADDR[RES_ID_CHANNEL[channel]]; // not used
  vec6[channel+4] = 0x0019;
  vec6[channel]   = 0x0008;

  func_819b(channel);

  waveCtrlReg[channel] = 0;
}

void func_819b(uint8_t channel) 
{
  reserve_channel(channel, 127, RES_ID_CHANNEL[channel]);

  statusBits1B |= BITMASK[channel];
  statusBits1A |= BITMASK[channel];
}

void build_step_tbl(uint8_t step) // $82B4
{
  stepTbl[0] = 0;
  stepTbl[1] = step - 2;
  for (uint8_t i = 2; i < 33; ++i) {
    stepTbl[i] = stepTbl[i-1] + step;
  }
}

uint8_t reserve_sound_filter(uint8_t value, uint8_t chanResIndex) // $4ED0
{
  uint8_t channel = 3;
  reserve_channel(channel, value, chanResIndex);
  return channel;
}

uint8_t reserve_sound_voice(uint8_t value, uint8_t chanResIndex) // $4EB8
{
  for (int8_t i = 2; i >= 0; --i) {
    if ((usedChannelBits & BITMASK[i]) == 0) {
      reserve_channel(i, value, chanResIndex);
      return i;
    }
  }
  return 0;
}

void find_less_prio_channels(uint8_t soundPrio) // $4ED8
{
  minChanPrio = 127;

  chansWithLowerPrioCount = 0;
  for (int8_t i = 2; i >= 0; --i) {
    if (usedChannelBits & BITMASK[i]) {
      if (chanPrio[i] < soundPrio)
        ++chansWithLowerPrioCount;
      if (chanPrio[i] < minChanPrio) {
        minChanPrio = chanPrio[i];
        minChanPrioIndex = i;
      }
    }
  }

  if (chansWithLowerPrioCount == 0)
    return;

  if (soundPrio >= chanPrio[3]) {
    actFilterHasLowerPrio = 1;
  } else {
    /* TODO: is this really a no-op?
    if (minChanPrioIndex < chanPrio[3])
      minChanPrioIndex = minChanPrioIndex;
    */

    actFilterHasLowerPrio = 0;
  }
}

void release_resource_by_sound(int8_t resID) // $5088
{
  var481A = 1;
  release_resource(resID, 1);
}

void read_vec6_data(int8_t x, int8_t *offset, int8_t chanResID) // $4E99
{
  //vec5[x] = songFilePtr;
  vec6[x] = NEAR_U8_PTR(RES_MAPPED)[*offset];
  *offset += 2;
  _soundQueue[x] = chanResID;
}

int8_t init_sound(int8_t soundResID, uint8_t res_page) // $4D0A
{
  initializing = 1;

  if (isMusicPlaying && (statusBits1A & 0x07) == 0x07) {
    initializing = 0;
    return -2;
  }

  //uint8_t __far *songFilePtr = getResource(soundResID);

  uint8_t soundPrio = *NEAR_U8_PTR(RES_MAPPED + 4);
  // for (mostly but not always looped) background sounds
  if (soundPrio == 1) {
    bgSoundResID = soundResID;
    bgSoundActive = 1;
  }

  uint8_t requestedChannels = 0;
  if ((*NEAR_U8_PTR(RES_MAPPED + 5) & 0x40) == 0) {
    ++requestedChannels;
    if (*NEAR_U8_PTR(RES_MAPPED + 5) & 0x02)
      ++requestedChannels;
    if (*NEAR_U8_PTR(RES_MAPPED + 5) & 0x08)
      ++requestedChannels;
  }

  uint8_t filterNeeded = (*NEAR_U8_PTR(RES_MAPPED + 5) & 0x20) != 0;
  uint8_t filterBlocked = (filterUsed && filterNeeded);
  if (filterBlocked || (freeChannelCount < requestedChannels)) {
    find_less_prio_channels(soundPrio);

    if ((freeChannelCount + chansWithLowerPrioCount < requestedChannels) ||
        (filterBlocked && !actFilterHasLowerPrio)) {
      initializing = 0;
      return -1;
    }

    if (filterBlocked) {
      if (soundPrio < chanPrio[3]) {
        initializing = 0;
        return -1;
      }

      uint8_t l_resID = channelMap[3];
      release_resource_by_sound(l_resID);
    }

    while ((freeChannelCount < requestedChannels) || (filterNeeded && filterUsed)) {
      find_less_prio_channels(soundPrio);
      if (minChanPrio >= soundPrio) {
        initializing = 0;
        return -1;
      }

      uint8_t l_resID = channelMap[minChanPrioIndex];
      release_resource_by_sound(l_resID);
    }
  }

  int8_t x;
  uint8_t soundByte5 = *NEAR_U8_PTR(RES_MAPPED + 5);
  if (soundByte5 & 0x40)
    x = reserve_sound_filter(soundPrio, soundResID);
  else
    x = reserve_sound_voice(soundPrio, soundResID);

  uint8_t var4CF3 = x;
  int8_t y = 6;
  if (soundByte5 & 0x01) {
    x += 4;
    read_vec6_data(x, &y, soundResID);
  }
  if (soundByte5 & 0x02) {
    x = reserve_sound_voice(soundPrio, soundResID);
    read_vec6_data(x, &y, soundResID);
  }
  if (soundByte5 & 0x04) {
    x += 4;
    read_vec6_data(x, &y, soundResID);
  }
  if (soundByte5 & 0x08) {
    x = reserve_sound_voice(soundPrio, soundResID);
    read_vec6_data(x, &y, soundResID);
  }
  if (soundByte5 & 0x10) {
    x += 4;
    read_vec6_data(x, &y, soundResID);
  }
  if (soundByte5 & 0x20) {
    x = reserve_sound_filter(soundPrio, soundResID);
    read_vec6_data(x, &y, soundResID);
  }

  //vec5[var4CF3] = songFilePtr;
  vec6[var4CF3] = y;
  _soundQueue[var4CF3] = soundResID;

  chanloop[var4CF3] = 0;


  initializing = 0;
  _soundInQueue = 1;

  return soundResID;
}

uint8_t *get_resource(int8_t resID) 
{
  if (resID > 0 && resID < 3) {
    fatal_error(ERR_INVALID_SID_RES_ID);
  }
  else if (resID > 2 && resID < 6) {
    return   (uint8_t *)chanBuffer[resID-3];
  }
  else {
    for (uint8_t i = 0; i < NUM_RES_SLOTS; ++i) {
      if (res_data.id[i] == (uint8_t)resID) {
        map_sound_res(res_data.page[i]);
        return NEAR_U8_PTR(RES_MAPPED);
      }
    }
  }
  return NULL;
}

//int readBuffer(int16_t *buffer, const int numSamples) {
  /*int samplesLeft = numSamples;

  Common::StackLock lock(_mutex);

  while (samplesLeft > 0) {
    // update SID status after each frame
    if (_cpuCyclesLeft <= 0) {
      update();
      _cpuCyclesLeft = timingProps[_videoSystem].cyclesPerFrame;
    }
    // fetch samples
    int sampleCount = _sid->updateClock(_cpuCyclesLeft, (short *)buffer, samplesLeft);
    samplesLeft -= sampleCount;
    buffer += sampleCount;
  }*/

  //return numSamples;
//}

void sid_write(uint8_t reg, uint8_t data) 
{
  //_sid->write(reg, data);
  *(volatile uint8_t *)(0xd400 + reg) = data;
  *(volatile uint8_t *)(0xd440 + reg) = data;
}


void start_sound(int8_t nr) 
{
  uint8_t res_page;
  uint8_t i;
  for (i = 0; i < NUM_RES_SLOTS; ++i) {
    if (res_data.id[i] == nr) {
      res_data.count[i]++;
      res_page = res_data.page[i];
      break;
    }
  }

  if (i == NUM_RES_SLOTS) {
    res_page = res_provide(RES_TYPE_SOUND_SID, nr, 0);
    res_activate_slot(res_page);
    for (uint8_t i = 0; i < NUM_RES_SLOTS; ++i) {
      if (res_data.id[i] == 0) {
        res_data.id[i] = (uint8_t)nr;
        res_data.page[i] = res_page;
        res_data.count[i] = 1;
        break;      
      }
    }
  }
  map_sound_res(res_page);

  // WORKAROUND:
  // sound[4] contains either a song prio or a music channel usage byte.
  // As music channel usage is always 0x07 for all music files and
  // prio 7 is never used in any sound file use this byte for auto-detection.
  uint8_t isMusic = (*NEAR_U8_PTR(RES_MAPPED + 4) == 0x07);

  //Common::StackLock lock(_mutex);

  __asm volatile (
    "   sei "
    :::
  );

  if (isMusic) {
    init_music(nr, res_page);
  } else {
    stop_sound_intern(nr, 0);
    init_sound(nr, res_page);
  }

  __asm volatile (
    "   cli "
    :::
  );
}

void stop_sound(int8_t nr) 
{
  //debug_out("sstp: %d", nr);

  if (nr == -1) {
    return;
  }

  //Common::StackLock lock(_mutex);
  stop_sound_intern(nr, 1);
  //releaseResource(nr, 1);
}

int8_t get_sound_status(int8_t nr) 
{
  int result = 0;

  //Common::StackLock lock(_mutex);

  if (resID_song == nr && isMusicPlaying) {
    result = 1;
    return result;
  }

  for (int i = 0; (i < 4) /*&& (result == 0)*/; ++i) {
    if (nr == _soundQueue[i] || nr == channelMap[i]) {
      //debug_out("scl %d %d", i, chanloop[i]);

      result |= 1;

      if (chanloop[i]) {
        result = 0;
        break;
      }
    }
  }

  //debug_out("sgst %d %d", nr, result);

  return result;
}

void map_sound_res(uint8_t page)
{
  if (is_irq) {
    __asm(" lda #0x40\n"
          " ldx #0x21\n"
          " ldz #0x31\n"
          " map\n"
          " eom\n"
          :
          : "Ky" (page)
          : "a", "x", "y", "z");
  }
  else {
    map_ds_resource(page);
  }
}


//int8_t getMusicTimer() {
  /*int8_t result = _music_timer;
  _music_timer = 0;
  
  return result;*/
  
  //return 0;
//}
