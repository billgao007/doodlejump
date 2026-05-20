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

// 前置声明
static void DrawBossRespawnEffect();

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
            else if (b->type == BUFF_HIGH_JUMP) {
                // 高跳 Buff：蓝绿色发光球体
                long long tick = GetGameTimeMs();
                int pulse = (tick / 150) % 4;
                int r = (int)b->radius + pulse;
                setfillcolor(RGB(0, 200 + pulse * 10, 220));
                solidcircle((int)b->x, (int)b->y, r);
                setfillcolor(RGB(180, 240, 255));
                solidcircle((int)b->x - r/3, (int)b->y - r/3, r/2);
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
        if (node->bullet.is_boss_bullet) {
            // Boss 弹幕：橙红色，稍大
            setfillcolor(RGB(255, 80 + (node->bullet.damage * 5), 30));
            solidcircle((int)node->bullet.x, (int)node->bullet.y, 5);
            // 发光外圈
            setfillcolor(RGB(255, 180, 80));
            solidcircle((int)node->bullet.x, (int)node->bullet.y, 2);
        } else {
            int damage = node->bullet.damage;
            int red = ClampColor(120 + (damage - 10) * 10);
            int green = ClampColor(90 - (damage - 10) * 4);
            int blue = ClampColor(90 - (damage - 10) * 4);
            setfillcolor(RGB(red, green, blue));
            solidcircle((int)node->bullet.x, (int)node->bullet.y, 3);
        }
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

    // Boss 复活特效（在 Boss 本体下方绘制）
    DrawBossRespawnEffect();

    // 散射预警：Boss 红色闪烁
    if (b->spread_warning_time > 0) {
        long long tick = GetGameTimeMs();
        int flash = ((tick / 100) % 2); // 每100ms交替闪烁
        if (flash) {
            setfillcolor(RGB(255, 60, 60));
            solidrectangle((int)b->x - 4, (int)b->y - 4,
                           (int)(b->x + b->width + 4), (int)(b->y + b->height + 4));
        }
    }

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

    _stprintf_s(textBuf, 64, _T("Dmg: %d x%.1f | Time: %d | Jump: %d"),
        g_game.player.base_damage, g_game.player.dmg_mult, g_game.player.special_buffs, g_game.player.high_jump_charges);
    outtextxy(120, SCREEN_HEIGHT - 20, textBuf);

    // 无尽模式专属 HUD（深色底栏保证可读性）
    if (g_game.endless_mode) {
        // 半透明深色底栏
        setfillcolor(RGB(20, 20, 40));
        solidrectangle(0, 18, SCREEN_WIDTH, 38);
        setfillcolor(RGB(40, 40, 70));
        solidrectangle(2, 20, SCREEN_WIDTH - 2, 36);

        setbkmode(TRANSPARENT);
        settextcolor(RGB(100, 255, 180)); settextstyle(14, 0, _T("Consolas"));
        TCHAR endless_str[64];
        _stprintf_s(endless_str, 64, _T("Round: %d  |  HP: x%d  |  Dmg Lv: %d"),
            g_game.boss_respawn_count + 1,
            1 << g_game.boss_respawn_count,
            g_game.damage_bonus_level);
        outtextxy(10, 22, endless_str);

        // 伤害翻倍进度条
        int progress = (g_game.score % 500) * 100 / 500;
        setfillcolor(RGB(30, 30, 30));
        solidrectangle(SCREEN_WIDTH - 112, 7, SCREEN_WIDTH - 8, 16);
        setfillcolor(RGB(255, 180, 30));
        solidrectangle(SCREEN_WIDTH - 110, 7, SCREEN_WIDTH - 110 + progress, 16);
        settextcolor(RGB(220, 220, 220)); settextstyle(10, 0, _T("Consolas"));
        outtextxy(SCREEN_WIDTH - 108, 8, _T("NEXT DMG x2"));
        setbkmode(OPAQUE);
    }

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

// ========== 胜利界面（击败Boss后） ==========
static void DrawVictory() {
    // 背景
    setfillcolor(RGB(20, 20, 40));
    solidrectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // 主标题
    settextcolor(RGB(255, 215, 0)); settextstyle(36, 0, _T("Consolas"));
    outtextxy(35, 100, _T("BOSS DEFEATED!"));

    // 分数
    settextcolor(WHITE); settextstyle(22, 0, _T("Consolas"));
    TCHAR str[64];
    _stprintf_s(str, 64, _T("Score: %d"), g_game.score);
    int tw = textwidth(str);
    outtextxy((SCREEN_WIDTH - tw) / 2, 180, str);

    _stprintf_s(str, 64, _T("Best: %d"), g_game.current_user.max_score);
    tw = textwidth(str);
    outtextxy((SCREEN_WIDTH - tw) / 2, 210, str);

    // 选项
    settextstyle(20, 0, _T("Consolas"));
    long long tick = GetGameTimeMs();
    settextcolor(RGB(100, 255, 100));
    outtextxy(50, 280, _T("[SPACE] 无尽连战模式"));

    // 闪烁提示
    if ((tick / 600) % 2) {
        settextcolor(RGB(255, 255, 100));
        outtextxy(60, 320, _T("Boss 血量逐次翻倍!"));
        outtextxy(60, 345, _T("爬升越高，子弹越强!"));
    }

    settextcolor(RGB(180, 180, 180));
    outtextxy(80, 400, _T("[ESC] 返回主菜单"));
}

// ========== Boss 复活特效 ==========
static void DrawBossRespawnEffect() {
    if (g_game.boss_respawn_effect_timer <= 0) return;

    Boss* b = &g_game.boss;
    float cx = b->x + b->width / 2.0f;
    float cy = b->y + b->height / 2.0f;
    float t = 1.0f - (float)g_game.boss_respawn_effect_timer / (2.0f * (float)FPS); // 0 → 1
    long long tick = GetGameTimeMs();

    // 多层扩散光环
    for (int ring = 0; ring < 3; ring++) {
        float phase = t * 3.0f - ring * 0.35f;
        if (phase < 0 || phase > 1.5f) continue;

        int radius = (int)(phase * 150.0f);
        int alpha = (int)(255 * (1.0f - phase / 1.5f));
        int r = 255;
        int g = ClampColor(215 - (int)(phase * 150));
        int b = ClampColor(alpha / 3);

        setlinecolor(RGB(r, g, b));
        setlinestyle(PS_SOLID, 3 - ring);
        circle((int)cx, (int)cy, radius);
    }

    // 粒子爆发
    int particle_count = 24;
    for (int i = 0; i < particle_count; i++) {
        float angle = (float)i / particle_count * 6.28318f + t * 2.0f;
        float dist = t * 120.0f + (i % 3) * 20.0f;
        int px = (int)(cx + cosf(angle) * dist);
        int py = (int)(cy + sinf(angle) * dist);
        int alpha = (int)(200 * (1.0f - t));

        int pr = 255;
        int pg = ClampColor(200 - (int)(t * 150) + (i % 3) * 30);
        int pb = ClampColor(alpha / 2 + (i % 2) * 50);
        setfillcolor(RGB(pr, pg, pb));
        solidcircle(px, py, 2 + (i % 3));
    }

    // 中心闪光
    int flash_r = (int)(40 * (1.0f - t));
    if (flash_r > 0) {
        setfillcolor(RGB(255, 255, 200));
        solidcircle((int)cx, (int)cy, flash_r);
        setfillcolor(RGB(255, 255, 255));
        solidcircle((int)cx, (int)cy, flash_r / 2);
    }
}

// ========== 节奏模式：绘制打击粒子 ==========
static void DrawScorePopups() {
    int popup_count;
    const ScorePopup* popups = GetScorePopups(&popup_count);

    setbkmode(TRANSPARENT);

    for (int i = 0; i < popup_count; i++) {
        if (popups[i].life <= 0) continue;

        float t = (float)popups[i].life / popups[i].max_life;
        int alpha = (int)(255 * t);
        int font_size = 28 + (int)(14 * t); // 大字体 28→42

        const TCHAR* text;
        COLORREF color;
        if (popups[i].type == JUDGMENT_PERFECT) {
            text = _T("PERFECT!");
            color = RGB(min(alpha + 40, 255), min(alpha + 80, 255), min(alpha / 2, 200));
        } else if (popups[i].type == JUDGMENT_GOOD) {
            text = _T("GOOD");
            color = RGB(min(alpha + 100, 255), min(alpha + 60, 255), 0);
        } else {
            text = _T("MISS");
            color = RGB(min(alpha + 80, 255), min(alpha / 3, 100), min(alpha / 3, 100));
        }

        settextcolor(color);
        settextstyle(font_size, 0, _T("Consolas"));

        // 居中偏上显示（X 屏幕正中，Y 初始在 120px，缓慢上浮）
        int tx = SCREEN_WIDTH / 2 - (int)textwidth(text) / 2;
        int ty = (int)popups[i].y; // y 由 ScorePopup 管理（初始约 100），缓慢上浮
        outtextxy(tx, ty, text);

        // 连击显示在下方
        if (popups[i].combo > 1 && popups[i].type != JUDGMENT_MISS) {
            TCHAR combo_str[32];
            _stprintf_s(combo_str, 32, _T("%d combo!"), popups[i].combo);
            settextcolor(RGB(min(alpha + 50, 255), min(alpha + 100, 255), min(alpha + 200, 255)));
            settextstyle(font_size - 8, 0, _T("Consolas"));
            int cx = SCREEN_WIDTH / 2 - (int)textwidth(combo_str) / 2;
            outtextxy(cx, ty + font_size + 4, combo_str);
        }

        settextstyle(16, 0, _T("Consolas")); // reset
    }

    setbkmode(OPAQUE);
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

// ========== 节奏模式：主绘制函数 ==========
static void DrawRhythm() {
    long long current_time = GetGameTimeMs();
    long long elapsed = current_time - g_game.rhythm_data.bar_start_time;
    float fall_speed = FALLING_SPEED * g_game.rhythm_data.fall_speed_mult;
    int judgment_y = JUDGMENT_Y;
    RhythmData* rd = &g_game.rhythm_data;

    // ---- 1. 背景：单次填充（消除逐行绘制的性能瓶颈）----
    setfillcolor(RGB(180, 210, 240));
    solidrectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    // ---- 2. 左右半区淡色标识 ----
    int left_glow = GetTrackGlow(TRACK_LEFT);
    int right_glow = GetTrackGlow(TRACK_RIGHT);

    // 左半区
    int lg = 220 - left_glow * 5; if (lg < 180) lg = 180;
    setfillcolor(RGB(lg, lg, 255));
    solidrectangle(0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT);
    if (left_glow > 0) {
        setfillcolor(RGB(150, 180, min(255, 200 + left_glow * 4)));
        solidrectangle(0, judgment_y - 30, SCREEN_WIDTH / 2, judgment_y + 30);
    }

    // 右半区
    int rg = 220 - right_glow * 5; if (rg < 180) rg = 180;
    setfillcolor(RGB(255, rg, rg));
    solidrectangle(SCREEN_WIDTH / 2, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (right_glow > 0) {
        setfillcolor(RGB(min(255, 200 + right_glow * 4), 150, 180));
        solidrectangle(SCREEN_WIDTH / 2, judgment_y - 30, SCREEN_WIDTH, judgment_y + 30);
    }

    // 分隔虚线
    setlinecolor(RGB(180, 180, 220));
    setlinestyle(PS_DASH, 1);
    line(SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT);

    // ---- 3. 判定线 ----
    setlinecolor(RGB(255, 200, 100));
    setlinestyle(PS_SOLID, 2);
    line(0, judgment_y, SCREEN_WIDTH, judgment_y);

    // ---- 4. 下落的 player（音符） + 弹飞/爆炸动画 ----
    {
        long long echo_start = rd->bar_start_time + rd->bar_duration / 2;
        for (int i = 0; i < rd->recorded_count; i++) {
            RhythmNote* note = &rd->recorded_sequence[i];

            float note_x, note_y;
            if (note->bounce_active) {
                // 抛物线弹跳：player 从板子向上飞出屏幕
                note_x = (note->track == TRACK_LEFT) ? (SCREEN_WIDTH / 4.0f) : (3.0f * SCREEN_WIDTH / 4.0f);
                float bt = (float)(BOUNCE_DURATION - note->bounce_timer) / BOUNCE_DURATION;
                note_x = note_x + note->bounce_vx * bt * 14.0f;
                note_y = judgment_y + note->bounce_vy * bt * 12.0f;
            } else if (note->exploding) {
                // 爆炸中：画在判定线位置，由粒子表现爆炸
                note_x = (note->track == TRACK_LEFT) ? (SCREEN_WIDTH / 4.0f) : (3.0f * SCREEN_WIDTH / 4.0f);
                note_y = judgment_y;
            } else {
                // 正常下落
                long long note_target = echo_start + note->relative_timestamp;
                note_x = (note->track == TRACK_LEFT) ? (SCREEN_WIDTH / 4.0f) : (3.0f * SCREEN_WIDTH / 4.0f);
                note_y = GetEchoNoteY(note_target, current_time, fall_speed);
            }

            if (note_y > -40 && note_y < SCREEN_HEIGHT + 40 && !note->exploding) {
                int ix = (int)(note_x - 15);
                int iy = (int)(note_y - 15);

                // 命中闪烁光晕
                if (note->hit_flash_timer > 0 && !note->bounce_active) {
                    float ft = (float)note->hit_flash_timer / HIT_FLASH_FRAMES;
                    int fa = (int)(150 * ft);
                    COLORREF fc;
                    if (note->judgment == JUDGMENT_PERFECT) fc = RGB(0, 255, 100);
                    else if (note->judgment == JUDGMENT_GOOD) fc = RGB(255, 220, 50);
                    else fc = RGB(255, 50, 50);
                    setfillcolor(RGB(min(GetRValue(fc), 255), min(GetGValue(fc), 255), min(GetBValue(fc), 255)));
                    solidcircle((int)note_x, (int)note_y, 22);
                }

                // 拖尾虚影
                for (int t = 1; t <= 5; t++) {
                    float ty = note_y - t * 8.0f;
                    if (ty < -20) break;
                    int alpha = 100 - t * 18;
                    if (alpha < 0) alpha = 0;
                    // EasyX putimage 不支持透明度，用淡色矩形模拟
                    setfillcolor(RGB(alpha + 100, alpha + 100, alpha + 100));
                    solidrectangle(ix - 1, (int)ty - 1, ix + 31, (int)ty + 31);
                }

                // 绘制 player.png
                putimage(ix, iy, &img_player);
            }
        }
    }

    // ---- 5. 板子（platform.png） ----
    float plat_x = GetPlatformX(current_time);
    float plat_y = GetPlatformY();
    int pw = 60, ph = 10;
    // 板子阴影
    setfillcolor(RGB(0, 0, 0));
    fillellipse((int)plat_x - pw/2 + 2, (int)plat_y + 4, (int)plat_x + pw/2 + 2, (int)plat_y + 8);
    // 板子贴图
    putimage((int)(plat_x - pw/2), (int)(plat_y - ph/2), &img_platform);

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
    const TCHAR* phase_text = _T("");
    COLORREF phase_color = RGB(255, 255, 255);
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
    int score_w = (int)textwidth(score_str);
    outtextxy(SCREEN_WIDTH - score_w - 10, 4, score_str);

    // 连击显示（中间）
    int combo = GetCombo();
    if (combo > 1) {
        TCHAR combo_str[32];
        _stprintf_s(combo_str, 32, _T("%d COMBO"), combo);
        settextcolor(RGB(255, 200, 50));
        settextstyle(18, 0, _T("Consolas"));
        int cw = (int)textwidth(combo_str);
        outtextxy(SCREEN_WIDTH / 2 - cw / 2, 26, combo_str);
    }

    settextstyle(16, 0, _T("Consolas")); // reset

    // ---- 11. POST_SCORE 结算面板 ----
    if (g_game.rhythm_data.sub_state == RHYTHM_POST_SCORE) {
        // 半透明遮罩（更深更实）
        setfillcolor(RGB(0, 0, 0));
        solidrectangle(40, 160, SCREEN_WIDTH - 40, 430);
        setfillcolor(RGB(15, 20, 50));
        solidrectangle(42, 162, SCREEN_WIDTH - 42, 428);

        setbkmode(TRANSPARENT);
        settextcolor(RGB(255, 220, 80));
        settextstyle(32, 0, _T("Consolas"));
        outtextxy(SCREEN_WIDTH / 2 - (int)textwidth(_T("RESULTS")) / 2, 180, _T("RESULTS"));

        TCHAR buf[64];
        int left_x = 80;
        settextstyle(22, 0, _T("Consolas"));

        settextcolor(RGB(120, 255, 120));
        _stprintf_s(buf, 64, _T("Perfect:   %d"), g_game.rhythm_data.total_perfect);
        outtextxy(left_x, 230, buf);

        settextcolor(RGB(255, 255, 100));
        _stprintf_s(buf, 64, _T("Good:      %d"), g_game.rhythm_data.total_good);
        outtextxy(left_x, 268, buf);

        settextcolor(RGB(255, 120, 120));
        _stprintf_s(buf, 64, _T("Miss:      %d"), g_game.rhythm_data.total_miss);
        outtextxy(left_x, 306, buf);

        settextcolor(RGB(130, 200, 255));
        _stprintf_s(buf, 64, _T("Max Combo: %d"), g_game.rhythm_data.max_combo);
        outtextxy(left_x, 344, buf);

        settextcolor(RGB(255, 255, 255));
        settextstyle(28, 0, _T("Consolas"));
        _stprintf_s(buf, 64, _T("Score: %d"), g_game.rhythm_data.current_score);
        outtextxy(left_x, 390, buf);

        setbkmode(OPAQUE);
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
    case STATE_VICTORY: DrawVictory(); break;
    case STATE_GAMEOVER: DrawGameOver(); break;
    case STATE_RHYTHM: DrawRhythm(); break;
    case STATE_PAUSED:
        // 先画游戏画面，再覆盖半透明遮罩
        if (g_game.state_before_pause == STATE_RHYTHM) DrawRhythm();
        else DrawGame();
        // 半透明黑色遮罩
        setfillcolor(RGB(0, 0, 0));
        setlinecolor(RGB(0, 0, 0));
        setlinestyle(PS_SOLID, 1);
        for (int i = 0; i < SCREEN_HEIGHT; i += 2) {
            line(0, i, SCREEN_WIDTH, i);
        }
        // 暂停文字
        settextcolor(RGB(255, 255, 255)); settextstyle(40, 0, _T("Consolas"));
        outtextxy(120, 250, _T("PAUSED"));
        settextcolor(RGB(200, 200, 200)); settextstyle(16, 0, _T("Consolas"));
        outtextxy(95, 310, _T("Press P to Resume"));
        outtextxy(100, 335, _T("Press ESC to Quit"));
        break;
    }
    FlushBatchDraw();
}