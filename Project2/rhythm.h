#pragma once
#include "data.h"
#include "audio.h"

// 节奏模式常量（渲染也需要用）
#define PERFECT_WINDOW 80    // ms
#define GOOD_WINDOW 150      // ms
#define FALLING_SPEED 0.25f  // pixels per ms
#define SPIRIT_LERP_TIME 80  // ms
#define JUDGMENT_Y 520       // 判定线 Y 坐标
#define HIT_FLASH_FRAMES 12  // 命中闪烁帧数

// 初始化节奏模式
void InitRhythm();

// 更新节奏模式逻辑
void UpdateRhythm(long long current_time);

// P1 (录音者) 在当前时间点添加一个音符
void RecordNote(long long current_time, TrackID track);

// P2 (模仿者) 移动 Spirit 到指定轨道（A/D 控制）
void MoveSpirit(TrackID track);

// 自动判定：检测是否有音符到达判定线，根据 Spirit 位置判定
void AutoJudgeNotes(long long current_time);

// 获取 Spirit 的当前 X 坐标（带 Lerp 动画）
float GetSpiritX(long long current_time);

// 获取 Spirit 的当前 Y 坐标（带弹跳动画）
float GetSpiritY(long long current_time);

// 获取一个音符在屏幕上的 Y 坐标
float GetNoteYPosition(long long note_timestamp, long long current_time, float falling_speed);

// 获取连击数
int GetCombo();

// 获取浮动得分弹出数组（供渲染使用）
const ScorePopup* GetScorePopups(int* out_count);

// 获取打击粒子数组（供渲染使用）
const RhythmParticle* GetRhythmParticles(int* out_count);

// 获取轨道高亮状态
int GetTrackGlow(TrackID track);

// 结束当前小节，计算匹配率
void FinishBar();

// 重置节奏模式数据
void ResetRhythm();
