#include "render.h"
#include "data.h"
#include <graphics.h>
#include <tchar.h>
#include <stdio.h>

static IMAGE img_player;
static IMAGE img_platform;

void InitRender() {
    initgraph(SCREEN_WIDTH, SCREEN_HEIGHT);
    setbkcolor(RGB(240, 248, 255));
    loadimage(&img_player, _T("player.png"), 30, 30);
    loadimage(&img_platform, _T("platform.png"), 60, 10);
}

void CloseRender() { closegraph(); }

// 补回了这两个缺失的函数
static void DrawAuth() {
    settextcolor(BLACK);
    settextstyle(40, 0, _T("Consolas"));
    outtextxy(60, 150, _T("Doodle Jump"));

    settextstyle(20, 0, _T("Consolas"));
    outtextxy(100, 250, _T("Press [L] to Login"));
    outtextxy(100, 300, _T("Press [R] to Register"));
}

static void DrawMenu() {
    settextcolor(BLACK);
    settextstyle(40, 0, _T("Consolas"));
    outtextxy(60, 150, _T("Doodle Jump"));

    TCHAR welcomeStr[64];
    _stprintf_s(welcomeStr, 64, _T("Welcome, %s!"), g_game.current_user.username);

    settextstyle(20, 0, _T("Consolas"));
    outtextxy(80, 230, welcomeStr);
    outtextxy(100, 300, _T("Press SPACE to Start"));
}

static void DrawGame() {
    // 1. 画平台
    for (int i = 0; i < PLATFORM_COUNT; i++) {
        Platform* p = &g_game.platforms[i];
        if (p->type == PLAT_NORMAL) {
            putimage((int)p->x, (int)p->y, &img_platform);
        }
        else {
            if (p->type == PLAT_FAKE) setfillcolor(RGB(100, 100, 100));
            else if (p->type == PLAT_SPRING) setfillcolor(RGB(255, 0, 255));
            solidrectangle((int)p->x, (int)p->y, (int)(p->x + p->width), (int)(p->y + p->height));
        }
    }

    // 2. 画增益道具 Buff
    for (int i = 0; i < MAX_BUFFS; i++) {
        if (g_game.buffs[i].active) {
            Buff* b = &g_game.buffs[i];
            if (b->type == BUFF_FIRE_RATE) setfillcolor(RGB(255, 255, 0));
            else if (b->type == BUFF_DMG_ADD) setfillcolor(RGB(255, 165, 0));
            else if (b->type == BUFF_DMG_MULT) setfillcolor(RGB(255, 0, 0));
            else if (b->type == BUFF_TIME) setfillcolor(RGB(0, 255, 255));
            solidcircle((int)b->x, (int)b->y, (int)b->radius);
        }
    }

    // 3. 画玩家与子弹
    putimage((int)(g_game.player.x - g_game.player.radius),
        (int)(g_game.player.y - g_game.player.radius), &img_player);

    setfillcolor(RGB(0, 0, 0));
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (g_game.bullets[i].active) {
            solidcircle((int)g_game.bullets[i].x, (int)g_game.bullets[i].y, 3);
        }
    }

    // 4. 画 Boss & 技能
    Boss* b = &g_game.boss;
    setfillcolor(b->phase == 1 ? RGB(139, 0, 0) : RGB(75, 0, 130));
    solidrectangle((int)b->x, (int)b->y, (int)(b->x + b->width), (int)(b->y + b->height));

    if (b->laser_warning_time > 0) {
        setlinecolor(RGB(255, 150, 150));
        setlinestyle(PS_DASH, 2);
        line((int)b->laser_x, (int)(b->y + b->height), (int)b->laser_x, SCREEN_HEIGHT);
        setlinestyle(PS_SOLID, 1);
    }
    else if (b->laser_active_time > 0) {
        setlinecolor(RGB(255, 0, 0));
        setlinestyle(PS_SOLID, 15);
        line((int)b->laser_x, (int)(b->y + b->height), (int)b->laser_x, SCREEN_HEIGHT);
        setlinestyle(PS_SOLID, 1);
    }

    // 5. 画 UI
    TCHAR textBuf[64];
    settextcolor(BLACK); settextstyle(16, 0, _T("Consolas"));

    setfillcolor(RED);
    solidrectangle(100, 5, 100 + (int)(200 * ((float)b->hp / b->max_hp)), 15);
    outtextxy(10, 2, _T("BOSS:"));

    setfillcolor(GREEN);
    solidrectangle(10, SCREEN_HEIGHT - 20, 10 + (int)(100 * ((float)g_game.player.hp / g_game.player.max_hp)), SCREEN_HEIGHT - 5);

    _stprintf_s(textBuf, 64, _T("Dmg: %d x%.1f | Time Items: %d"),
        g_game.player.base_damage, g_game.player.dmg_mult, g_game.player.special_buffs);
    outtextxy(120, SCREEN_HEIGHT - 20, textBuf);

    if (g_game.gravity_dir == -1) {
        settextcolor(RED); settextstyle(30, 0, _T("Consolas"));
        outtextxy(70, 300, _T("GRAVITY REVERSED!"));
    }
    if (g_game.time_scale < 1.0f) {
        settextcolor(RGB(0, 255, 255)); settextstyle(30, 0, _T("Consolas"));
        outtextxy(100, 350, _T("TIME SLOWED"));
    }
}

static void DrawGameOver() {
    settextcolor(RED); settextstyle(40, 0, _T("Consolas"));
    if (g_game.boss.hp <= 0) outtextxy(60, 200, _T("BOSS DEFEATED!"));
    else outtextxy(100, 200, _T("GAME OVER"));

    settextcolor(BLACK); settextstyle(20, 0, _T("Consolas"));
    TCHAR str[64]; _stprintf_s(str, 64, _T("Score: %d  Max: %d"), g_game.score, g_game.current_user.max_score);
    outtextxy(80, 260, str);
    outtextxy(80, 320, _T("Press SPACE to Restart"));
}

void RenderFrame() {
    cleardevice();
    switch (g_game.state) {
    case STATE_AUTH: DrawAuth(); break;
    case STATE_MENU: DrawMenu(); break;
    case STATE_PLAYING: DrawGame(); break;
    case STATE_GAMEOVER: DrawGameOver(); break;
    }
    FlushBatchDraw();
}