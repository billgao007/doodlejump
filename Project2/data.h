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

typedef enum { STATE_AUTH, STATE_MENU, STATE_PLAYING, STATE_VICTORY, STATE_GAMEOVER, STATE_RHYTHM, STATE_PAUSED } GameState;

typedef enum { MODE_NORMAL, MODE_RHYTHM } GameMode;

typedef enum { RHYTHM_PRE_START, RHYTHM_PHASE_RECORD, RHYTHM_PHASE_ECHO, RHYTHM_POST_SCORE } RhythmSubState;
typedef enum { TRACK_LEFT, TRACK_RIGHT } TrackID;
typedef enum { JUDGMENT_NONE, JUDGMENT_PERFECT, JUDGMENT_GOOD, JUDGMENT_MISS } JudgmentType;

typedef enum { PLAT_NORMAL, PLAT_FAKE, PLAT_SPRING } PlatType;
typedef enum { BUFF_FIRE_RATE, BUFF_DMG_ADD, BUFF_DMG_MULT, BUFF_TIME, BUFF_HIGH_JUMP } BuffType;

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
    int high_jump_charges; // 高跳充能次数（按9触发）
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
    int spread_warning_time; // 弹幕散射预警计时
    int spread_fire_timer;   // 弹幕多波发射间隔计时
    int spread_wave_fired;   // 已发射波数
} Boss;

typedef struct {
    float x, y, vx, vy;
    int damage;
    int is_boss_bullet; // 1=Boss弹幕(向下伤玩家), 0=玩家子弹(向上伤Boss)
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
    int hit_flash_timer; // 命中闪烁计时（帧数）
    int pitch_index; // 音高编号 0-7

    // 弹飞动画（被 platform 接住后）
    int bounce_active;
    float bounce_vx, bounce_vy;
    int bounce_timer;

    // 爆炸消失动画（Miss 后）
    int exploding;
    int explode_timer;
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
    
    // P2 (模仿者) 数据 — 用板子接住下落的 player
    float platform_x;           // 板子当前 X 坐标（中心）
    float platform_target_x;    // 板子目标 X 坐标
    long long platform_lerp_start; // 板子移动动画起始时间
    int platform_lerp_duration; // 板子移动持续时长（ms）

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

    // 轮次结算系统：每3轮显示一次结算面板
    int settlement_round_counter; // 当前已完成的轮数（每轮结束+1）
    int pending_settlement;       // 1=当前应显示结算，0=跳过直接继续
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

    // 无尽连战模式
    int endless_mode;           // 0=普通, 1=无尽连战
    int boss_respawn_count;     // Boss 重生次数
    int damage_bonus_level;     // 伤害翻倍等级（每级翻倍一次）
    int next_damage_bonus_score; // 下次伤害翻倍需要的分数
    int boss_respawn_effect_timer; // Boss 复活特效计时器

    GameState state_before_pause; // 暂停前状态，用于恢复
} GameData;

extern GameData g_game;
