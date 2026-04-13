#include "input.h"
#include "data.h"
#include "logic.h"
#include <graphics.h>
#include <tchar.h>

// 增加按键状态防抖
static int key_0_pressed = 0;

void ProcessInput() {
    if (g_game.state == STATE_AUTH) {
        // 按 L 键登录
        if (GetAsyncKeyState('L') & 0x8000) {
            TCHAR user[32] = { 0 }, pass[32] = { 0 };
            
            // 弹出输入框获取账号密码
            InputBox(user, 32, _T("请输入账号:"), _T("用户登录"), _T(""), 0, 0, false);
            if (_tcslen(user) > 0) {
                InputBox(pass, 32, _T("请输入密码:"), _T("用户登录"), _T(""), 0, 0, false);
                
                // 调用文件对比逻辑
                if (AttemptLogin(user, pass)) {
                    MessageBox(GetHWnd(), _T("登录成功！"), _T("欢迎"), MB_OK | MB_ICONINFORMATION);
                    g_game.state = STATE_MENU;
                } else {
                    MessageBox(GetHWnd(), _T("登录失败：账号不存在或密码错误！"), _T("错误"), MB_OK | MB_ICONERROR);
                }
            }
        }
        
        // 按 R 键注册
        if (GetAsyncKeyState('R') & 0x8000) {
            TCHAR user[32] = { 0 }, pass[32] = { 0 };
            
            InputBox(user, 32, _T("请设置新账号:"), _T("用户注册"), _T(""), 0, 0, false);
            if (_tcslen(user) > 0) {
                InputBox(pass, 32, _T("请设置新密码:"), _T("用户注册"), _T(""), 0, 0, false);
                
                // 调用文件写入逻辑
                if (AttemptRegister(user, pass)) {
                    MessageBox(GetHWnd(), _T("注册成功，已自动为您登录！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
                    g_game.state = STATE_MENU;
                } else {
                    MessageBox(GetHWnd(), _T("注册失败：该用户名已被注册！"), _T("错误"), MB_OK | MB_ICONWARNING);
                }
            }
        }
        
        // 按 ESC 退出游戏
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) g_game.is_running = 0;
        return;
    }

    if (g_game.state == STATE_MENU || g_game.state == STATE_GAMEOVER) {
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) g_game.state = STATE_PLAYING;
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) g_game.is_running = 0;
        return;
    }

    if (g_game.state == STATE_PLAYING) {
        g_game.player.vx = 0;
        if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000) {
            g_game.player.vx = -PLAYER_SPEED;
        }
        if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000) {
            g_game.player.vx = PLAYER_SPEED;
        }

        // 新增：按下 '0' 键激活时间减缓（防抖）
        if (GetAsyncKeyState('0') & 0x8000) {
            if (!key_0_pressed && g_game.player.special_buffs > 0 && g_game.time_scale == 1.0f) {
                g_game.player.special_buffs--;
                g_game.time_scale = 0.5f;
                g_game.slow_timer = 5 * FPS; // 持续 5 秒
            }
            key_0_pressed = 1;
        } else {
            key_0_pressed = 0;
        }
    }
}
