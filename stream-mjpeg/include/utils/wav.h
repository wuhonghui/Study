#ifndef wav_H
#define wav_H

#ifdef __cplusplus
extern "C" {
#endif
#include <inttypes.h>

/* wav heade and help funcs */
/* ---------------------------------------------------------------------- */
/* *.wav I/O stolen from cdda2wav */

/* Copyright (C) by Heiko Eissfeldt */

typedef uint8_t   BYTE;
typedef uint16_t  WORD;
typedef uint32_t  DWORD;
typedef uint32_t  FOURCC;	/* a four character code */

/* flags for 'wFormatTag' field of WAVEFORMAT */
#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_ALAW 6
#define WAVE_FORMAT_MULAW 7

/* MMIO macros */
#define mmioFOURCC(ch0, ch1, ch2, ch3) \
  ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
  ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))

#define FOURCC_RIFF	mmioFOURCC ('R', 'I', 'F', 'F')
#define FOURCC_LIST	mmioFOURCC ('L', 'I', 'S', 'T')
#define FOURCC_WAVE	mmioFOURCC ('W', 'A', 'V', 'E')
#define FOURCC_FMT	mmioFOURCC ('f', 'm', 't', ' ')
#define FOURCC_DATA	mmioFOURCC ('d', 'a', 't', 'a')
#define FOURCC_FACT mmioFOURCC ('f', 'a', 'c', 't')

struct wav_pcm_header {
	uint32_t riffChkID;
	uint32_t riffChkSize;
	uint32_t waveChkID;
	uint32_t fmtChkID;
	uint32_t fmtChkSize;
	uint16_t wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec;
	uint32_t nAvgBytesPerSec;
	uint16_t nBlockAlign;
	uint16_t wBitsPerSample;
	uint32_t dataChkID;
	uint32_t dataChkSize;
}__attribute__((packed));

struct wav_nonpcm_header {
	uint32_t riffChkID;
	uint32_t riffChkSize;
	uint32_t waveChkID;
	uint32_t fmtChkID;
	uint32_t fmtChkSize;
	uint16_t wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec;
	uint32_t nAvgBytesPerSec;
	uint16_t nBlockAlign;
	uint16_t wBitsPerSample;
	uint16_t cbSize;
	uint32_t factChkID;
	uint32_t factChksize;
	uint32_t dwSampleLength;
	uint32_t dataChkID;
	uint32_t dataChkSize;
}__attribute__((packed));

/* -------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif


#endif

