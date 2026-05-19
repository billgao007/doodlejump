#include "rhythm.h"
#include "audio.h"
#include <math.h>
#include <string.h>

#define POPUP_LIFE 45
#define PLAYER_SECONDS 5

// ---- 内部辅助 ----

static void AdvanceToNextBar(void) {
    RhythmData* rd = &g_game.rhythm_data;
    rd->bar_index++;
    int saved_score   = rd->current_score;
    int saved_perfect = rd->total_perfect;
    int saved_good    = rd->total_good;
    int saved_miss    = rd->total_miss;
    int saved_settle  = rd->settlement_round_counter;
    InitRhythm();
    rd->current_score   = saved_score;
    rd->total_perfect   = saved_perfect;
    rd->total_good      = saved_good;
    rd->total_miss      = saved_miss;
    rd->settlement_round_counter = saved_settle;
    rd->pending_settlement = 0;
}

static void SpawnExplosion(float x, float y) {
    int spawned = 0;
    for (int i = 0; i < MAX_RHYTHM_PARTICLES && spawned < 20; i++) {
        if (g_game.rhythm_data.rhythm_particles[i].life <= 0) {
            float angle = (float)(spawned) / 20.0f * 6.28318f;
            float speed = 3.0f + (float)(spawned % 5) * 0.8f;
            g_game.rhythm_data.rhythm_particles[i].x = x;
            g_game.rhythm_data.rhythm_particles[i].y = y;
            g_game.rhythm_data.rhythm_particles[i].vx = cosf(angle) * speed;
            g_game.rhythm_data.rhythm_particles[i].vy = sinf(angle) * speed - 2.0f;
            g_game.rhythm_data.rhythm_particles[i].life = 25;
            g_game.rhythm_data.rhythm_particles[i].max_life = 25;
            g_game.rhythm_data.rhythm_particles[i].r = 255;
            g_game.rhythm_data.rhythm_particles[i].g = 80 + (spawned % 5) * 20;
            g_game.rhythm_data.rhythm_particles[i].b = 20;
            g_game.rhythm_data.rhythm_particles[i].radius = 2.0f + (float)(spawned % 3);
            spawned++;
        }
    }
}

// 生成一次判定结果（含弹飞/爆炸、粒子、弹出文字、连击、音效）
static void ApplyJudgment(RhythmNote* note, JudgmentType judgment, float note_x, float note_y, long long current_time) {
    note->judgment = judgment;
    note->is_handled = 1;
    note->hit_flash_timer = HIT_FLASH_FRAMES;

    int score_add = 0;

    if (judgment == JUDGMENT_PERFECT) {
        g_game.rhythm_data.total_perfect++;
        g_game.rhythm_data.combo++;
        score_add = 100 + g_game.rhythm_data.combo * 5;
        g_game.rhythm_data.current_score += score_add;
        g_game.rhythm_data.perfect_ripple_active = 1;
        g_game.rhythm_data.perfect_ripple_start = current_time;
        g_game.rhythm_data.perfect_ripple_x = note_x;

        // 向上弹跳（落在板子上方，飞出屏幕顶部）
        note->bounce_active = 1;
        note->bounce_timer = BOUNCE_DURATION;
        note->bounce_vx = (note->track == TRACK_LEFT) ? -2.0f : 2.0f; // 轻微横向偏移
        note->bounce_vy = -22.0f;  // 强力向上弹起，飞出屏幕
    } else if (judgment == JUDGMENT_GOOD) {
        g_game.rhythm_data.total_good++;
        g_game.rhythm_data.combo++;
        score_add = 50 + g_game.rhythm_data.combo * 2;
        g_game.rhythm_data.current_score += score_add;

        note->bounce_active = 1;
        note->bounce_timer = BOUNCE_DURATION;
        note->bounce_vx = (note->track == TRACK_LEFT) ? -1.5f : 1.5f;
        note->bounce_vy = -16.0f;
    } else {
        g_game.rhythm_data.total_miss++;
        g_game.rhythm_data.combo = 0;

        // Miss：爆炸消失
        note->exploding = 1;
        note->explode_timer = EXPLODE_DURATION;
        SpawnExplosion(note_x, note_y);
    }

    if (g_game.rhythm_data.combo > g_game.rhythm_data.max_combo) {
        g_game.rhythm_data.max_combo = g_game.rhythm_data.combo;
    }

    PlayNoteBeep(note->pitch_index);
    if (judgment == JUDGMENT_PERFECT)      PlayEffect(SOUND_PERFECT);
    else if (judgment == JUDGMENT_GOOD)    PlayEffect(SOUND_GOOD);
    else                                   PlayEffect(SOUND_MISS);

    // 浮动得分弹出文字 — 屏幕中央偏上
    for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
        if (g_game.rhythm_data.score_popups[i].life <= 0) {
            g_game.rhythm_data.score_popups[i].x = (float)SCREEN_WIDTH / 2.0f;
            g_game.rhythm_data.score_popups[i].y = 100.0f; // 屏幕上方
            g_game.rhythm_data.score_popups[i].vy = -1.2f;  // 缓慢上浮
            g_game.rhythm_data.score_popups[i].life = POPUP_LIFE;
            g_game.rhythm_data.score_popups[i].max_life = POPUP_LIFE;
            g_game.rhythm_data.score_popups[i].type = judgment;
            g_game.rhythm_data.score_popups[i].combo = g_game.rhythm_data.combo;
            break;
        }
    }

    // 命中粒子（Perfect/Good 才有，Miss 已用 SpawnExplosion）
    if (judgment != JUDGMENT_MISS) {
        int pc = (judgment == JUDGMENT_PERFECT) ? 12 : 6;
        int sp = 0;
        for (int i = 0; i < MAX_RHYTHM_PARTICLES && sp < pc; i++) {
            if (g_game.rhythm_data.rhythm_particles[i].life <= 0) {
                float a = (float)(sp) / pc * 6.28318f;
                float s = (judgment == JUDGMENT_PERFECT) ? 3.5f : 2.0f;
                g_game.rhythm_data.rhythm_particles[i].x = note_x;
                g_game.rhythm_data.rhythm_particles[i].y = note_y;
                g_game.rhythm_data.rhythm_particles[i].vx = cosf(a) * s;
                g_game.rhythm_data.rhythm_particles[i].vy = sinf(a) * s - 1.0f;
                g_game.rhythm_data.rhythm_particles[i].life = 18;
                g_game.rhythm_data.rhythm_particles[i].max_life = 18;
                g_game.rhythm_data.rhythm_particles[i].r = (judgment == JUDGMENT_PERFECT) ? 0 : 255;
                g_game.rhythm_data.rhythm_particles[i].g = (judgment == JUDGMENT_PERFECT) ? 255 : 220;
                g_game.rhythm_data.rhythm_particles[i].b = (judgment == JUDGMENT_PERFECT) ? 100 : 50;
                g_game.rhythm_data.rhythm_particles[i].radius = 2.5f;
                sp++;
            }
        }
    }
}

// --- 公开接口 ---

void InitRhythm() {
    RhythmData* rd = &g_game.rhythm_data;
    memset(rd, 0, sizeof(RhythmData));

    rd->sub_state = RHYTHM_PRE_START;
    rd->bar_index = 0;
    rd->bpm = 120;
    rd->bar_duration = (long long)PLAYER_SECONDS * 2 * 1000LL;
    rd->bar_start_time = GetGameTimeMs();
    
    // 板子初始居中
    rd->platform_x = (float)SCREEN_WIDTH / 2.0f;
    rd->platform_target_x = rd->platform_x;
    rd->platform_lerp_duration = SPIRIT_LERP_TIME;

    rd->perfect_ripple_duration = 200;
    rd->fall_speed_mult = 1.0f;
    rd->bg_pulse_phase = 0.0f;

    for (int i = 0; i < MAX_SCORE_POPUPS; i++)  rd->score_popups[i].life = 0;
    for (int i = 0; i < MAX_RHYTHM_PARTICLES; i++) rd->rhythm_particles[i].life = 0;
}

void UpdateRhythm(long long current_time) {
    RhythmData* rd = &g_game.rhythm_data;
    long long elapsed = current_time - rd->bar_start_time;

    // 更新背景脉冲相位（与 BPM 同步）
    float beat_duration = 60000.0f / rd->bpm;
    rd->bg_pulse_phase = fmodf((float)elapsed / beat_duration, 1.0f);

    // 更新弹出文字生命周期
    for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
        if (rd->score_popups[i].life > 0) {
            rd->score_popups[i].life--;
            rd->score_popups[i].y += rd->score_popups[i].vy;
        }
    }

    // 更新打击粒子
    for (int i = 0; i < MAX_RHYTHM_PARTICLES; i++) {
        if (rd->rhythm_particles[i].life > 0) {
            rd->rhythm_particles[i].life--;
            rd->rhythm_particles[i].x += rd->rhythm_particles[i].vx;
            rd->rhythm_particles[i].y += rd->rhythm_particles[i].vy;
            rd->rhythm_particles[i].vy += 0.15f;
            rd->rhythm_particles[i].radius *= 0.94f;
        }
    }

    // 更新音符命中闪烁
    for (int i = 0; i < rd->recorded_count; i++) {
        if (rd->recorded_sequence[i].hit_flash_timer > 0)
            rd->recorded_sequence[i].hit_flash_timer--;
    }

    // 更新轨道高亮
    for (int t = 0; t < 2; t++) {
        if (rd->track_glow_timer[t] > 0) rd->track_glow_timer[t]--;
    }

    // 更新弹飞 / 爆炸动画计时
    for (int i = 0; i < rd->recorded_count; i++) {
        RhythmNote* note = &rd->recorded_sequence[i];
        if (note->bounce_active) {
            note->bounce_timer--;
            note->bounce_vy += 0.55f; // 重力加速下落
            if (note->bounce_timer <= 0) note->bounce_active = 0;
        }
        if (note->exploding) {
            note->explode_timer--;
            if (note->explode_timer <= 0) note->exploding = 0;
        }
        if (note->hit_flash_timer > 0)
            note->hit_flash_timer--;
    }
    
    // 更新 Perfect 波纹
    if (rd->perfect_ripple_active) {
        long long ripple_elapsed = current_time - rd->perfect_ripple_start;
        if (ripple_elapsed >= rd->perfect_ripple_duration)
            rd->perfect_ripple_active = 0;
    }

    // ECHO 阶段：自动判定 + Miss 超时检测
    if (rd->sub_state == RHYTHM_PHASE_ECHO) {
        AutoJudgeNotes(current_time);

        long long echo_start = rd->bar_start_time + rd->bar_duration / 2;
        for (int i = 0; i < rd->recorded_count; i++) {
            RhythmNote* note = &rd->recorded_sequence[i];
            if (!note->is_handled) {
                // 音符目标命中时间 = echo_start + relative_timestamp
                long long note_target = echo_start + note->relative_timestamp;
                if (current_time > note_target + GOOD_WINDOW) {
                    float note_x = (note->track == TRACK_LEFT) ? (SCREEN_WIDTH / 4.0f) : (3.0f * SCREEN_WIDTH / 4.0f);
                    float note_y = GetEchoNoteY(note_target, current_time, FALLING_SPEED * rd->fall_speed_mult);
                    ApplyJudgment(note, JUDGMENT_MISS, note_x, note_y, current_time);
                }
            }
        }
    }

    // ---- 状态机推进 ----
    if (rd->sub_state == RHYTHM_PRE_START && elapsed > 1000) {
        rd->sub_state = RHYTHM_PHASE_RECORD;
        rd->bar_start_time = GetGameTimeMs();
        rd->recorded_count = 0;
        PlayEffect(SOUND_BAR_START);
    }
    else if (rd->sub_state == RHYTHM_PHASE_RECORD && elapsed > rd->bar_duration / 2) {
        rd->sub_state = RHYTHM_PHASE_ECHO;
        rd->p2_input_time = 0;
    }
    else if (rd->sub_state == RHYTHM_PHASE_ECHO && elapsed > rd->bar_duration) {
        FinishBar();
        rd->settlement_round_counter++;
        // 每 3 轮显示结算面板
        rd->pending_settlement = (rd->settlement_round_counter % 3 == 0) ? 1 : 0;
        rd->sub_state = RHYTHM_POST_SCORE;
    }
    else if (rd->sub_state == RHYTHM_POST_SCORE) {
        if (rd->pending_settlement) {
            // 显示结算面板 2 秒
            if (elapsed > rd->bar_duration + 2000) {
                AdvanceToNextBar();
            }
        } else {
            // 不显示结算：立即进入下一轮
            AdvanceToNextBar();
        }
    }
}

void RecordNote(long long current_time, TrackID track) {
    RhythmData* rd = &g_game.rhythm_data;
    if (rd->sub_state != RHYTHM_PHASE_RECORD) return;

    // 短防抖 60ms
    if (current_time - rd->p2_input_time < 60) return;
    rd->p2_input_time = current_time;
    
    long long relative_time = current_time - rd->bar_start_time;
    
    if (rd->recorded_count < MAX_RHYTHM_NOTES) {
        RhythmNote* note = &rd->recorded_sequence[rd->recorded_count];
        note->relative_timestamp = relative_time;
        note->track = track;
        note->is_handled = 0;
        note->judgment = JUDGMENT_NONE;
        note->hit_flash_timer = 0;
        // 按轨道分配音高：左轨 do-mi-sol-xi 循环，右轨 re-fa-la-do 循环
        int seq = rd->recorded_count / 2; // 同轨编号
        if (track == TRACK_LEFT)
            note->pitch_index = (seq * 2) % 8;      // 0,2,4,6 → do, mi, sol, xi
        else
            note->pitch_index = (seq * 2 + 1) % 8;  // 1,3,5,7 → re, fa, la, do(高)
        rd->recorded_count++;
    }
}

void MovePlatform(TrackID track) {
    RhythmData* rd = &g_game.rhythm_data;
    long long now = GetGameTimeMs();
    if (now - rd->p2_input_time < 60) return;
    rd->p2_input_time = now;

    rd->platform_target_x = (track == TRACK_LEFT) ? (SCREEN_WIDTH / 4.0f) : (3.0f * SCREEN_WIDTH / 4.0f);
    rd->platform_lerp_start = now;
    rd->platform_x = GetPlatformX(now);
}

void ReleasePlatform(long long current_time) {
    RhythmData* rd = &g_game.rhythm_data;
    // 只在板子不在中央时才回中，避免重复触发
    float center = (float)SCREEN_WIDTH / 2.0f;
    if (fabsf(rd->platform_target_x - center) < 1.0f) return;
    rd->platform_target_x = center;
    rd->platform_lerp_start = current_time;
    rd->platform_x = GetPlatformX(current_time);
}

void AutoJudgeNotes(long long current_time) {
    RhythmData* rd = &g_game.rhythm_data;
    float plat_cx = GetPlatformX(current_time);
    float plat_y = GetPlatformY();
    float fall_speed = FALLING_SPEED * rd->fall_speed_mult;
    long long echo_start = rd->bar_start_time + rd->bar_duration / 2;

    // 板子半宽 30px，player 半径 15px → 碰撞检测范围 ≈ 45px
    float catch_range = 45.0f;

    for (int i = 0; i < rd->recorded_count; i++) {
        RhythmNote* note = &rd->recorded_sequence[i];
        if (note->is_handled) continue;

        long long note_target = echo_start + note->relative_timestamp;
        float note_y = GetEchoNoteY(note_target, current_time, fall_speed);
        float note_x = (note->track == TRACK_LEFT) ? (SCREEN_WIDTH / 4.0f) : (3.0f * SCREEN_WIDTH / 4.0f);

        // 水平碰撞检测：板子 X 范围与 player X 重叠
        float dist_x = fabsf(plat_cx - note_x);
        // 垂直碰撞检测：player 到达板子高度附近
        float dist_y = fabsf(note_y - plat_y);

        if (dist_x < catch_range && dist_y < 36.0f) {
            // 碰撞到了！只判这一个音符（单次按键单判 — 最近的未处理音符）
            long long timing_error = current_time - note_target;
            if (timing_error < 0) timing_error = -timing_error;

            if (timing_error < PERFECT_WINDOW) {
                ApplyJudgment(note, JUDGMENT_PERFECT, note_x, note_y, current_time);
                rd->track_glow_timer[note->track] = 20;
            } else if (timing_error < GOOD_WINDOW) {
                ApplyJudgment(note, JUDGMENT_GOOD, note_x, note_y, current_time);
                rd->track_glow_timer[note->track] = 12;
            } else {
                ApplyJudgment(note, JUDGMENT_MISS, note_x, note_y, current_time);
            }
            // 只判一个就退出，防止一次碰撞判定多个音符
            break;
        }
    }
}

float GetPlatformX(long long current_time) {
    RhythmData* rd = &g_game.rhythm_data;
    long long elapsed = current_time - rd->platform_lerp_start;
    if (elapsed >= rd->platform_lerp_duration) {
        return rd->platform_target_x;
    }
    float t = (float)elapsed / rd->platform_lerp_duration;
    float eased = 1.0f - (1.0f - t) * (1.0f - t); // ease-out
    return rd->platform_x + (rd->platform_target_x - rd->platform_x) * eased;
}

float GetPlatformY() {
    return (float)JUDGMENT_Y; // 板子固定在判定线高度
}

float GetNoteYPosition(long long note_timestamp, long long current_time, float falling_speed) {
    long long elapsed = current_time - note_timestamp;
    if (elapsed < 0) return -100;
    return (float)elapsed * falling_speed;
}

// 回放阶段专用：从判定线反推音符 Y 坐标
// target_time = echo_start + relative_timestamp（音符到达判定线的时刻）
//   → 到达判定线时 Y = JUDGMENT_Y
//   → 提前出现时 Y < JUDGMENT_Y（从屏幕上方下落）
float GetEchoNoteY(long long target_time, long long current_time, float falling_speed) {
    long long remaining = target_time - current_time;
    return (float)JUDGMENT_Y - (float)remaining * falling_speed;
}

int GetCombo() {
    return g_game.rhythm_data.combo;
}

const ScorePopup* GetScorePopups(int* out_count) {
    *out_count = MAX_SCORE_POPUPS;
    return g_game.rhythm_data.score_popups;
}

const RhythmParticle* GetRhythmParticles(int* out_count) {
    *out_count = MAX_RHYTHM_PARTICLES;
    return g_game.rhythm_data.rhythm_particles;
}

int GetTrackGlow(TrackID track) {
    return g_game.rhythm_data.track_glow_timer[track];
}

void FinishBar() {
    RhythmData* rd = &g_game.rhythm_data;
    long long now = GetGameTimeMs();
    long long echo_start = rd->bar_start_time + rd->bar_duration / 2;
    for (int i = 0; i < rd->recorded_count; i++) {
        RhythmNote* note = &rd->recorded_sequence[i];
        if (!note->is_handled) {
            float note_x = (note->track == TRACK_LEFT) ? (SCREEN_WIDTH / 4.0f) : (3.0f * SCREEN_WIDTH / 4.0f);
            long long note_target = echo_start + note->relative_timestamp;
            float note_y = GetEchoNoteY(note_target, now, FALLING_SPEED * rd->fall_speed_mult);
            if (note_y > SCREEN_HEIGHT) note_y = (float)JUDGMENT_Y;
            ApplyJudgment(note, JUDGMENT_MISS, note_x, note_y, now);
        }
    }
}

void ResetRhythm() {
    RhythmData* rd = &g_game.rhythm_data;
    rd->recorded_count = 0;
    rd->p2_input_time = 0;
    rd->perfect_ripple_active = 0;
    rd->combo = 0;
    for (int i = 0; i < MAX_RHYTHM_NOTES; i++) {
        rd->recorded_sequence[i].is_handled = 0;
        rd->recorded_sequence[i].judgment = JUDGMENT_NONE;
        rd->recorded_sequence[i].hit_flash_timer = 0;
        rd->recorded_sequence[i].bounce_active = 0;
        rd->recorded_sequence[i].exploding = 0;
    }
    for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
        rd->score_popups[i].life = 0;
    }
    for (int i = 0; i < MAX_RHYTHM_PARTICLES; i++) {
        rd->rhythm_particles[i].life = 0;
    }
}
