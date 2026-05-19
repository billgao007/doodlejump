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

void ResetAudioTiming() {
    g_bar_start_time = GetGameTimeMs();
}
