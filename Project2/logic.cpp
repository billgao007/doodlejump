#include "logic.h"
#include "data.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static int logic_initialized = 0;
const TCHAR* DB_FILE = _T("users.dat");






// 尝试登录
int AttemptLogin(const TCHAR* username, const TCHAR* password) {
    FILE* fp;
    // 以只读二进制模式打开文件，如果文件不存在说明还没人注册过，直接返回失败
    if (_tfopen_s(&fp, DB_FILE, _T("rb")) != 0) return 0;

    User u;
    // 遍历读取文件中的每一个 User 结构体
    while (fread(&u, sizeof(User), 1, fp) == 1) {
        // 比对账号和密码
        if (_tcscmp(u.username, username) == 0 && _tcscmp(u.password, password) == 0) {
            g_game.current_user = u; // 匹配成功，载入当前用户数据
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0; // 遍历完未找到，或密码错误
}

// 尝试注册
int AttemptRegister(const TCHAR* username, const TCHAR* password) {
    FILE* fp;
    
    // 1. 先检查是否已经存在同名账号
    if (_tfopen_s(&fp, DB_FILE, _T("rb")) == 0) {
        User u;
        while (fread(&u, sizeof(User), 1, fp) == 1) {
            if (_tcscmp(u.username, username) == 0) {
                fclose(fp);
                return 0; // 账号已存在，注册失败
            }
        }
        fclose(fp);
    }

    // 2. 账号不存在，执行注册逻辑。以追加二进制模式打开
    if (_tfopen_s(&fp, DB_FILE, _T("ab")) != 0) return 0;

    User newUser;
    _tcscpy_s(newUser.username, 32, username);
    _tcscpy_s(newUser.password, 32, password);
    newUser.max_score = 0; // 新用户最高分为 0

    // 将新用户写入文件末尾
    fwrite(&newUser, sizeof(User), 1, fp);
    fclose(fp);

    // 注册成功后自动登录
    g_game.current_user = newUser;
    return 1;
}

// 保存/刷新最高分
void SaveHighScore() {
    FILE* fp;
    // 以读写二进制模式打开
    if (_tfopen_s(&fp, DB_FILE, _T("r+b")) != 0) return;

    User u;
    long pos = 0;
    while (fread(&u, sizeof(User), 1, fp) == 1) {
        if (_tcscmp(u.username, g_game.current_user.username) == 0) {
            // 找到当前用户后，将文件指针往回退一个 User 的长度
            fseek(fp, pos, SEEK_SET);
            // 覆盖写入更新了 max_score 的数据
            fwrite(&g_game.current_user, sizeof(User), 1, fp);
            break;
        }
        pos = ftell(fp); // 记录当前指针位置
    }
    fclose(fp);
}

void InitLogic() {
    g_game.player.x = SCREEN_WIDTH / 2.0f;
    g_game.player.y = SCREEN_HEIGHT - 100.0f;
    g_game.player.vx = 0; g_game.player.vy = JUMP_FORCE;
    g_game.player.radius = 15.0f;
    g_game.player.hp = 100; g_game.player.max_hp = 100;
    g_game.player.base_damage = 10;
    g_game.player.dmg_mult = 1.0f;
    g_game.player.fire_rate = 30; // 初始半秒发一发
    g_game.player.fire_timer = 0;
    g_game.player.special_buffs = 0;

    g_game.boss.x = SCREEN_WIDTH / 2.0f - 40.0f;
    g_game.boss.y = 20.0f;
    g_game.boss.width = 80.0f; g_game.boss.height = 40.0f;
    g_game.boss.hp = 2000; g_game.boss.max_hp = 2000;
    g_game.boss.phase = 1;
    g_game.boss.vx = 3.0f;
    g_game.boss.skill_timer = 120; // 2秒后放技能
    g_game.boss.laser_warning_time = 0;
    g_game.boss.laser_active_time = 0;

    g_game.gravity_dir = 1;
    g_game.time_scale = 1.0f;
    g_game.slow_timer = 0;
    g_game.logic_accumulator = 0.0f;
    g_game.score = 0;

    for (int i = 0; i < MAX_BULLETS; i++) g_game.bullets[i].active = 0;
    for (int i = 0; i < MAX_BUFFS; i++) g_game.buffs[i].active = 0;

    // 初始化平台
    for (int i = 0; i < PLATFORM_COUNT; i++) {
        g_game.platforms[i].width = 60.0f;
        g_game.platforms[i].height = 10.0f;
        g_game.platforms[i].type = PLAT_NORMAL;
        if (i == 0) {
            g_game.platforms[i].x = SCREEN_WIDTH / 2.0f - 30.0f;
            g_game.platforms[i].y = SCREEN_HEIGHT - 20.0f;
        } else {
            g_game.platforms[i].x = (float)GetRandomInt(0, SCREEN_WIDTH - 60);
            g_game.platforms[i].y = g_game.platforms[i - 1].y - GetRandomInt(50, 80);
        }
    }
    logic_initialized = 1;
}

// 在顶端生成一个道具
static void SpawnBuff(float x, float y) {
    for (int i = 0; i < MAX_BUFFS; i++) {
        if (!g_game.buffs[i].active) {
            g_game.buffs[i].active = 1;
            g_game.buffs[i].x = x + 30.0f; // 平台中间
            g_game.buffs[i].y = y - 15.0f;
            g_game.buffs[i].radius = 10.0f;
            g_game.buffs[i].type = (BuffType)GetRandomInt(0, 3);
            break;
        }
    }
}

// 物理与实体逻辑单步推进
static void DoLogicStep() {
    Player* p = &g_game.player;
    Boss* b = &g_game.boss;

    // 1. 时间流速处理
    if (g_game.slow_timer > 0) {
        g_game.slow_timer--;
        if (g_game.slow_timer <= 0) g_game.time_scale = 1.0f;
    }

    // 2. 玩家物理
    p->vy += GRAVITY * g_game.gravity_dir;
    p->x += p->vx;
    p->y += p->vy;

    if (p->x < -p->radius) p->x = SCREEN_WIDTH + p->radius;
    else if (p->x > SCREEN_WIDTH + p->radius) p->x = -p->radius;

    // 3. 平台碰撞与滚屏
    int falling = (g_game.gravity_dir == 1 && p->vy > 0) || (g_game.gravity_dir == -1 && p->vy < 0);
    if (falling) {
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            Platform* plat = &g_game.platforms[i];
            if (plat->type == PLAT_FAKE) continue; // 假板子直接穿透

            int hit = 0;
            if (g_game.gravity_dir == 1) { // 正常重力，踩板子上面
                if (p->x + p->radius > plat->x && p->x - p->radius < plat->x + plat->width &&
                    p->y + p->radius > plat->y && p->y + p->radius < plat->y + plat->height + p->vy) hit = 1;
            } else { // 反向重力，撞板子下面
                if (p->x + p->radius > plat->x && p->x - p->radius < plat->x + plat->width &&
                    p->y - p->radius < plat->y + plat->height && p->y - p->radius > plat->y + p->vy) hit = 1;
            }

            if (hit) {
                float force = (plat->type == PLAT_SPRING) ? JUMP_FORCE * 1.5f : JUMP_FORCE;
                p->vy = (g_game.gravity_dir == 1) ? force : -force;
                break;
            }
        }
    }

    // 滚屏 (仅在正常重力下视角跟随向上)
    if (g_game.gravity_dir == 1 && p->y < SCROLL_THRESHOLD) {
        float offset = SCROLL_THRESHOLD - p->y;
        p->y = SCROLL_THRESHOLD;
        g_game.score += (int)offset;
        
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            g_game.platforms[i].y += offset;
            if (g_game.platforms[i].y > SCREEN_HEIGHT) {
                g_game.platforms[i].x = (float)GetRandomInt(0, SCREEN_WIDTH - 60);
                g_game.platforms[i].y = 0.0f;
                g_game.platforms[i].type = PLAT_NORMAL;
                
                if (GetRandomInt(1, 100) <= 20) SpawnBuff(g_game.platforms[i].x, g_game.platforms[i].y);
            }
        }
        for (int i = 0; i < MAX_BUFFS; i++) {
            if (g_game.buffs[i].active) g_game.buffs[i].y += offset;
        }
    }

    // 4. 边缘掉落与扣血判定
    if (p->y > SCREEN_HEIGHT + p->radius) { // 掉出底部
        p->hp -= 20;
        p->vy = JUMP_FORCE * 1.5f; // 高高弹起
    } else if (p->y < -p->radius && g_game.gravity_dir == -1) { // 反转重力掉出顶部
        p->hp -= 20;
        p->vy = -JUMP_FORCE * 1.5f;
    }

    // 5. 玩家射击
    p->fire_timer++;
    if (p->fire_timer >= p->fire_rate) {
        p->fire_timer = 0;
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!g_game.bullets[i].active) {
                g_game.bullets[i].active = 1;
                g_game.bullets[i].x = p->x;
                g_game.bullets[i].y = p->y;
                g_game.bullets[i].vy = -12.0f; // 子弹永远向上飞
                g_game.bullets[i].damage = (int)(p->base_damage * p->dmg_mult);
                break;
            }
        }
    }

    // 6. 子弹更新与伤害 Boss
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (g_game.bullets[i].active) {
            g_game.bullets[i].y += g_game.bullets[i].vy;
            if (g_game.bullets[i].y < 0) g_game.bullets[i].active = 0;
            // 打中Boss
            if (g_game.bullets[i].active &&
                g_game.bullets[i].x > b->x && g_game.bullets[i].x < b->x + b->width &&
                g_game.bullets[i].y > b->y && g_game.bullets[i].y < b->y + b->height) {
                b->hp -= g_game.bullets[i].damage;
                g_game.bullets[i].active = 0;
                
                // 二阶段判定
                if (b->hp < b->max_hp / 2 && b->phase == 1) {
                    b->phase = 2;
                    g_game.gravity_dir = -1; // 重力反转！
                }
            }
        }
    }

    // 7. Buff 碰撞
    for (int i = 0; i < MAX_BUFFS; i++) {
        if (g_game.buffs[i].active) {
            Buff* buff = &g_game.buffs[i];
            if (buff->y > SCREEN_HEIGHT) buff->active = 0; // 移出屏幕销毁
            
            float dx = p->x - buff->x; float dy = p->y - buff->y;
            if (sqrt(dx*dx + dy*dy) < p->radius + buff->radius) {
                buff->active = 0;
                if (buff->type == BUFF_FIRE_RATE) {
                    p->fire_rate -= 5;
                    if (p->fire_rate < 10) p->fire_rate = 10;
                }
                else if (buff->type == BUFF_DMG_ADD) p->base_damage += 5;
                else if (buff->type == BUFF_DMG_MULT) p->dmg_mult += 0.2f;
                else if (buff->type == BUFF_TIME) p->special_buffs++;
            }
        }
    }

    // 8. Boss 逻辑
    b->x += b->vx;
    if (b->x <= 0 || b->x + b->width >= SCREEN_WIDTH) b->vx *= -1; // 左右来回

    // 撞击 Boss 扣血弹开
    if (p->x + p->radius > b->x && p->x - p->radius < b->x + b->width &&
        p->y - p->radius < b->y + b->height && p->y + p->radius > b->y) {
        p->hp -= 15;
        p->vy = (g_game.gravity_dir == 1) ? 5.0f : -5.0f; // 向下弹开
    }

    // Boss 一阶段技能
    if (b->phase == 1) {
        if (b->laser_warning_time > 0) {
            b->laser_warning_time--;
            if (b->laser_warning_time == 0) b->laser_active_time = 30; // 激光激活 0.5 秒
        } else if (b->laser_active_time > 0) {
            b->laser_active_time--;
            // 判定激光伤害
            if (p->x > b->laser_x - 15 && p->x < b->laser_x + 15) p->hp -= 1; // 每帧扣1血
        } else {
            b->skill_timer--;
            if (b->skill_timer <= 0) {
                b->skill_timer = GetRandomInt(120, 240);
                if (GetRandomInt(0, 1) == 0) {
                    // 技能1：瞄准激光
                    b->laser_warning_time = 90; // 1.5秒警告
                    b->laser_x = b->x + b->width / 2;
                } else {
                    // 技能2：改变随机平台
                    int idx = GetRandomInt(0, PLATFORM_COUNT - 1);
                    g_game.platforms[idx].type = (GetRandomInt(0, 1) == 0) ? PLAT_FAKE : PLAT_SPRING;
                }
            }
        }
    }

    // 死亡判定
    if (p->hp <= 0 || b->hp <= 0) {
        if (g_game.score > g_game.current_user.max_score) {
            g_game.current_user.max_score = g_game.score;
            SaveHighScore();
        }
        g_game.state = STATE_GAMEOVER;
    }
}

void UpdateLogic() {
    if (g_game.state != STATE_PLAYING) {
        if (g_game.state == STATE_PLAYING && !logic_initialized) InitLogic();
        else if (g_game.state == STATE_GAMEOVER || g_game.state == STATE_MENU) logic_initialized = 0;
        return;
    }

    // 根据流速缩放，使用累加器来决定执行多少次逻辑步进
    g_game.logic_accumulator += g_game.time_scale;
    while (g_game.logic_accumulator >= 1.0f) {
        DoLogicStep();
        g_game.logic_accumulator -= 1.0f;
    }
}
