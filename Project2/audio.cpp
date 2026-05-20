#include "audio.h"
#pragma comment(lib, "winmm.lib")

static long long g_bar_start_time = 0;

void InitAudio() {
    g_bar_start_time = GetGameTimeMs();
}

void CloseAudio() {
    StopBackgroundMusic();
}

void PlayBackgroundMusic(const TCHAR* filepath, int bpm) {
    // 使用 mciSendString 播放背景音乐（循环）
    TCHAR cmd[512];
    _stprintf_s(cmd, 512, _T("open %s alias rhythm_music"), filepath);
    mciSendString(cmd, NULL, 0, 0);
    
    // 设置循环播放
    mciSendString(_T("play rhythm_music repeat"), NULL, 0, 0);
}

void StopBackgroundMusic() {
    mciSendString(_T("stop rhythm_music"), NULL, 0, 0);
    mciSendString(_T("close rhythm_music"), NULL, 0, 0);
}

void PlayEffect(SoundType type) {
    // 使用系统音效的非阻塞提示，避免影响主逻辑更新速度
    switch (type) {
    case SOUND_PERFECT:
        MessageBeep(MB_ICONASTERISK);
        break;
    case SOUND_GOOD:
        MessageBeep(MB_OK);
        break;
    case SOUND_MISS:
        MessageBeep(MB_ICONHAND);
        break;
    case SOUND_BAR_START:
        MessageBeep(MB_ICONEXCLAMATION);
        break;
    }
}

void PlayNoteBeep(int pitch_index) {
    // C4 大调音阶: do re mi fa sol la xi do (C4~C5, 8 个音)
    static const int note_freqs[8] = {
        262, // C4  do
        294, // D4  re
        330, // E4  mi
        349, // F4  fa
        392, // G4  sol
        440, // A4  la
        494, // B4  xi
        523  // C5  do(高八度)
    };
    if (pitch_index < 0) pitch_index = 0;
    if (pitch_index > 7) pitch_index = pitch_index % 8;
    // Beep 可能在部分系统上被禁用；MessageBeep 作为保底
    if (!Beep(note_freqs[pitch_index], 80)) {
        MessageBeep(MB_OK);
    }
}

void ResetAudioTiming() {
    g_bar_start_time = GetGameTimeMs();
}
