#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#include <tchar.h>

typedef enum {
    SOUND_PERFECT,
    SOUND_GOOD,
    SOUND_MISS,
    SOUND_BAR_START
} SoundType;

void InitAudio();

void CloseAudio();

void PlayBackgroundMusic(const TCHAR* filepath, int bpm);

void StopBackgroundMusic();

void PlayEffect(SoundType type);

long long GetGameTimeMs();

void ResetAudioTiming();
