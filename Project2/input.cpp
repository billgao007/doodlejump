#include "input.h"
#include "data.h"
#include "logic.h"
#include "rhythm.h"
#include <graphics.h>
#include <tchar.h>
#include <windows.h> 

static int key_0_pressed = 0;
static int key_9_pressed = 0;
static int menu_left_pressed = 0;
static int menu_right_pressed = 0;
static int rhythm_a_prev = 0;  // P1 录音：A 键上一帧状态
static int rhythm_d_prev = 0;  // P1 录音：D 键上一帧状态
static int plat_any_down_prev = 0; // P2 回放：上一帧是否有方向键按下
static RhythmSubState rhythm_prev_sub_state = RHYTHM_PRE_START;


static bool GetCustomInput(const TCHAR* title, const TCHAR* prompt, TCHAR* buf, int maxLen, bool isPwd) {
    int len = (int)_tcslen(buf);
    ExMessage msg;
    
    
    while (peekmessage(&msg, EX_KEY | EX_CHAR)) {} 

    while (true) {
        // 绘制背景
        setfillcolor(RGB(220, 230, 240)); 
        solidrectangle(40, 180, 360, 380);
        setlinecolor(BLACK);
        rectangle(40, 180, 360, 380);
        
        // 绘制标题
        settextcolor(BLACK);
        settextstyle(22, 0, _T("Consolas"));
        outtextxy(50, 190, title);
        
        settextstyle(18, 0, _T("Consolas"));
        outtextxy(50, 230, prompt);
        
        // 输入框
        setfillcolor(WHITE);
        solidrectangle(50, 260, 350, 290);
        setlinecolor(BLACK);
        rectangle(50, 260, 350, 290);
        
        // 密码为星号
        TCHAR displayBuf[64] = {0};
        if (isPwd) {
            for (int i = 0; i < len; i++) displayBuf[i] = _T('*');
        } else {
            _tcscpy_s(displayBuf, 64, buf);
        }
        
        // 动态闪烁的光标
        if ((GetGameTimeMs() / 500) % 2 == 0) {
            _tcscat_s(displayBuf, 64, _T("_"));
        }
        
        outtextxy(55, 266, displayBuf);
        settextcolor(RGB(100, 100, 100));
        settextstyle(16, 0, _T("Consolas"));
        outtextxy(50, 330, _T("[Enter] 确认    [Esc] 取消"));
        
        FlushBatchDraw();
        
        // 处理键盘输入 
        while (peekmessage(&msg, EX_CHAR)) {
            if (msg.message == WM_CHAR) {
                TCHAR c = msg.ch;
                if (c == '\r' || c == '\n') { // 回车键确认
                    return true;
                } else if (c == '\b') { // 退格键 (删除)
                    if (len > 0) buf[--len] = '\0';
                } else if (c == 27) { // Esc键取消
                    buf[0] = '\0';
                    return false;
                } else if (c >= 32 && c <= 126 && len < maxLen - 1) { // 正常的可见字符
                    buf[len++] = c;
                    buf[len] = '\0';
                }
            }
        }
        Sleep(10); 
    }
    return false;
}

void ProcessInput() {
    
    if (g_game.state == STATE_AUTH) {
        
        // 按 L 键登录
        if (GetAsyncKeyState('L') & 0x8000) {
            // 防抖：等待玩家把 L 键松开
            while(GetAsyncKeyState('L') & 0x8000) Sleep(10); 

            TCHAR user[32] = { 0 }, pass[32] = { 0 };
            
            if (GetCustomInput(_T("【用户登录】"), _T("请输入账号:"), user, 32, false)) {
                if (GetCustomInput(_T("【用户登录】"), _T("请输入密码:"), pass, 32, true)) {
                    if (AttemptLogin(user, pass)) {
                        MessageBox(GetHWnd(), _T("登录成功！"), _T("欢迎"), MB_OK | MB_ICONINFORMATION);
                        g_game.state = STATE_MENU;
                    } else {
                        MessageBox(GetHWnd(), _T("登录失败：账号不存在或密码错误！"), _T("错误"), MB_OK | MB_ICONERROR);
                    }
                }
            }
        }
        
        // 按 R 键注册
        if (GetAsyncKeyState('R') & 0x8000) {
            
            while(GetAsyncKeyState('R') & 0x8000) Sleep(10);

            TCHAR user[32] = { 0 }, pass[32] = { 0 };
            
            if (GetCustomInput(_T("【用户注册】"), _T("请设置新账号:"), user, 32, false)) {
                if (GetCustomInput(_T("【用户注册】"), _T("请设置新密码:"), pass, 32, true)) {
                    if (AttemptRegister(user, pass)) {
                        MessageBox(GetHWnd(), _T("注册成功，已自动为您登录！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
                        g_game.state = STATE_MENU;
                    } else {
                        MessageBox(GetHWnd(), _T("注册失败：该用户名已被注册！"), _T("错误"), MB_OK | MB_ICONWARNING);
                    }
                }
            }
        }
        
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) g_game.is_running = 0;
        return;
    }

    // 菜单与结算界面
    if (g_game.state == STATE_MENU || g_game.state == STATE_GAMEOVER) {
        // 切换模式：左/右 或 A/D
        if (GetAsyncKeyState(VK_LEFT) & 0x8000 || GetAsyncKeyState('A') & 0x8000) {
            if (!menu_left_pressed) {
                g_game.mode = MODE_NORMAL;
                g_game.mode_flash_timer = FPS / 2; // 半秒高亮
            }
            menu_left_pressed = 1;
        } else {
            menu_left_pressed = 0;
        }

        if (GetAsyncKeyState(VK_RIGHT) & 0x8000 || GetAsyncKeyState('D') & 0x8000) {
            if (!menu_right_pressed) {
                g_game.mode = MODE_RHYTHM;
                g_game.mode_flash_timer = FPS / 2; // 半秒高亮
            }
            menu_right_pressed = 1;
        } else {
            menu_right_pressed = 0;
        }

        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            if (g_game.mode == MODE_NORMAL) {
                g_game.state = STATE_PLAYING;
            } else {
                g_game.state = STATE_RHYTHM;
                InitRhythm();
            }
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) g_game.is_running = 0;
        return;
    }

    // 胜利界面（击败Boss后）
    if (g_game.state == STATE_VICTORY) {
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            // 进入无尽连战模式
            while (GetAsyncKeyState(VK_SPACE) & 0x8000) Sleep(10);
            g_game.endless_mode = 1;
            g_game.boss_respawn_count = 0;
            g_game.damage_bonus_level = 0;
            // 下一档伤害翻倍基于当前分数，避免继承的分数瞬间触发多次翻倍
            g_game.next_damage_bonus_score = g_game.score + 500;
            g_game.boss_respawn_effect_timer = 2 * FPS;
            // Boss 复活并继续游戏
            RespawnBoss();
            g_game.state = STATE_PLAYING;
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            while (GetAsyncKeyState(VK_ESCAPE) & 0x8000) Sleep(10);
            g_game.state = STATE_MENU;
        }
        return;
    }

    // 游戏内交互 
    if (g_game.state == STATE_PLAYING) {
        g_game.player.vx = 0;
        
        if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000) {
            g_game.player.vx = -PLAYER_SPEED;
        }
        if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000) {
            g_game.player.vx = PLAYER_SPEED;
        }

        if (GetAsyncKeyState('0') & 0x8000) {
            if (!key_0_pressed && g_game.player.special_buffs > 0 && g_game.time_scale == 1.0f) {
                g_game.player.special_buffs--;
                g_game.time_scale = 0.5f;
                g_game.slow_timer = 5 * FPS; 
            }
            key_0_pressed = 1;
        } else {
            key_0_pressed = 0;
        }

        // 按 9 触发高跳
        if (GetAsyncKeyState('9') & 0x8000) {
            if (!key_9_pressed && g_game.player.high_jump_charges > 0) {
                g_game.player.high_jump_charges--;
                // 给玩家一个强力向上的速度
                g_game.player.vy = (g_game.gravity_dir == 1) ? JUMP_FORCE * 2.5f : -JUMP_FORCE * 2.5f;
            }
            key_9_pressed = 1;
        } else {
            key_9_pressed = 0;
        }
    }

    // 节奏模式交互
    if (g_game.state == STATE_RHYTHM) {
        // P1 (录音阶段): A = 左轨道，D = 右轨道 — 按下瞬间生成一个音符
        if (g_game.rhythm_data.sub_state == RHYTHM_PHASE_RECORD) {
            // 刚切换到录音阶段时重置按键状态，避免上一轮按住带入
            if (rhythm_prev_sub_state != RHYTHM_PHASE_RECORD) {
                rhythm_a_prev = 0;
                rhythm_d_prev = 0;
            }
            int a_now = (GetAsyncKeyState('A') & 0x8000) ? 1 : 0;
            int d_now = (GetAsyncKeyState('D') & 0x8000) ? 1 : 0;
            // 边缘触发：仅当按键从未按下变为按下时生成一个音符
            if (a_now && !rhythm_a_prev) {
                RecordNote(GetGameTimeMs(), TRACK_LEFT);
            }
            if (d_now && !rhythm_d_prev) {
                RecordNote(GetGameTimeMs(), TRACK_RIGHT);
            }
            rhythm_a_prev = a_now;
            rhythm_d_prev = d_now;
        }

        // P2 (回放阶段): A/← = 板子去左轨，D/→ = 板子去右轨，松键回中（边缘触发）
        if (g_game.rhythm_data.sub_state == RHYTHM_PHASE_ECHO) {
            // 刚切换到回放阶段时重置状态
            if (rhythm_prev_sub_state != RHYTHM_PHASE_ECHO) {
                plat_any_down_prev = 0;
            }
            int a_down = (GetAsyncKeyState('A') & 0x8000) || (GetAsyncKeyState(VK_LEFT) & 0x8000);
            int d_down = (GetAsyncKeyState('D') & 0x8000) || (GetAsyncKeyState(VK_RIGHT) & 0x8000);
            int any_down = a_down || d_down;

            if (a_down) {
                MovePlatform(TRACK_LEFT);
            } else if (d_down) {
                MovePlatform(TRACK_RIGHT);
            } else if (plat_any_down_prev && !any_down) {
                // 边缘触发：刚从按下变为松开时，回中一次
                ReleasePlatform(GetGameTimeMs());
            }
            plat_any_down_prev = any_down;
        }

        // ESC 退出节奏模式
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            g_game.state = STATE_MENU;
            ResetRhythm();
        }

        rhythm_prev_sub_state = g_game.rhythm_data.sub_state;
    }
}