#pragma once
#include <tchar.h>

#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 600
#define FPS 60

#define GRAVITY 0.4f
#define JUMP_FORCE -10.0f
#define PLAYER_SPEED 5.0f
#define PLATFORM_COUNT 12
#define SCROLL_THRESHOLD 300.0f

#define MAX_BULLETS 50
#define MAX_BUFFS 10

typedef enum { STATE_AUTH, STATE_MENU, STATE_PLAYING, STATE_GAMEOVER } GameState;

typedef enum { PLAT_NORMAL, PLAT_FAKE, PLAT_SPRING } PlatType;
typedef enum { BUFF_FIRE_RATE, BUFF_DMG_ADD, BUFF_DMG_MULT, BUFF_TIME } BuffType;

typedef struct {
    TCHAR username[32];
    TCHAR password[32];
    int max_score;
} User;

typedef struct {
    float x, y, vx, vy, radius;
    int hp, max_hp;
    int base_damage;
    float dmg_mult;
    int fire_rate;  // 发射间隔(帧)
    int fire_timer;
    int special_buffs; // 收集到的'0'技能数量
} Player;

typedef struct {
    float x, y, width, height;
    PlatType type;
} Platform;

typedef struct {
    float x, y, width, height;
    int hp, max_hp;
    int phase;
    float vx;
    int skill_timer;
    float laser_x;
    int laser_warning_time;
    int laser_active_time;
} Boss;

typedef struct {
    float x, y, vy;
    int damage;
    int active;
} Bullet;

typedef struct {
    float x, y, radius;
    BuffType type;
    int active;
} Buff;

typedef struct {
    GameState state;
    User current_user;
    Player player;
    Platform platforms[PLATFORM_COUNT];
    Boss boss;
    Bullet bullets[MAX_BULLETS];
    Buff buffs[MAX_BUFFS];
    
    int gravity_dir; // 1 为向下，-1 为向上
    float time_scale; // 时间流速 (1.0 或 0.5)
    int slow_timer;   // 减速持续时间
    float logic_accumulator; // 逻辑步进累加器

    int score;
    int is_running;
} GameData;

extern GameData g_game;
