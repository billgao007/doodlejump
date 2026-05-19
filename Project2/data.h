#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
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
#define MAX_PARTICLES 64

typedef enum { STATE_AUTH, STATE_MENU, STATE_PLAYING, STATE_GAMEOVER, STATE_RHYTHM } GameState;

typedef enum { MODE_NORMAL, MODE_RHYTHM } GameMode;

typedef enum { RHYTHM_PRE_START, RHYTHM_PHASE_RECORD, RHYTHM_PHASE_ECHO, RHYTHM_POST_SCORE } RhythmSubState;
typedef enum { TRACK_LEFT, TRACK_RIGHT } TrackID;
typedef enum { JUDGMENT_NONE, JUDGMENT_PERFECT, JUDGMENT_GOOD, JUDGMENT_MISS } JudgmentType;

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
    int fire_rate;  // 发射间隔
    int fire_timer;
    int special_buffs; // 收集的技能数量
    int bullet_double_stacks; // 子弹数量翻倍叠加层数
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
} Bullet;

typedef struct BulletNode {
    Bullet bullet;
    struct BulletNode* next;
} BulletNode;

typedef struct {
    float x, y, radius;
    BuffType type;
    int active;
} Buff;

typedef struct {
    float x, y;
    float vx, vy;
    int life;
    int max_life;
    int r, g, b;
    int radius;
} Particle;

// 节奏模式：音符结构
#define MAX_RHYTHM_NOTES 128
#define MAX_SCORE_POPUPS 16
typedef struct {
    long long relative_timestamp; // 相对于小节起始点的毫秒数
    TrackID track; // 音符轨道（左/右）
    int is_handled; // P2是否已响应
    JudgmentType judgment; // 判定结果
    int hit_flash_timer; // 命中闪烁计时（帧数），>0 表示正在播放命中动画
} RhythmNote;

// 节奏模式：浮动得分弹出文字
typedef struct {
    float x, y;
    float vy; // 上浮速度
    int life;  // 剩余帧数
    int max_life;
    JudgmentType type; // PERFECT/GOOD/MISS
    int combo; // 当时连击数
} ScorePopup;

// 节奏模式：打击粒子特效
#define MAX_RHYTHM_PARTICLES 48
typedef struct {
    float x, y;
    float vx, vy;
    int life;
    int max_life;
    int r, g, b;
    float radius;
} RhythmParticle;

// 节奏模式：主数据结构
typedef struct {
    RhythmSubState sub_state;
    int bar_index; // 当前小节编号
    long long bar_start_time; // 当前小节起始系统时间（ms）
    int bpm; // 每分钟节拍数
    long long bar_duration; // 一个小节的总时长（ms）
    
    // P1 (录音者) 数据
    RhythmNote recorded_sequence[MAX_RHYTHM_NOTES];
    int recorded_count;
    
    // P2 (模仿者) 数据
    float spirit_x; // Spirit 位置（左墙=0，右墙=SCREEN_WIDTH）
    float spirit_target_x; // Spirit 目标位置
    long long spirit_lerp_start; // Spirit 跳跃动画起始时间
    int spirit_lerp_duration; // Spirit 跳跃持续时长（ms）
    float spirit_y; // Spirit Y 坐标（带弹跳动画）
    float spirit_vy; // Spirit 垂直速度（弹跳用）
    float spirit_base_y; // Spirit 基础 Y 坐标（判定线附近）
    int spirit_bob_timer; // 待机浮动计时器

    int p2_input_handled; // 防抖：P2本小节是否已输入
    long long p2_input_time; // P2 最后输入时间
    
    int total_perfect;
    int total_good;
    int total_miss;
    int current_score;

    // 连击系统
    int combo;       // 当前连击数
    int max_combo;   // 本小节最大连击

    // Perfect 判定特效波纹
    int perfect_ripple_active;
    long long perfect_ripple_start;
    int perfect_ripple_duration;
    float perfect_ripple_x;

    // 浮动得分弹出文字
    ScorePopup score_popups[MAX_SCORE_POPUPS];

    // 打击粒子特效
    RhythmParticle rhythm_particles[MAX_RHYTHM_PARTICLES];

    // 背景视觉相位（用于 BPM 同步脉冲）
    float bg_pulse_phase;
    // 轨道高亮计时器（音符接近判定线时亮起）
    int track_glow_timer[2]; // [0]=左轨, [1]=右轨

    // 音符下落速度倍率（可随难度调整）
    float fall_speed_mult;
} RhythmData;

/// 包含当前状态、玩家信息、平台、Boss、子弹、道具
typedef struct {
    GameState state;
    User current_user;
    Player player;
    Platform platforms[PLATFORM_COUNT];
    Boss boss;
    BulletNode bullet_pool[MAX_BULLETS];
    BulletNode* bullet_active_head;
    BulletNode* bullet_free_head;
    Buff buffs[MAX_BUFFS];
    Particle particles[MAX_PARTICLES];
    TCHAR buff_hint[64];
    int buff_hint_timer;
    GameMode mode;
    int mode_flash_timer; // 菜单模式切换的视觉提示计时器 (ticks)
    RhythmData rhythm_data; // 节奏模式数据
    
    int gravity_dir; // 1 为向下，-1 为向上
    float time_scale; // 时间流速 (1.0 或 0.5)
    int slow_timer;   // 减速持续时间
    float logic_accumulator; // 逻辑步进累加器

    int score;
    int is_running;
} GameData;

extern GameData g_game;
