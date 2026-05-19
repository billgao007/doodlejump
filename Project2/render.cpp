#include "render.h"
#include "data.h"
#include "rhythm.h"
#include <graphics.h>
#include <tchar.h>
#include <stdio.h>
#include <math.h>

static IMAGE img_player;
static IMAGE img_platform;
static IMAGE img_platform_fake;
static IMAGE img_platform_spring;
static IMAGE img_buff_dmgadd;
static IMAGE img_buff_dmgdouble;
static IMAGE img_buff_timeslow;
static IMAGE img_boss;

static int ClampColor(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return value;
}

static void DrawLaserBeam(const Boss* b) {
    int x = (int)b->laser_x;
    int y = (int)(b->y + b->height);
    int bottom = SCREEN_HEIGHT;
    long long tick = GetGameTimeMs();

    if (b->laser_warning_time > 0) {
        int pulse = (tick / 90) % 4;
        int offsets[] = { -7, -4, -2, 0, 2, 4, 7 };
        COLORREF colors[] = {
            RGB(255, 220, 220),
            RGB(255, 180, 180),
            RGB(255, 120, 150),
            RGB(255, 90, 120),
            RGB(255, 120, 150),
            RGB(255, 180, 180),
            RGB(255, 220, 220)
        };

        for (int i = 0; i < 7; i++) {
            setlinecolor(colors[i]);
            setlinestyle(PS_SOLID, 1 + (pulse % 2));
            line(x + offsets[i], y, x + offsets[i], bottom);
        }

        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_DASH, 2);
        line(x, y, x, bottom);

        setlinecolor(RGB(255, 120, 120));
        setlinestyle(PS_SOLID, 1);
        solidcircle(x, y, 6 + pulse);
        solidcircle(x, bottom - 2, 5 + pulse);
    }
    else if (b->laser_active_time > 0) {
        int pulse = (tick / 40) % 6;
        int offsets[] = { -10, -7, -4, -2, 0, 2, 4, 7, 10 };
        COLORREF outer = RGB(255, 60 + pulse * 5, 60 + pulse * 3);
        COLORREF inner = RGB(255, 220, 120 + pulse * 2);

        for (int i = 0; i < 9; i++) {
            setlinecolor(outer);
            setlinestyle(PS_SOLID, 4 - (i > 5));
            line(x + offsets[i], y, x + offsets[i], bottom);
        }

        setlinecolor(inner);
        setlinestyle(PS_SOLID, 11 + (pulse % 2));
        line(x, y, x, bottom);

        setlinecolor(RGB(255, 255, 255));
        setlinestyle(PS_SOLID, 2);
        line(x - 1, y, x - 1, bottom);
        line(x + 1, y, x + 1, bottom);

        setlinecolor(RGB(255, 180, 180));
        solidcircle(x, y, 10 + pulse);
        solidcircle(x, bottom - 4, 9 + pulse);

        for (int i = 0; i < 4; i++) {
            int flare_x = x + (i - 1) * 5;
            int flare_y = bottom - 22 - (i % 2) * 6;
            setlinecolor(RGB(255, 80, 80));
            line(flare_x - 3, flare_y, flare_x + 3, flare_y);
            line(flare_x, flare_y - 3, flare_x, flare_y + 3);
        }
    }
}

void InitRender() {
    initgraph(SCREEN_WIDTH, SCREEN_HEIGHT);
    setbkcolor(RGB(240, 248, 255));
    loadimage(&img_player, _T("player.png"), 30, 30);
    loadimage(&img_platform, _T("platform.png"), 60, 10);
    loadimage(&img_platform_fake, _T("fakeplatform.png"), 60, 10);
    loadimage(&img_platform_spring, _T("springplatform.png"), 60, 10);
    loadimage(&img_buff_dmgadd, _T("dmgadd.png"), 24, 24);
    loadimage(&img_buff_dmgdouble, _T("dmgdouble.png"), 24, 24);
    loadimage(&img_buff_timeslow, _T("timeslow.png"), 24, 24);
    loadimage(&img_boss, _T("boss.png"), 80, 40);
}

void CloseRender() { closegraph(); }


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

    // 模式选择显示
    settextstyle(18, 0, _T("Consolas"));
    outtextxy(100, 330, _T("Mode:"));

    // 若计时器激活，绘制高亮背景
    if (g_game.mode_flash_timer > 0) {
        setfillcolor(RGB(255, 230, 230));
        solidrectangle(144, 324, 340, 354);
    }

    if (g_game.mode == MODE_NORMAL) {
        settextcolor(RGB(255, 0, 0));
        outtextxy(150, 330, _T("[Normal]  Rhythm"));
    } else {
        settextcolor(RGB(255, 0, 0));
        outtextxy(150, 330, _T("Normal  [Rhythm]"));
    }
    settextcolor(BLACK);

    // 在渲染处递减计时器（简单处理）
    if (g_game.mode_flash_timer > 0) g_game.mode_flash_timer--; 
}

static void DrawGame() {
    // 1. 画平台
    for (int i = 0; i < PLATFORM_COUNT; i++) {
        Platform* p = &g_game.platforms[i];
        if (p->type == PLAT_NORMAL) {
            putimage((int)p->x, (int)p->y, &img_platform);
        }
        else if (p->type == PLAT_FAKE) {
            putimage((int)p->x, (int)p->y, &img_platform_fake);
        }
        else if (p->type == PLAT_SPRING) {
            putimage((int)p->x, (int)p->y, &img_platform_spring);
        }
    }

    // 2. 画增益道具 Buff
    for (int i = 0; i < MAX_BUFFS; i++) {
        if (g_game.buffs[i].active) {
            Buff* b = &g_game.buffs[i];
            if (b->type == BUFF_DMG_ADD) {
                putimage((int)(b->x - b->radius), (int)(b->y - b->radius), &img_buff_dmgadd);
            }
            else if (b->type == BUFF_DMG_MULT) {
                putimage((int)(b->x - b->radius), (int)(b->y - b->radius), &img_buff_dmgdouble);
            }
            else if (b->type == BUFF_TIME) {
                putimage((int)(b->x - b->radius), (int)(b->y - b->radius), &img_buff_timeslow);
            }
            else {
                setfillcolor(RGB(255, 255, 0));
                solidcircle((int)b->x, (int)b->y, (int)b->radius);
            }
        }
    }

    // 3. 画玩家与子弹
    putimage((int)(g_game.player.x - g_game.player.radius),
        (int)(g_game.player.y - g_game.player.radius), &img_player);

    setfillcolor(RGB(0, 0, 0));
    for (BulletNode* node = g_game.bullet_active_head; node; node = node->next) {
        int damage = node->bullet.damage;
        int red = ClampColor(120 + (damage - 10) * 10);
        int green = ClampColor(90 - (damage - 10) * 4);
        int blue = ClampColor(90 - (damage - 10) * 4);
        setfillcolor(RGB(red, green, blue));
        solidcircle((int)node->bullet.x, (int)node->bullet.y, 3);
    }

    // 3.5 画 Boss 被击中的粒子特效
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle* particle = &g_game.particles[i];
        if (particle->life > 0) {
            int alpha = 255 * particle->life / particle->max_life;
            int red = ClampColor(particle->r * alpha / 255);
            int green = ClampColor(particle->g * alpha / 255);
            int blue = ClampColor(particle->b * alpha / 255);
            setfillcolor(RGB(red, green, blue));
            solidcircle((int)particle->x, (int)particle->y, particle->radius);
        }
    }

    // 4. 画 Boss & 技能
    Boss* b = &g_game.boss;
    putimage((int)b->x, (int)b->y, &img_boss);
    DrawLaserBeam(b);

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

    if (g_game.buff_hint_timer > 0 && g_game.buff_hint[0] != '\0') {
        settextstyle(20, 0, _T("Consolas"));
        settextcolor(BLACK);
        outtextxy(81, SCREEN_HEIGHT - 58, g_game.buff_hint);
        settextcolor(RED);
        outtextxy(80, SCREEN_HEIGHT - 60, g_game.buff_hint);
    }

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

// ========== 节奏模式：Spirit 角色绘制 ==========
static void DrawRhythmSpirit(float cx, float cy, long long current_time) {
    int x = (int)cx, y = (int)cy;
    int body_r = 18;
    int eye_r = 4;

    // 身体阴影
    setfillcolor(RGB(200, 100, 20));
    solidcircle(x + 2, y + 2, body_r);

    // 身体主体（橙色小怪兽）
    setfillcolor(RGB(255, 140, 30));
    solidcircle(x, y, body_r);

    // 高光
    setfillcolor(RGB(255, 180, 80));
    solidcircle(x - 5, y - 7, 7);

    // 眼睛（白色 + 黑色瞳孔）
    setfillcolor(WHITE);
    solidcircle(x - 6, y - 4, eye_r + 1);
    solidcircle(x + 6, y - 4, eye_r + 1);

    // 瞳孔微小动画（跟随 BPM 脉冲）
    float pulse = g_game.rhythm_data.bg_pulse_phase;
    int pupil_offset = (int)(sinf(pulse * 6.28318f) * 1.5f);
    setfillcolor(BLACK);
    solidcircle(x - 6 + pupil_offset, y - 4, eye_r - 1);
    solidcircle(x + 6 + pupil_offset, y - 4, eye_r - 1);

    // 微笑嘴巴
    setlinecolor(BLACK);
    setlinestyle(PS_SOLID, 2);
    arc(x - 6, y + 2, x + 6, y + 10, 0.0f, 3.14159f);

    // 小触角/天线
    setlinecolor(RGB(255, 140, 30));
    setlinestyle(PS_SOLID, 3);
    line(x - 2, y - body_r, x - 4, y - body_r - 10);
    line(x + 2, y - body_r, x + 4, y - body_r - 10);
    setfillcolor(RGB(255, 220, 50));
    solidcircle(x - 4, y - body_r - 10, 3);
    solidcircle(x + 4, y - body_r - 10, 3);
}

// ========== 节奏模式：音符绘制（菱形 + 光晕）==========
static void DrawRhythmNote(int cx, int cy, TrackID track, JudgmentType judgment, int hit_flash) {
    int size = 16;

    // 命中闪烁效果
    if (hit_flash > 0) {
        float flash_t = (float)hit_flash / HIT_FLASH_FRAMES;
        int flash_r = size + (int)(12 * flash_t);
        int alpha = (int)(180 * flash_t);
        setfillcolor(RGB(alpha, alpha, 255));
        solidcircle(cx, cy, flash_r);
    }

    // 外发光
    COLORREF glow_color;
    if (judgment == JUDGMENT_PERFECT)      glow_color = RGB(0, 255, 100);
    else if (judgment == JUDGMENT_GOOD)    glow_color = RGB(255, 220, 50);
    else if (judgment == JUDGMENT_MISS)    glow_color = RGB(255, 50, 50);
    else {
        // 未判定音符：根据轨道着色
        glow_color = (track == TRACK_LEFT) ? RGB(100, 180, 255) : RGB(255, 130, 200);
    }

    // 光晕
    setfillcolor(glow_color);
    solidcircle(cx, cy, size + 6);
    // 内圈亮色
    int r = GetRValue(glow_color), g = GetGValue(glow_color), b = GetBValue(glow_color);
    setfillcolor(RGB(min(r + 60, 255), min(g + 60, 255), min(b + 60, 255)));
    solidcircle(cx, cy, size + 2);
    // 白色核心
    setfillcolor(WHITE);
    solidcircle(cx, cy, size - 4);

    // 菱形边框
    POINT diamond[4] = {
        {cx, cy - size},
        {cx + size, cy},
        {cx, cy + size},
        {cx - size, cy}
    };
    setlinecolor(RGB(min(r + 40, 255), min(g + 40, 255), min(b + 40, 255)));
    setlinestyle(PS_SOLID, 2);
    polygon(diamond, 4);

    // 轨道方向指示三角箭头
    setfillcolor(WHITE);
    POINT tri[3];
    if (track == TRACK_LEFT) {
        tri[0].x = cx - 6; tri[0].y = cy;
        tri[1].x = cx + 4; tri[1].y = cy - 5;
        tri[2].x = cx + 4; tri[2].y = cy + 5;
    } else {
        tri[0].x = cx + 6; tri[0].y = cy;
        tri[1].x = cx - 4; tri[1].y = cy - 5;
        tri[2].x = cx - 4; tri[2].y = cy + 5;
    }
    solidpolygon(tri, 3);
}

// ========== 节奏模式：绘制得分弹出文字 ==========
static void DrawScorePopups() {
    int popup_count;
    const ScorePopup* popups = GetScorePopups(&popup_count);

    for (int i = 0; i < popup_count; i++) {
        if (popups[i].life <= 0) continue;

        float t = (float)popups[i].life / popups[i].max_life;
        int alpha = (int)(255 * t);
        int font_size = 18 + (int)(8 * t); // 逐渐缩小

        const TCHAR* text;
        COLORREF color;
        if (popups[i].type == JUDGMENT_PERFECT) {
            text = _T("PERFECT!");
            color = RGB(0, min(alpha + 60, 255), min(alpha, 200));
        } else if (popups[i].type == JUDGMENT_GOOD) {
            text = _T("GOOD");
            color = RGB(min(alpha + 60, 255), min(alpha + 100, 255), 0);
        } else {
            text = _T("MISS");
            color = RGB(min(alpha + 80, 255), min(alpha / 2, 120), min(alpha / 2, 120));
        }

        settextcolor(color);
        settextstyle(font_size, 0, _T("Consolas"));
        int tx = (int)(popups[i].x - textwidth(text) / 2);
        int ty = (int)popups[i].y;
        outtextxy(tx, ty, text);

        // 如果有连击，显示在下方
        if (popups[i].combo > 1 && popups[i].type != JUDGMENT_MISS) {
            TCHAR combo_str[32];
            _stprintf_s(combo_str, 32, _T("%d combo!"), popups[i].combo);
            settextcolor(RGB(min(alpha + 50, 255), min(alpha + 100, 255), min(alpha + 200, 255)));
            settextstyle(font_size - 4, 0, _T("Consolas"));
            int cx = (int)(popups[i].x - textwidth(combo_str) / 2);
            outtextxy(cx, ty + font_size + 2, combo_str);
        }

        settextstyle(18, 0, _T("Consolas")); // reset
    }
}

// ========== 节奏模式：绘制打击粒子 ==========
static void DrawRhythmParticles() {
    int count;
    const RhythmParticle* particles = GetRhythmParticles(&count);

    for (int i = 0; i < count; i++) {
        if (particles[i].life <= 0) continue;
        float t = (float)particles[i].life / particles[i].max_life;
        int alpha = (int)(255 * t);
        setfillcolor(RGB(
            min(particles[i].r * alpha / 255, 255),
            min(particles[i].g * alpha / 255, 255),
            min(particles[i].b * alpha / 255, 255)
        ));
        solidcircle((int)particles[i].x, (int)particles[i].y, (int)particles[i].radius);
    }
}

// ========== 节奏模式：绘制下落音符轨迹 ==========
static void DrawNoteTrails(long long current_time) {
    float fall_speed = FALLING_SPEED * g_game.rhythm_data.fall_speed_mult;

    for (int i = 0; i < g_game.rhythm_data.recorded_count; i++) {
        RhythmNote* note = &g_game.rhythm_data.recorded_sequence[i];
        if (note->is_handled && note->hit_flash_timer <= 0) continue;

        long long note_abs = g_game.rhythm_data.bar_start_time + note->relative_timestamp;
        float note_y = GetNoteYPosition(note_abs, current_time, fall_speed);
        if (note_y < -100 || note_y > SCREEN_HEIGHT + 100) continue;

        int note_x = (note->track == TRACK_LEFT) ? (int)(SCREEN_WIDTH / 4.0f) : (int)(3.0f * SCREEN_WIDTH / 4.0f);

        // 拖尾效果：在音符上方绘制渐隐的虚影
        int trail_len = 8;
        for (int t = 1; t <= trail_len; t++) {
            float trail_y = note_y - t * 6.0f;
            if (trail_y < -20) break;
            int alpha = 60 - t * 7;
            if (alpha < 0) alpha = 0;

            COLORREF tc;
            if (note->track == TRACK_LEFT) tc = RGB(100, 180, 255);
            else tc = RGB(255, 130, 200);

            setfillcolor(RGB(
                min(GetRValue(tc) * alpha / 60, 255),
                min(GetGValue(tc) * alpha / 60, 255),
                min(GetBValue(tc) * alpha / 60, 255)
            ));
            int tr = 14 - t;
            if (tr < 2) tr = 2;
            solidcircle(note_x, (int)trail_y, tr);
        }
    }
}

// ========== 节奏模式：主绘制函数 ==========
static void DrawRhythm() {
    long long current_time = GetGameTimeMs();
    long long elapsed = current_time - g_game.rhythm_data.bar_start_time;
    float fall_speed = FALLING_SPEED * g_game.rhythm_data.fall_speed_mult;
    int judgment_y = JUDGMENT_Y;

    // ---- 1. 背景：深色渐变 + 动态星点 ----
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        int r = 20 + y * 30 / SCREEN_HEIGHT;
        int g = 15 + y * 35 / SCREEN_HEIGHT;
        int b = 50 + y * 40 / SCREEN_HEIGHT;
        setfillcolor(RGB(r, g, b));
        solidrectangle(0, y, SCREEN_WIDTH, y);
    }

    // BPM 脉冲背景光
    float pulse = g_game.rhythm_data.bg_pulse_phase;
    float pulse_intensity = (sinf(pulse * 6.28318f) + 1.0f) * 0.5f; // 0~1
    int pulse_alpha = (int)(40 + pulse_intensity * 20);
    setfillcolor(RGB(pulse_alpha, pulse_alpha / 3, pulse_alpha));
    solidrectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // ---- 2. 左右轨道绘制 ----
    int left_glow = GetTrackGlow(TRACK_LEFT);
    int right_glow = GetTrackGlow(TRACK_RIGHT);

    // 左轨背景
    int lg = 60 + left_glow * 8; if (lg > 160) lg = 160;
    setfillcolor(RGB(30, 40, lg));
    solidrectangle(0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT);
    // 左轨高亮闪烁
    if (left_glow > 0) {
        setfillcolor(RGB(60, 100, min(220, 140 + left_glow * 6)));
        solidrectangle(0, judgment_y - 30, SCREEN_WIDTH / 2, judgment_y + 30);
    }

    // 右轨背景
    int rg = 60 + right_glow * 8; if (rg > 160) rg = 160;
    setfillcolor(RGB(30, rg / 2 + 20, rg));
    solidrectangle(SCREEN_WIDTH / 2, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    // 右轨高亮闪烁
    if (right_glow > 0) {
        setfillcolor(RGB(min(220, 140 + right_glow * 6), 60, 100));
        solidrectangle(SCREEN_WIDTH / 2, judgment_y - 30, SCREEN_WIDTH, judgment_y + 30);
    }

    // 轨道分隔线
    setlinecolor(RGB(100, 100, 180));
    setlinestyle(PS_DASH, 1);
    line(SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT);

    // ---- 3. 音符拖尾 ----
    DrawNoteTrails(current_time);

    // ---- 4. 下落的音符 ----
    for (int i = 0; i < g_game.rhythm_data.recorded_count; i++) {
        RhythmNote* note = &g_game.rhythm_data.recorded_sequence[i];
        long long note_abs = g_game.rhythm_data.bar_start_time + note->relative_timestamp;
        float note_y = GetNoteYPosition(note_abs, current_time, fall_speed);

        if (note_y > -60 && note_y < SCREEN_HEIGHT + 60) {
            int note_x = (note->track == TRACK_LEFT) ? (int)(SCREEN_WIDTH / 4.0f) : (int)(3.0f * SCREEN_WIDTH / 4.0f);
            DrawRhythmNote(note_x, (int)note_y, note->track, note->judgment, note->hit_flash_timer);
        }
    }

    // ---- 5. 判定线（带脉冲发光） ----
    int line_glow = (int)(40 + pulse_intensity * 40);
    // 外发光
    setlinecolor(RGB(line_glow, line_glow, 0));
    setlinestyle(PS_SOLID, 4);
    line(0, judgment_y, SCREEN_WIDTH, judgment_y);
    // 内线
    setlinecolor(RGB(255, 255, 100));
    setlinestyle(PS_SOLID, 2);
    line(0, judgment_y, SCREEN_WIDTH, judgment_y);
    // 判定线两端标记
    setfillcolor(RGB(255, 255, 0));
    solidcircle(10, judgment_y, 4);
    solidcircle(SCREEN_WIDTH - 10, judgment_y, 4);

    // ---- 6. Spirit 角色 ----
    float spirit_x = GetSpiritX(current_time);
    float spirit_y = GetSpiritY(current_time);
    DrawRhythmSpirit(spirit_x, spirit_y, current_time);

    // Spirit 阴影
    setfillcolor(RGB(0, 0, 0));
    int shadow_w = 18;
    fillellipse((int)spirit_x - shadow_w/2, judgment_y - 15, (int)spirit_x + shadow_w/2, judgment_y - 9);

    // ---- 7. 打击粒子 ----
    DrawRhythmParticles();

    // ---- 8. Perfect 波纹特效 ----
    if (g_game.rhythm_data.perfect_ripple_active) {
        long long ripple_elapsed = current_time - g_game.rhythm_data.perfect_ripple_start;
        if (ripple_elapsed < g_game.rhythm_data.perfect_ripple_duration) {
            float ripple_t = (float)ripple_elapsed / g_game.rhythm_data.perfect_ripple_duration;
            int base_radius = 10 + (int)(ripple_t * 100.0f);
            int fade = 255 - (int)(ripple_t * 240.0f);
            if (fade < 0) fade = 0;

            // 多层波纹
            for (int ring = 0; ring < 3; ring++) {
                int r = base_radius + ring * 15;
                int ring_fade = fade - ring * 30;
                if (ring_fade < 0) ring_fade = 0;

                setlinecolor(RGB(ring_fade, ring_fade + 40 > 255 ? 255 : ring_fade + 40, 255));
                setlinestyle(PS_SOLID, 2 + ring);
                circle((int)g_game.rhythm_data.perfect_ripple_x, judgment_y, r);
            }
        }
    }

    // ---- 9. 得分弹出文字 ----
    DrawScorePopups();

    // ---- 10. UI 信息 ----
    // 顶部状态条
    setfillcolor(RGB(0, 0, 0));
    solidrectangle(0, 0, SCREEN_WIDTH, 48);
    setfillcolor(RGB(20, 20, 40));
    solidrectangle(0, 0, SCREEN_WIDTH, 46);

    settextcolor(WHITE);
    settextstyle(16, 0, _T("Consolas"));

    // 状态文本
    const TCHAR* phase_text;
    COLORREF phase_color;
    switch (g_game.rhythm_data.sub_state) {
    case RHYTHM_PRE_START:
        phase_text = _T("GET READY..."); phase_color = RGB(255, 255, 100);
        break;
    case RHYTHM_PHASE_RECORD:
        phase_text = _T("REC  [A/D]"); phase_color = RGB(100, 200, 255);
        break;
    case RHYTHM_PHASE_ECHO:
        phase_text = _T("PLAY  [A/D] 接音符!"); phase_color = RGB(100, 255, 100);
        break;
    case RHYTHM_POST_SCORE:
        phase_text = _T("结算中..."); phase_color = RGB(255, 200, 100);
        break;
    }
    settextcolor(phase_color);
    outtextxy(10, 6, phase_text);

    // 分数
    settextcolor(RGB(255, 255, 200));
    settextstyle(20, 0, _T("Consolas"));
    TCHAR score_str[32];
    _stprintf_s(score_str, 32, _T("%d"), g_game.rhythm_data.current_score);
    int score_w = textwidth(score_str);
    outtextxy(SCREEN_WIDTH - score_w - 10, 4, score_str);

    // 连击显示（中间）
    int combo = GetCombo();
    if (combo > 1) {
        TCHAR combo_str[32];
        _stprintf_s(combo_str, 32, _T("%d COMBO"), combo);
        settextcolor(RGB(255, 200, 50));
        settextstyle(18, 0, _T("Consolas"));
        int cw = textwidth(combo_str);
        outtextxy(SCREEN_WIDTH / 2 - cw / 2, 26, combo_str);
    }

    settextstyle(16, 0, _T("Consolas")); // reset

    // ---- 11. POST_SCORE 结算面板 ----
    if (g_game.rhythm_data.sub_state == RHYTHM_POST_SCORE) {
        // 半透明遮罩
        setfillcolor(RGB(0, 0, 0));
        solidrectangle(40, 180, SCREEN_WIDTH - 40, 420);
        setfillcolor(RGB(30, 30, 60));
        solidrectangle(42, 182, SCREEN_WIDTH - 42, 418);

        settextcolor(RGB(255, 200, 50));
        settextstyle(28, 0, _T("Consolas"));
        outtextxy(SCREEN_WIDTH / 2 - 60, 200, _T("RESULTS"));

        TCHAR buf[64];
        settextstyle(18, 0, _T("Consolas"));
        settextcolor(RGB(0, 255, 100));
        _stprintf_s(buf, 64, _T("Perfect:  %d"), g_game.rhythm_data.total_perfect);
        outtextxy(80, 250, buf);

        settextcolor(RGB(255, 220, 50));
        _stprintf_s(buf, 64, _T("Good:     %d"), g_game.rhythm_data.total_good);
        outtextxy(80, 280, buf);

        settextcolor(RGB(255, 80, 80));
        _stprintf_s(buf, 64, _T("Miss:     %d"), g_game.rhythm_data.total_miss);
        outtextxy(80, 310, buf);

        settextcolor(RGB(100, 200, 255));
        _stprintf_s(buf, 64, _T("Max Combo: %d"), g_game.rhythm_data.max_combo);
        outtextxy(80, 340, buf);

        settextcolor(RGB(255, 255, 200));
        settextstyle(24, 0, _T("Consolas"));
        _stprintf_s(buf, 64, _T("Score: %d"), g_game.rhythm_data.current_score);
        outtextxy(80, 380, buf);
    }

    // ---- 12. 底部进度条 ----
    float progress = (float)elapsed / g_game.rhythm_data.bar_duration;
    if (progress > 1.0f) progress = 1.0f;
    if (progress < 0.0f) progress = 0.0f;

    // 进度条背景
    setfillcolor(RGB(30, 30, 50));
    solidrectangle(20, SCREEN_HEIGHT - 36, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 24);
    // 进度条前景
    int bar_r = (int)(100 + progress * 155);
    int bar_g = (int)(200 - progress * 100);
    setfillcolor(RGB(bar_r, bar_g, 50));
    solidrectangle(20, SCREEN_HEIGHT - 36, 20 + (int)((SCREEN_WIDTH - 40) * progress), SCREEN_HEIGHT - 24);
    // 进度条边框
    setlinecolor(RGB(150, 150, 200));
    setlinestyle(PS_SOLID, 1);
    rectangle(20, SCREEN_HEIGHT - 36, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 24);

    // 小节编号
    settextcolor(RGB(150, 150, 200));
    settextstyle(14, 0, _T("Consolas"));
    TCHAR bar_str[16];
    _stprintf_s(bar_str, 16, _T("Bar %d"), g_game.rhythm_data.bar_index + 1);
    outtextxy(SCREEN_WIDTH / 2 - 20, SCREEN_HEIGHT - 22, bar_str);

    // BPM 显示
    TCHAR bpm_str[16];
    _stprintf_s(bpm_str, 16, _T("%d BPM"), g_game.rhythm_data.bpm);
    outtextxy(SCREEN_WIDTH - 80, SCREEN_HEIGHT - 55, bpm_str);
}

void RenderFrame() {
    cleardevice();
    switch (g_game.state) {
    case STATE_AUTH: DrawAuth(); break;
    case STATE_MENU: DrawMenu(); break;
    case STATE_PLAYING: DrawGame(); break;
    case STATE_GAMEOVER: DrawGameOver(); break;
    case STATE_RHYTHM: DrawRhythm(); break;
    }
    FlushBatchDraw();
}