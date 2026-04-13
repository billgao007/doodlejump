#include "core.h"
#include "data.h"
#include "utils.h"
#include "input.h"
#include "logic.h"
#include "render.h"
#include <graphics.h>
#include <time.h>

// 实例化全局数据
GameData g_game;

void SystemInit() {
    // 基础数据初始化
    g_game.state = STATE_MENU;
    g_game.score = 0;
    g_game.is_running = 1;

    InitRandom();
    InitRender();
    InitLogic();
    
    BeginBatchDraw(); // 开启双缓冲，防止闪烁
}

void SystemRun() {
    // 控制帧率的变量
    DWORD start_time, frame_time;
    const DWORD delay_per_frame = 1000 / FPS;

    while (g_game.is_running) {
        start_time = GetTickCount();

        // 1. 获取输入
        ProcessInput();

        // 2. 逻辑更新
        UpdateLogic();

        // 3. 渲染绘制
        RenderFrame();

        // 4. 帧率控制
        frame_time = GetTickCount() - start_time;
        if (frame_time < delay_per_frame) {
            Sleep(delay_per_frame - frame_time);
        }
    }

    EndBatchDraw();
    CloseRender();
}
