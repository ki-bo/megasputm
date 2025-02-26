//#include <stdlib.h>
//#include <string.h>


#include "c64sound.h"
#include "resource.h"
#include "map.h"

#include "util.h"


#define ZEROMEM(a) memset(a, 0, sizeof(a))

#define READ_LE_UINT16(a) READ_UINT16(a)

#define NUM_SOUND_SLOTS 6








#pragma clang section data="data_sound" rodata="cdata_sound" bss="zdata"







uint8_t sound_triggers[NUM_SOUND_SLOTS];
uint8_t __far *resourceData;

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


uint8_t __far *_music;

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

int8_t resID_song;

// statusBits1A/1B are always equal
uint8_t statusBits1A;
uint8_t statusBits1B;

uint8_t busyChannelBits;

uint8_t SIDReg23;
uint8_t SIDReg23Stuff;
uint8_t SIDReg24;

uint8_t __far *chanFileData[3];
uint16_t chanDataOffset[3];
uint8_t __far *songPosPtr[7];

// 0..2: freq value voice1/2/3
// 3:    filter freq
// 4..6: pulse width
uint16_t freqReg[7];

// start offset[i] for songFileOrChanBufData to obtain songPosPtr[i]
//	vec6[0..2] = 0x0008;
//	vec6[4..6] = 0x0019;
int16_t vec6[7];

// current offset[i] for songFileOrChanBufData to obtain songPosPtr[i] (starts with vec6[i], increased later)
int16_t songFileOrChanBufOffset[7];

uint16_t freqDelta[7];
int16_t freqDeltaCounter[7];
uint8_t __far *swapSongPosPtr[3];
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

uint8_t __far *vec20[7];

uint8_t __far *swapVec20[3];

// resource status (never read)
// bit7: some flag
// bit6..0: counter (use-count?), maybe just bit0 as flag (used/unused?)
//uint8_t resStatus[70];

uint8_t __far *songFileOrChanBufData;
uint8_t __far *actSongFileData;

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



void SID_Write(uint8_t reg, uint8_t data);
void initSID();
uint8_t __far *getResource(int8_t resID);

void startSound(int8_t nr);
void stopSound(int8_t nr);
int8_t getSoundStatus(int8_t nr);


void initMusic(int8_t songResIndex, uint8_t __far *data); // $7de6
int8_t initSound(int8_t soundResID, uint8_t __far *data); // $4D0A
void stopSound_intern(int8_t soundResID, uint8_t flags); // $5093
void stopMusic_intern(); // $4CAA

void resetSID(); // $48D8
void handleMusicBuffer();
int8_t setupSongFileData(); // $36cb
void func_3674(uint8_t channel); // $3674
void resetPlayerState(); // $48f7
void processSongData(uint8_t channel); // $4939
void readSetSIDFilterAndProps(int8_t *offset, uint8_t __far *dataPtr);  // $49e7
void saveSongPos(int8_t y, uint8_t channel);
void updateFreq(uint8_t channel);
void resetFreqDelta(uint8_t channel);
void readSongChunk(uint8_t channel); // $4a6b
void setSIDFreqAS(uint8_t channel); // $4be6
void setSIDWaveCtrlReg(uint8_t channel); // $4C0D
int8_t setupSongPtr(uint8_t channel); // $4C1C
//void unlockResource(int8_t chanResIndex); // $4CDA
void countFreeChannels(); // $4f26
void func_4F45(uint8_t channel); // $4F45
//void safeUnlockResource(int8_t resIndex); // $4FEA
void releaseResource(int8_t resIndex, uint8_t flags); // $5031
void releaseResChannels(int8_t resIndex); // $5070
void releaseResourceUnk(int8_t resIndex); // $50A4
void releaseChannel(uint8_t channel);
void clearSIDWaveform(uint8_t channel);
void stopChannel(uint8_t channel);
void swapVars(uint8_t channel, int8_t swapIndex); // $51a5
void resetSwapVars(); // $52d0
void prepareSwapVars(uint8_t channel); // $52E5
void useSwapVars(uint8_t channel); // $5342
//void lockResource(int8_t resIndex); // $4ff4
void reserveChannel(uint8_t channel, uint8_t prioValue, int8_t chanResIndex); // $4ffe
//void unlockCodeLocation(); // $513e
//void lockCodeLocation(); // $514f
void func_7eae(uint8_t channel, uint8_t __far *songFileDataPtr); // $7eae
void func_819b(uint8_t channel); // $819b
void buildStepTbl(uint8_t step); // $82B4
uint8_t reserveSoundFilter(uint8_t value, uint8_t chanResIndex); // $4ED0
uint8_t reserveSoundVoice(uint8_t value, uint8_t chanResIndex); // $4EB8
void findLessPrioChannels(uint8_t soundPrio); // $4ED8
void releaseResourceBySound(int8_t resID); // $5088
void readVec6Data(int8_t x, int8_t *offset, uint8_t __far *songFilePtr, int8_t chanResID); // $4E99

void unused1(); // $50AF


#pragma clang section text="code_init" data="data_init" rodata="cdata_init" bss="bss_init"

void c64_sound_init(void) {

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

	initSID();
	resetSID();

	//_mixer->playStream(Audio::Mixer::kPlainSoundType, &_soundHandle, this, -1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO, true);
}

void c64_sound_fin() {
	//_mixer->stopHandle(_soundHandle);
	//delete _sid;
}


void initSID() {
  /*	_sid = new Resid::SID();
    _sid->set_sampling_parameters(
      timingProps[_videoSystem].clockFreq,
      _sampleRate);
    _sid->enable_filter(true);*/
  
  
  
    //_sid->reset();
  
    //Synchronize the waveform generators (must occur after reset)
    SID_Write(4, 0x08);
    SID_Write(11, 0x08);
    SID_Write(18, 0x08);
    SID_Write(4, 0x00);
    SID_Write(11, 0x00);
    SID_Write(18, 0x00);
  }
  
  void resetSID() { // $48D8
    SIDReg24 = 0x0f;
  
    SID_Write(4, 0);
    SID_Write(11, 0);
    SID_Write(18, 0);
    SID_Write(23, 0);
    SID_Write(21, 0);
    SID_Write(22, 0);
    SID_Write(24, SIDReg24);
  
    resetPlayerState();
  }
  

#pragma clang section text="code_main" data="data_main" rodata="cdata_main" bss="zdata"


void c64_startSound(uint8_t sound_id) {
  SAVE_CS_AUTO_RESTORE
  MAP_CS_SOUND

  //if (sound_id > 60) {
    //sound_id = 58;
  //}

  //if (sound_id != 58) {
    //return;
  //}

  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    if (!sound_triggers[i]) {
      sound_triggers[i] = sound_id;
      // we do a first res_provide() for each sound to avoid loading latency in sound_handle_play_triggers() later
      res_provide(RES_TYPE_C64SOUND, sound_id, 0);
      return;
    }
  }
}

void c64_sound_handle_play_triggers(void)
{
  SAVE_CS_AUTO_RESTORE
  MAP_CS_SOUND

  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    if (sound_triggers[i]) {
      startSound((int8_t)sound_triggers[i]);
      sound_triggers[i] = 0;
    }
  }
}

void c64_stopSound(uint8_t sound_id) {
  SAVE_CS_AUTO_RESTORE
  MAP_CS_SOUND

  //return;

  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    if (sound_triggers[i] == sound_id) {
      sound_triggers[i] = 0;
    }
  }
  stopSound((int8_t)sound_id);
}

void c64_stopAllSounds() {
  SAVE_CS_AUTO_RESTORE
  MAP_CS_SOUND

	//Common::StackLock lock(_mutex);
	resetPlayerState();
}

uint8_t c64_sound_is_playing(uint8_t sound_id) {
  SAVE_CS_AUTO_RESTORE
  MAP_CS_SOUND

  return (uint8_t)getSoundStatus((int8_t)sound_id);
}



#pragma clang section text="code_sound" data="data_sound" rodata="cdata_sound" bss="bss_sound"



void c64_sound_update() { // $481B
  //SAVE_CS_AUTO_RESTORE
  //MAP_CS_SOUND

	if (initializing)
		return;

	if (_soundInQueue) {
		for (int8_t i = 6; i >= 0; --i) {
			if (_soundQueue[i] != -1)
				processSongData(i);
		}
		_soundInQueue = 0;
	}

	// no sound
	if (busyChannelBits == 0)
		return;

	for (int8_t i = 6; i >= 0; --i) {
		if (busyChannelBits & BITMASK[i]) {
			updateFreq(i);
		}
	}

	// seems to be used for background (prio=1?) sounds.
	// If a bg sound cannot be played because all SID
	// voices are used by higher priority sounds, the
	// bg sound's state is updated here so it will be at
	// the correct state when a voice is available again.
	if (swapPrepared) {
		swapVars(0, 0);
		swapVarLoaded = 1;
		updateFreq(0);
		swapVars(0, 0);
		if (pulseWidthSwapped) {
			swapVars(4, 1);
			updateFreq(4);
			swapVars(4, 1);
		}
		swapVarLoaded = 0;
	}

	for (int8_t i = 6; i >= 0; --i) {
		if (busyChannelBits & BITMASK[i])
			setSIDWaveCtrlReg(i);
	};

	if (isMusicPlaying) {
    //while(1) {
      //*(volatile uint8_t *)0xd020 =  *(volatile uint8_t *)0xd020 + 1;
    //}
		handleMusicBuffer();
	}

	return;
}


void SWAP(uint8_t *a, uint8_t *b) { 
  uint8_t tmp = *a; *a = *b; *b = tmp; 
}

void SWAP_int8(int8_t *a, int8_t *b) { 
  int8_t tmp = *a; *a = *b; *b = tmp; 
}

void SWAP_ptr(uint8_t __far **a, uint8_t __far **b) { 
  uint8_t __far *tmp = *a; *a = *b; *b = tmp; 
}

void SWAP_16(uint16_t *a, uint16_t *b) { 
  uint16_t tmp = *a; *a = *b; *b = tmp; 
}

void SWAP_int(int16_t *a, int16_t *b) { 
  int16_t tmp = *a; *a = *b; *b = tmp; 
}

uint16_t READ_UINT16(const void __far *ptr) {
	return *(const uint16_t __far *)(ptr);
}


void handleMusicBuffer() { // $33cd
	int8_t channel = 2;
	while (channel >= 0) {
		if ((statusBits1A & BITMASK[channel]) == 0 ||
		    (busyChannelBits & BITMASK[channel]) != 0) {
			--channel;
			continue;
		}

		if (setupSongFileData() == 1) {
      while(1) {
        *(volatile uint8_t *)0xd020 = *(volatile uint8_t *)0xd020 + 1;
      }
			return;
    }

		uint8_t __far *l_chanFileDataPtr = chanFileData[channel];

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

		uint8_t __far *l_chanBuf = getResource(RES_ID_CHANNEL[channel]);

		if (local1 != 0) {
			// TODO: signed or unsigned?
			uint16_t offset = READ_LE_UINT16(&actSongFileData[local1*2 + 12]);
			l_chanFileDataPtr = actSongFileData + offset;

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
		processSongData(channel);
		_soundQueue[channel+4] = RES_ID_CHANNEL[channel];
		processSongData(channel+4);
		--channel;
	}
}

int8_t setupSongFileData() { // $36cb
	// no song playing
	// TODO: remove (never NULL)
	if (_music == NULL) {
		for (int8_t i = 2; i >= 0; --i) {
			if (songChannelBits & BITMASK[i]) {
				func_3674(i);
			}
		}
		return 1;
	}

	// no new song
	songFileOrChanBufData = _music;
	if (_music == actSongFileData) {
		return 0;
	}

	// new song selected
	actSongFileData = _music;
	for (int i = 0; i < 3; ++i) {
		chanFileData[i] = _music + chanDataOffset[i];
	}

	return -1;
}

void func_3674(uint8_t channel) { // $3674
	statusBits1B &= BITMASK_INV[channel];
	if (statusBits1B == 0) {
		isMusicPlaying = 0;
		//unlockCodeLocation();
		//safeUnlockResource(resID_song);
		//for (int i = 0; i < 3; ++i) {
			//safeUnlockResource(RES_ID_CHANNEL[i]);
		//}
	}

	chanPrio[channel] = 2;

	statusBits1A &= BITMASK_INV[channel];
	phaseBit[channel] = 0;

	func_4F45(channel);
}

void resetPlayerState() { // $48f7
  for (uint8_t i = 0; i < NUM_SOUND_SLOTS; ++i) {
    sound_triggers[i] = 0;
  }

	for (int8_t i = 6; i >= 0; --i)
		releaseChannel(i);

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
void processSongData(uint8_t channel) { // $4939
	// always: _soundQueue[channel] != -1
	// -> channelMap[channel] != -1
	
  //debug_out("spsd %d %d", channel, _soundQueue[channel]);
  
  channelMap[channel] = _soundQueue[channel];
	_soundQueue[channel] = -1;


	songPosUpdateCounter[channel] = 0;

	isVoiceChannel = (channel < 3);

	songFileOrChanBufOffset[channel] = vec6[channel];

	setupSongPtr(channel);

	//vec5[channel] = songFileOrChanBufData; // not used

	if (songFileOrChanBufData == NULL) { // chanBuf (4C1C)
		/*
		// TODO: do we need this?
		LOBYTE_(vec20[channel]) = 0;
		LOBYTE_(songPosPtr[channel]) = LOBYTE_(songFileOrChanBufOffset[channel]);
		*/
		releaseResourceUnk(channel);
		return;
	}

	vec20[channel] = songFileOrChanBufData; // chanBuf (4C1C)
	songPosPtr[channel] = songFileOrChanBufData + songFileOrChanBufOffset[channel]; // chanBuf (4C1C)
	uint8_t __far *ptr1 = songPosPtr[channel];

	int8_t y = -1;
	if (channel < 4) {
		++y;
		if (channel == 3) {
			readSetSIDFilterAndProps(&y, ptr1);
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
			SID_Write(23, SIDReg23);
		}
	}

	saveSongPos(y, channel);
	busyChannelBits |= BITMASK[channel];
	readSongChunk(channel);
}

void readSetSIDFilterAndProps(int8_t *offset, uint8_t __far *dataPtr) {  // $49e7
	SIDReg23 |= dataPtr[*offset];
	SID_Write(23, SIDReg23);
	++*offset;
	SIDReg24 = dataPtr[*offset];
	SID_Write(24, SIDReg24);
}

void saveSongPos(int8_t y, uint8_t channel) {
	++y;
	songPosPtr[channel] += y;
	songFileOrChanBufOffset[channel] += y;
}

// channel: 0..6
void updateFreq(uint8_t channel) {
	isVoiceChannel = (channel < 3);

	--freqDeltaCounter[channel];
	if (freqDeltaCounter[channel] < 0) {
		readSongChunk(channel);
	} else {
		freqReg[channel] += freqDelta[channel];
	}
	setSIDFreqAS(channel);
}

void resetFreqDelta(uint8_t channel) {
	freqDeltaCounter[channel] = 0;
	freqDelta[channel] = 0;
}

void readSongChunk(uint8_t channel) { // $4a6b
	while (1) {
		if (setupSongPtr(channel) == 1) {
			// do something with code resource
			releaseResourceUnk(1);
			return;
		}

		uint8_t __far *ptr1 = songPosPtr[channel];

		//curChannelActive = true;

		uint8_t l_cmdByte = ptr1[0];
		if (l_cmdByte == 0) {
			//curChannelActive = false;
			songPosUpdateCounter[channel] = 0;

			var481A = -1;
			releaseChannel(channel);
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
			releaseChannel(channel);
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
				resetFreqDelta(channel);
			}
		} else {
			resetFreqDelta(channel);
		}

		// attack / release
		if (isVoiceChannel && GETBIT(l_cmdByte, 3)) {
			// start release phase
			waveCtrlReg[channel] &= 0xfe;
			setSIDWaveCtrlReg(channel);

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
				SID_Write(reg, ptr1[y-1]);
				SID_Write(reg+1, ptr1[y]);
			}

			if (GETBIT(curByte, 1)) {
				++y;
				readSetSIDFilterAndProps(&y, ptr1);

				y += 2;
				SID_Write(21, ptr1[y-1]);
				SID_Write(22, ptr1[y]);
			}

			if (GETBIT(curByte, 2)) {
				resetFreqDelta(channel);

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
      chanloop[channel] = 1;

      //debug_out("srsc 7.1 %d", channel);

			if (songPosUpdateCounter[channel] == 1) {
				y += 2;
				--songPosUpdateCounter[channel];
				saveSongPos(y, channel);
			} else {
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
			saveSongPos(y, channel);
			return;
		}
	}
}

/**
 * Sets frequency, attack and sustain register
 */
void setSIDFreqAS(uint8_t channel) { // $4be6
	if (swapVarLoaded)
		return;
	int8_t reg = SID_REG_OFFSET[channel];
	SID_Write(reg,   LOBYTE_(freqReg[channel]));   // freq/pulseWidth voice 1/2/3
	SID_Write(reg+1, HIBYTE_(freqReg[channel]));
	if (channel < 3) {
		SID_Write(reg+5, attackReg[channel]); // attack
		SID_Write(reg+6, sustainReg[channel]); // sustain
	}
}

void setSIDWaveCtrlReg(uint8_t channel) { // $4C0D
	if (channel < 3) {
		int8_t reg = SID_REG_OFFSET[channel];
		SID_Write(reg+4, waveCtrlReg[channel]);
	}
}

// channel: 0..6
int8_t setupSongPtr(uint8_t channel) { // $4C1C
	//resID:5,4,3,songid
	int8_t resID = channelMap[channel];

	// TODO: when does this happen, only if resID == 0?
	if (getResource(resID) == NULL) {
		releaseResourceUnk(resID);
		if (resID == bgSoundResID) {
			bgSoundResID = 0;
			bgSoundActive = 0;
			swapPrepared = 0;
			pulseWidthSwapped = 0;
		}
		return 1;
	}

	songFileOrChanBufData = getResource(resID); // chanBuf (4C1C)
	if (songFileOrChanBufData == vec20[channel]) {
		return 0;
	} else {
		vec20[channel] = songFileOrChanBufData;
		songPosPtr[channel] = songFileOrChanBufData + songFileOrChanBufOffset[channel];
		return -1;
	}
}

// ignore: no effect
// chanResIndex: 3,4,5 or 58
/*void unlockResource(int8_t chanResIndex) { // $4CDA
	if ((resStatus[chanResIndex] & 0x7F) != 0)
		--resStatus[chanResIndex];
}*/

void countFreeChannels() { // $4f26
	freeChannelCount = 0;
	for (uint8_t i = 0; i < 3; ++i) {
		if (GETBIT(usedChannelBits, i) == 0)
			++freeChannelCount;
	}
}

void func_4F45(uint8_t channel) { // $4F45
	if (swapVarLoaded) {
		if (channel == 0) {
			swapPrepared = 0;
			resetSwapVars();
		}
		pulseWidthSwapped = 0;
	} else {
		if (channel == 3) {
			filterUsed = 0;
		}

		if (chanPrio[channel] == 1) {
			if (var481A == 1)
				prepareSwapVars(channel);
			else if (channel < 3)
				clearSIDWaveform(channel);
		} else if (channel < 3 && bgSoundActive && swapPrepared &&
		    !(filterSwapped && filterUsed))
		{
			busyChannelBits |= BITMASK[channel];
			useSwapVars(channel);
			waveCtrlReg[channel] |= 0x01;
			setSIDWaveCtrlReg(channel);

			//safeUnlockResource(channelMap[channel]);
			return;
		}

		chanPrio[channel] = 0;
		usedChannelBits &= BITMASK_INV[channel];
		countFreeChannels();
	}

	int8_t resIndex = channelMap[channel];
	channelMap[channel] = 0;
	
  //debug_out("s4f45 %d %d", channel, resIndex);
  
  //safeUnlockResource(resIndex);
}

// chanResIndex: 3,4,5 or 58
/*void safeUnlockResource(int8_t resIndex) { // $4FEA
	if (!isMusicPlaying) {
		unlockResource(resIndex);
	}
}*/

void releaseResource(int8_t resIndex, uint8_t flags) { // $5031
  releaseResChannels(resIndex);
	if (resIndex == bgSoundResID && var481A == -1) {
		//safeUnlockResource(resIndex);

		bgSoundResID = 0;
		bgSoundActive = 0;
		swapPrepared = 0;
		pulseWidthSwapped = 0;

		resetSwapVars();
	}

  if (flags) {
	  res_unlock(RES_TYPE_C64SOUND, resIndex, 0);
    res_deactivate(RES_TYPE_C64SOUND, resIndex, 0);
  }
}

void releaseResChannels(int8_t resIndex) { // $5070
	for (int8_t i = 3; i >= 0; --i) {
		if (resIndex == channelMap[i]) {
			releaseChannel(i);
		}
	}
}

void stopSound_intern(int8_t soundResID, uint8_t flags) { // $5093
	for (uint8_t i = 0; i < 7; ++i) {
		if (soundResID == _soundQueue[i]) {
			_soundQueue[i] = -1;
		}
	}
	var481A = -1;
	releaseResource(soundResID, flags);
}

void stopMusic_intern() { // $4CAA
	statusBits1B = 0;
	isMusicPlaying = 0;

	//if (resID_song != 0) {
		//unlockResource(resID_song);
	//}

	chanPrio[0] = 2;
	chanPrio[1] = 2;
	chanPrio[2] = 2;

	statusBits1A = 0;
	phaseBit[0] = 0;
	phaseBit[1] = 0;
	phaseBit[2] = 0;
}

void releaseResourceUnk(int8_t resIndex) { // $50A4
	var481A = -1;
	releaseResource(resIndex, 1);
}

// a: 0..6
void releaseChannel(uint8_t channel) {
	stopChannel(channel);
	if (channel >= 4) {
		return;
	}
	if (channel < 3) {
		SIDReg23Stuff = SIDReg23;
		clearSIDWaveform(channel);
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
			stopChannel(3);
		}
	}

	stopChannel(channel + 4);
}

void clearSIDWaveform(uint8_t channel) {
	if (!isMusicPlaying && var481A == -1) {
		waveCtrlReg[channel] &= 0x0e;
		setSIDWaveCtrlReg(channel);
	}
}

void stopChannel(uint8_t channel) {
	songPosUpdateCounter[channel] = 0;
	// clear "channel" bit
	busyChannelBits &= BITMASK_INV[channel];
	if (channel >= 4) {
		// pulsewidth = 0
		channelMap[channel] = 0;
	}
}

// channel: 0..6, swapIndex: 0..2
void swapVars(uint8_t channel, int8_t swapIndex) { // $51a5
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

void resetSwapVars() { // $52d0
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

void prepareSwapVars(uint8_t channel) { // $52E5
	if (channel >= 4)
		return;

	if (channel < 3) {
		if (!keepSwapVars) {
			resetSwapVars();
		}
		swapVars(channel, 0);
		if (busyChannelBits & BITMASK[channel+4]) {
			swapVars(channel+4, 1);
			pulseWidthSwapped = 1;
		}
	} else if (channel == 3) {
		SIDReg24_HiNibble = SIDReg24 & 0x70;
		resetSwapVars();
		keepSwapVars = 1;
		swapVars(3, 2);
		filterSwapped = 1;
	}
	swapPrepared = 1;
}

void useSwapVars(uint8_t channel) { // $5342
	if (channel >= 3)
		return;

	swapVars(channel, 0);
	setSIDFreqAS(channel);
	if (pulseWidthSwapped) {
		swapVars(channel+4, 1);
		setSIDFreqAS(channel+4);
	}
	if (filterSwapped) {
		swapVars(3, 2);

		// resonating filter freq. or voice-to-filter mapping?
		SIDReg23 = (SIDReg23Stuff & 0xf0) | BITMASK[channel];
		SID_Write(23, SIDReg23);

		// filter props
		SIDReg24 = (SIDReg24 & 0x0f) | SIDReg24_HiNibble;
		SID_Write(24, SIDReg24);

		// filter freq.
		SID_Write(21, LOBYTE_(freqReg[3]));
		SID_Write(22, HIBYTE_(freqReg[3]));
	} else {
		SIDReg23 = SIDReg23Stuff & BITMASK_INV[channel];
		SID_Write(23, SIDReg23);
	}

	swapPrepared = 0;
	pulseWidthSwapped = 0;
	keepSwapVars = 0;
	SIDReg24_HiNibble = 0;
	filterSwapped = 0;
}

// ignore: no effect
// resIndex: 3,4,5 or 58
/*void lockResource(int8_t resIndex) { // $4ff4
	if (!isMusicPlaying)
		++resStatus[resIndex];
}*/

void reserveChannel(uint8_t channel, uint8_t prioValue, int8_t chanResIndex) { // $4ffe
	if (channel == 3) {
		filterUsed = 1;
	} else if (channel < 3) {
		usedChannelBits |= BITMASK[channel];
		countFreeChannels();
	}

	chanPrio[channel] = prioValue;
	//lockResource(chanResIndex);
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

void initMusic(int8_t songResIndex, uint8_t __far *data) { // $7de6
	//unlockResource(resID_song);

	resID_song = songResIndex;
	//_music = getResource(resID_song);
  _music = data;
	if (_music == NULL) {

    //while(1) {
      //*(volatile uint8_t *)0xd020 = *(volatile uint8_t *)0xd020 + 1;
    //}

		return;
	}

	// song base address
	uint8_t __far *songFileDataPtr = _music;
	//actSongFileData = _music;

	initializing = 1;
	_soundInQueue = 0;
	isMusicPlaying = 0;

	//unlockCodeLocation();
	resetPlayerState();

	//lockResource(resID_song);
	buildStepTbl(songFileDataPtr[5]);

  // fetch sound
	songChannelBits = songFileDataPtr[4];
	for (int8_t i = 2; i >= 0; --i) {
    if ((songChannelBits & BITMASK[i]) != 0) {
			func_7eae(i, songFileDataPtr);
		}
	}

	isMusicPlaying = 1;
	//lockCodeLocation();

  SIDReg23 &= 0xf0;
	SID_Write(23, SIDReg23);

	handleMusicBuffer();

	initializing = 0;
	_soundInQueue = 1;
}

// params:
//   channel: channel 0..2
void func_7eae(uint8_t channel, uint8_t __far *songFileDataPtr) {
	uint8_t pos = SONG_CHANNEL_OFFSET[channel];
	chanDataOffset[channel] = READ_LE_UINT16(&songFileDataPtr[pos]);
	chanFileData[channel] = songFileDataPtr + chanDataOffset[channel];

	//vec5[channel+4] = vec5[channel] = CHANNEL_BUFFER_ADDR[RES_ID_CHANNEL[channel]]; // not used
	vec6[channel+4] = 0x0019;
	vec6[channel]   = 0x0008;

	func_819b(channel);

	waveCtrlReg[channel] = 0;
}

void func_819b(uint8_t channel) {
	reserveChannel(channel, 127, RES_ID_CHANNEL[channel]);

	statusBits1B |= BITMASK[channel];
	statusBits1A |= BITMASK[channel];
}

void buildStepTbl(uint8_t step) { // $82B4
	stepTbl[0] = 0;
	stepTbl[1] = step - 2;
	for (uint8_t i = 2; i < 33; ++i) {
		stepTbl[i] = stepTbl[i-1] + step;
	}
}

uint8_t reserveSoundFilter(uint8_t value, uint8_t chanResIndex) { // $4ED0
	uint8_t channel = 3;
	reserveChannel(channel, value, chanResIndex);
	return channel;
}

uint8_t reserveSoundVoice(uint8_t value, uint8_t chanResIndex) { // $4EB8
	for (int8_t i = 2; i >= 0; --i) {
		if ((usedChannelBits & BITMASK[i]) == 0) {
			reserveChannel(i, value, chanResIndex);
			return i;
		}
	}
	return 0;
}

void findLessPrioChannels(uint8_t soundPrio) { // $4ED8
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

void releaseResourceBySound(int8_t resID) { // $5088
	var481A = 1;
	releaseResource(resID, 1);
}

void readVec6Data(int8_t x, int8_t *offset, uint8_t __far *songFilePtr, int8_t chanResID) { // $4E99
	//vec5[x] = songFilePtr;
	vec6[x] = songFilePtr[*offset];
	*offset += 2;
	_soundQueue[x] = chanResID;
}

int8_t initSound(int8_t soundResID, uint8_t __far *data) { // $4D0A
	initializing = 1;

	if (isMusicPlaying && (statusBits1A & 0x07) == 0x07) {
		initializing = 0;
		return -2;
	}

	//uint8_t __far *songFilePtr = getResource(soundResID);
	uint8_t __far *songFilePtr = data;

	if (songFilePtr == NULL) {
		initializing = 0;
		return 1;
	}

	uint8_t soundPrio = songFilePtr[4];
	// for (mostly but not always looped) background sounds
	if (soundPrio == 1) {
		bgSoundResID = soundResID;
		bgSoundActive = 1;
	}

	uint8_t requestedChannels = 0;
	if ((songFilePtr[5] & 0x40) == 0) {
		++requestedChannels;
		if (songFilePtr[5] & 0x02)
			++requestedChannels;
		if (songFilePtr[5] & 0x08)
			++requestedChannels;
	}

	uint8_t filterNeeded = (songFilePtr[5] & 0x20) != 0;
	uint8_t filterBlocked = (filterUsed && filterNeeded);
	if (filterBlocked || (freeChannelCount < requestedChannels)) {
		findLessPrioChannels(soundPrio);

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
			releaseResourceBySound(l_resID);
		}

		while ((freeChannelCount < requestedChannels) || (filterNeeded && filterUsed)) {
			findLessPrioChannels(soundPrio);
			if (minChanPrio >= soundPrio) {
				initializing = 0;
				return -1;
			}

			uint8_t l_resID = channelMap[minChanPrioIndex];
			releaseResourceBySound(l_resID);
		}
	}

	int8_t x;
	uint8_t soundByte5 = songFilePtr[5];
	if (soundByte5 & 0x40)
		x = reserveSoundFilter(soundPrio, soundResID);
	else
		x = reserveSoundVoice(soundPrio, soundResID);

	uint8_t var4CF3 = x;
	int8_t y = 6;
	if (soundByte5 & 0x01) {
		x += 4;
		readVec6Data(x, &y, songFilePtr, soundResID);
	}
	if (soundByte5 & 0x02) {
		x = reserveSoundVoice(soundPrio, soundResID);
		readVec6Data(x, &y, songFilePtr, soundResID);
	}
	if (soundByte5 & 0x04) {
		x += 4;
		readVec6Data(x, &y, songFilePtr, soundResID);
	}
	if (soundByte5 & 0x08) {
		x = reserveSoundVoice(soundPrio, soundResID);
		readVec6Data(x, &y, songFilePtr, soundResID);
	}
	if (soundByte5 & 0x10) {
		x += 4;
		readVec6Data(x, &y, songFilePtr, soundResID);
	}
	if (soundByte5 & 0x20) {
		x = reserveSoundFilter(soundPrio, soundResID);
		readVec6Data(x, &y, songFilePtr, soundResID);
	}

	//vec5[var4CF3] = songFilePtr;
	vec6[var4CF3] = y;
	_soundQueue[var4CF3] = soundResID;

  chanloop[var4CF3] = 0;


	initializing = 0;
	_soundInQueue = 1;

	return soundResID;
}

void unused1() { // $50AF
	var481A = -1;
	if (bgSoundResID != 0) {
		releaseResourceUnk(bgSoundResID);
	}
}

uint8_t __far *getResource(int8_t resID) {
	//switch (resID) {
	//case 0:
  if (resID == 0) {
		return NULL;
  } else if (resID > 0 && resID < 3) {
    while(1) {
      //*(volatile uint8_t *)0xd021 = 2;
      *(volatile uint8_t *)0xd020 = *(volatile uint8_t *)0xd020 + 1;
    }

  } else if (resID > 2 && resID < 6) {
  //case 3:
	//case 4:
	//case 5:

    //while(1) {
      //*(volatile uint8_t *)0xd020 = *(volatile uint8_t *)0xd020 + 1;
    //}

	  return 	(uint8_t __far *)chanBuffer[resID-3];
  } else {
	//default: {
//	  return vm_getResourceAddress(rtSound, resID);
      //uint8_t res_page = res_provide(RES_TYPE_C64SOUND, (uint8_t)resID, 0);
      //res_activate_slot(res_page);
      //res_lock(RES_TYPE_C64SOUND, (uint8_t)resID, 0);
      return resourceData;
    //}
	//}
  }
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

void SID_Write(uint8_t reg, uint8_t data) {
	//_sid->write(reg, data);
  *(volatile uint8_t *)(0xd400 + reg) = data;
  *(volatile uint8_t *)(0xd440 + reg) = data;
}


void startSound(int8_t nr) {
	//uint8_t __far *data = vm_getResourceAddress(rtSound, nr);
    uint8_t res_page = res_provide(RES_TYPE_C64SOUND, nr, 0);
    res_activate_slot(res_page);
    res_lock(RES_TYPE_C64SOUND, nr, 0);

    debug_out("ssnd: %d", nr);

    uint8_t __far *data = (uint8_t __far *)res_get_huge_ptr(res_page);
    resourceData = data;

    //uint8_t __far *data = getResource(nr);

	// WORKAROUND:
	// sound[4] contains either a song prio or a music channel usage byte.
	// As music channel usage is always 0x07 for all music files and
	// prio 7 is never used in any sound file use this byte for auto-detection.
	uint8_t isMusic = (data[4] == 0x07);

	//Common::StackLock lock(_mutex);

  __asm volatile (
    "   sei "
    :::
  );

	if (isMusic) {
		initMusic(nr, data);
	} else {
		stopSound_intern(nr, 0);
		initSound(nr, data);
	}

  __asm volatile (
    "   cli "
    :::
  );
}

void stopSound(int8_t nr) {
  debug_out("sstp: %d", nr);

  if (nr == -1) {
		return;
  }

	//Common::StackLock lock(_mutex);
	stopSound_intern(nr, 1);
	//releaseResource(nr, 1);
}

int8_t getSoundStatus(int8_t nr) {
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

//int8_t getMusicTimer() {
	/*int8_t result = _music_timer;
	_music_timer = 0;
	
  return result;*/
  
  //return 0;
//}
