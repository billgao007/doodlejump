#include "core.h"
#include "data.h"
#include "utils.h"
#include "input.h"
#include "logic.h"
#include "render.h"
#include "audio.h"
#include "rhythm.h"
#include <graphics.h>
#include <time.h>

// 实例化全局数据
GameData g_game;

void SystemInit() {
    // 基础数据初始化
    g_game.state = STATE_AUTH;
    g_game.current_user.username[0] = '\0';
    g_game.current_user.password[0] = '\0';
    g_game.current_user.max_score = 0;
    g_game.mode = MODE_NORMAL;
    g_game.score = 0;
    g_game.is_running = 1;

    InitRandom();
    InitRender();
    InitLogic();
    InitAudio();
    InitRhythm();
    
    BeginBatchDraw(); 
}

void SystemRun() {
    // 控制帧率的变量
    long long start_time, frame_time;
    const long long delay_per_frame = 1000 / FPS;

    while (g_game.is_running) {
        start_time = GetGameTimeMs();

        
        ProcessInput();

        // 节奏模式的逻辑更新
        if (g_game.state == STATE_RHYTHM) {
            UpdateRhythm(GetGameTimeMs());
        } else {
            UpdateLogic();
        }

        
        RenderFrame();

        // 帧率控制
        frame_time = GetGameTimeMs() - start_time;
        if (frame_time < delay_per_frame) {
            Sleep((DWORD)(delay_per_frame - frame_time));
        }
    }

    EndBatchDraw();
    CloseRender();
    CloseAudio();
}
