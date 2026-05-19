#include "rhythm.h"
#include "audio.h"
#include <math.h>
#include <string.h>

#define SPIRIT_BOUNCE_TIME 200 // ms 弹跳动画时长
#define POPUP_LIFE 45          // 弹出文字存活帧数

// --- 内部辅助函数 ---

// 生成一次判定结果（含粒子、弹出文字、连击、音效）
static void ApplyJudgment(RhythmNote* note, JudgmentType judgment, float note_x, float note_y, long long current_time) {
    note->judgment = judgment;
    note->is_handled = 1;
    note->hit_flash_timer = HIT_FLASH_FRAMES;

    int score_add = 0;
    SoundType sound = SOUND_MISS;

    if (judgment == JUDGMENT_PERFECT) {
        g_game.rhythm_data.total_perfect++;
        g_game.rhythm_data.combo++;
        score_add = 100 + g_game.rhythm_data.combo * 5; // 连击加分
        g_game.rhythm_data.current_score += score_add;
        sound = SOUND_PERFECT;

        // Perfect 波纹特效
        g_game.rhythm_data.perfect_ripple_active = 1;
        g_game.rhythm_data.perfect_ripple_start = current_time;
        g_game.rhythm_data.perfect_ripple_x = note_x;
    } else if (judgment == JUDGMENT_GOOD) {
        g_game.rhythm_data.total_good++;
        g_game.rhythm_data.combo++;
        score_add = 50 + g_game.rhythm_data.combo * 2;
        g_game.rhythm_data.current_score += score_add;
        sound = SOUND_GOOD;
    } else {
        g_game.rhythm_data.total_miss++;
        g_game.rhythm_data.combo = 0; // 断连
        sound = SOUND_MISS;
    }

    if (g_game.rhythm_data.combo > g_game.rhythm_data.max_combo) {
        g_game.rhythm_data.max_combo = g_game.rhythm_data.combo;
    }

    PlayEffect(sound);

    // 生成浮动得分弹出文字
    for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
        if (g_game.rhythm_data.score_popups[i].life <= 0) {
            g_game.rhythm_data.score_popups[i].x = note_x;
            g_game.rhythm_data.score_popups[i].y = note_y - 20;
            g_game.rhythm_data.score_popups[i].vy = -2.5f;
            g_game.rhythm_data.score_popups[i].life = POPUP_LIFE;
            g_game.rhythm_data.score_popups[i].max_life = POPUP_LIFE;
            g_game.rhythm_data.score_popups[i].type = judgment;
            g_game.rhythm_data.score_popups[i].combo = g_game.rhythm_data.combo;
            break;
        }
    }

    // 生成打击粒子
    int particle_count = (judgment == JUDGMENT_PERFECT) ? 16 : (judgment == JUDGMENT_GOOD ? 8 : 4);
    int spawned = 0;
    for (int i = 0; i < MAX_RHYTHM_PARTICLES && spawned < particle_count; i++) {
        if (g_game.rhythm_data.rhythm_particles[i].life <= 0) {
            float angle = (float)(spawned) / particle_count * 6.28318f;
            float speed = (judgment == JUDGMENT_PERFECT) ? 4.0f : 2.5f;
            g_game.rhythm_data.rhythm_particles[i].x = note_x;
            g_game.rhythm_data.rhythm_particles[i].y = note_y;
            g_game.rhythm_data.rhythm_particles[i].vx = cosf(angle) * speed;
            g_game.rhythm_data.rhythm_particles[i].vy = sinf(angle) * speed - 1.5f;
            g_game.rhythm_data.rhythm_particles[i].life = 20;
            g_game.rhythm_data.rhythm_particles[i].max_life = 20;
            if (judgment == JUDGMENT_PERFECT) {
                g_game.rhythm_data.rhythm_particles[i].r = 0;
                g_game.rhythm_data.rhythm_particles[i].g = 255;
                g_game.rhythm_data.rhythm_particles[i].b = 100;
            } else if (judgment == JUDGMENT_GOOD) {
                g_game.rhythm_data.rhythm_particles[i].r = 255;
                g_game.rhythm_data.rhythm_particles[i].g = 220;
                g_game.rhythm_data.rhythm_particles[i].b = 50;
            } else {
                g_game.rhythm_data.rhythm_particles[i].r = 255;
                g_game.rhythm_data.rhythm_particles[i].g = 60;
                g_game.rhythm_data.rhythm_particles[i].b = 60;
            }
            g_game.rhythm_data.rhythm_particles[i].radius = 2.5f;
            spawned++;
        }
    }

    // Spirit 弹跳动画
    g_game.rhythm_data.spirit_vy = -8.0f;
}

// --- 公开接口 ---

void InitRhythm() {
    RhythmData* rd = &g_game.rhythm_data;
    memset(rd, 0, sizeof(RhythmData));

    rd->sub_state = RHYTHM_PRE_START;
    rd->bar_index = 0;
    rd->bpm = 120;
    rd->bar_duration = (60000LL * 4) / rd->bpm;
    rd->bar_start_time = GetGameTimeMs();
    
    rd->spirit_x = (float)SCREEN_WIDTH / 2.0f; // 初始居中
    rd->spirit_target_x = rd->spirit_x;
    rd->spirit_lerp_duration = SPIRIT_LERP_TIME;
    rd->spirit_base_y = (float)JUDGMENT_Y - 30.0f;
    rd->spirit_y = rd->spirit_base_y;
    rd->spirit_vy = 0.0f;

    rd->perfect_ripple_duration = 200;
    rd->fall_speed_mult = 1.0f;
    rd->bg_pulse_phase = 0.0f;

    // 初始化弹出文字状态
    for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
        rd->score_popups[i].life = 0;
    }
    for (int i = 0; i < MAX_RHYTHM_PARTICLES; i++) {
        rd->rhythm_particles[i].life = 0;
    }
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
            rd->rhythm_particles[i].vy += 0.15f; // 重力
            rd->rhythm_particles[i].radius *= 0.94f;
        }
    }

    // 更新音符命中闪烁计时器
    for (int i = 0; i < rd->recorded_count; i++) {
        if (rd->recorded_sequence[i].hit_flash_timer > 0) {
            rd->recorded_sequence[i].hit_flash_timer--;
        }
    }

    // 更新轨道高亮计时器
    for (int t = 0; t < 2; t++) {
        if (rd->track_glow_timer[t] > 0) rd->track_glow_timer[t]--;
    }

    // 更新 Spirit 弹跳
    rd->spirit_y += rd->spirit_vy;
    rd->spirit_vy += 0.5f; // 重力
    if (rd->spirit_y >= rd->spirit_base_y) {
        rd->spirit_y = rd->spirit_base_y;
        rd->spirit_vy = 0.0f;
    }

    // Spirit 待机浮动
    rd->spirit_bob_timer = (rd->spirit_bob_timer + 1) % 120;
    
    // 更新 Perfect 波纹
    if (rd->perfect_ripple_active) {
        long long ripple_elapsed = current_time - rd->perfect_ripple_start;
        if (ripple_elapsed >= rd->perfect_ripple_duration) {
            rd->perfect_ripple_active = 0;
        }
    }

    // ECHO 阶段：自动判定 + Miss 超时检测
    if (rd->sub_state == RHYTHM_PHASE_ECHO) {
        AutoJudgeNotes(current_time);

        // 超时 Miss 检测
        for (int i = 0; i < rd->recorded_count; i++) {
            RhythmNote* note = &rd->recorded_sequence[i];
            if (!note->is_handled) {
                long long note_deadline = rd->bar_start_time + note->relative_timestamp + GOOD_WINDOW;
                if (current_time > note_deadline) {
                    float note_x = (note->track == TRACK_LEFT) ? (SCREEN_WIDTH / 4.0f) : (3.0f * SCREEN_WIDTH / 4.0f);
                    float note_y = GetNoteYPosition(rd->bar_start_time + note->relative_timestamp, current_time, FALLING_SPEED * rd->fall_speed_mult);
                    ApplyJudgment(note, JUDGMENT_MISS, note_x, note_y, current_time);
                }
            }
        }
    }

    // 状态机推进
    if (rd->sub_state == RHYTHM_PRE_START && elapsed > 1000) {
        rd->sub_state = RHYTHM_PHASE_RECORD;
        rd->bar_start_time = GetGameTimeMs();
        rd->recorded_count = 0;
        PlayEffect(SOUND_BAR_START);
    }
    else if (rd->sub_state == RHYTHM_PHASE_RECORD && elapsed > rd->bar_duration / 2) {
        rd->sub_state = RHYTHM_PHASE_ECHO;
        rd->p2_input_time = 0; // 重置防抖，避免录音阶段防抖影响回放操作
    }
    else if (rd->sub_state == RHYTHM_PHASE_ECHO && elapsed > rd->bar_duration) {
        FinishBar();
        rd->sub_state = RHYTHM_POST_SCORE;
    }
    else if (rd->sub_state == RHYTHM_POST_SCORE && elapsed > rd->bar_duration + 2000) {
        rd->bar_index++;
        // 保留累计分数，重置其他
        int saved_score = rd->current_score;
        int saved_perfect = rd->total_perfect;
        int saved_good = rd->total_good;
        int saved_miss = rd->total_miss;
        InitRhythm();
        rd->current_score = saved_score;
        rd->total_perfect = saved_perfect;
        rd->total_good = saved_good;
        rd->total_miss = saved_miss;
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
        rd->recorded_count++;
    }
}

void MoveSpirit(TrackID track) {
    RhythmData* rd = &g_game.rhythm_data;
    // 防抖
    long long now = GetGameTimeMs();
    if (now - rd->p2_input_time < 60) return;
    rd->p2_input_time = now;

    rd->spirit_target_x = (track == TRACK_LEFT) ? (SCREEN_WIDTH / 4.0f) : (3.0f * SCREEN_WIDTH / 4.0f);
    rd->spirit_lerp_start = now;
    // 记录当前位置用于 Lerp 起点
    rd->spirit_x = GetSpiritX(now);
}

void AutoJudgeNotes(long long current_time) {
    RhythmData* rd = &g_game.rhythm_data;
    float spirit_cx = GetSpiritX(current_time);
    float fall_speed = FALLING_SPEED * rd->fall_speed_mult;

    for (int i = 0; i < rd->recorded_count; i++) {
        RhythmNote* note = &rd->recorded_sequence[i];
        if (note->is_handled) continue;

        long long note_abs_time = rd->bar_start_time + note->relative_timestamp;
        float note_y = GetNoteYPosition(note_abs_time, current_time, fall_speed);

        // 音符到达判定线附近
        if (note_y >= JUDGMENT_Y - 30.0f && note_y <= JUDGMENT_Y + 30.0f) {
            float note_x = (note->track == TRACK_LEFT) ? (SCREEN_WIDTH / 4.0f) : (3.0f * SCREEN_WIDTH / 4.0f);
            
            // 检查 Spirit 是否在正确轨道
            float dist_to_note = fabsf(spirit_cx - note_x);
            int on_correct_track = (dist_to_note < 60.0f); // 判定范围

            if (on_correct_track) {
                // 计算时间偏差
                long long timing_error = current_time - note_abs_time;
                if (timing_error < 0) timing_error = -timing_error;

                if (timing_error < PERFECT_WINDOW) {
                    ApplyJudgment(note, JUDGMENT_PERFECT, note_x, note_y, current_time);
                    // 轨道高亮
                    rd->track_glow_timer[note->track] = 20;
                } else if (timing_error < GOOD_WINDOW) {
                    ApplyJudgment(note, JUDGMENT_GOOD, note_x, note_y, current_time);
                    rd->track_glow_timer[note->track] = 12;
                } else {
                    ApplyJudgment(note, JUDGMENT_MISS, note_x, note_y, current_time);
                }
            }
            // 如果不在正确轨道，不做判定；等超时 Miss 逻辑处理
        }
    }
}

float GetSpiritX(long long current_time) {
    RhythmData* rd = &g_game.rhythm_data;
    long long elapsed = current_time - rd->spirit_lerp_start;
    if (elapsed >= rd->spirit_lerp_duration) {
        return rd->spirit_target_x;
    }
    float t = (float)elapsed / rd->spirit_lerp_duration;
    // ease-out 缓动
    float eased = 1.0f - (1.0f - t) * (1.0f - t);
    return rd->spirit_x + (rd->spirit_target_x - rd->spirit_x) * eased;
}

float GetSpiritY(long long current_time) {
    (void)current_time;
    RhythmData* rd = &g_game.rhythm_data;
    // 弹跳 + 待机浮动
    float bob = sinf(rd->spirit_bob_timer * 0.1047f) * 3.0f; // ±3px 浮动
    return rd->spirit_y + bob;
}

float GetNoteYPosition(long long note_timestamp, long long current_time, float falling_speed) {
    long long elapsed = current_time - note_timestamp;
    if (elapsed < 0) return -100;
    return (float)elapsed * falling_speed;
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
    for (int i = 0; i < rd->recorded_count; i++) {
        RhythmNote* note = &rd->recorded_sequence[i];
        if (!note->is_handled) {
            float note_x = (note->track == TRACK_LEFT) ? (SCREEN_WIDTH / 4.0f) : (3.0f * SCREEN_WIDTH / 4.0f);
            float note_y = GetNoteYPosition(rd->bar_start_time + note->relative_timestamp, now, FALLING_SPEED * rd->fall_speed_mult);
            ApplyJudgment(note, JUDGMENT_MISS, note_x, note_y > SCREEN_HEIGHT ? JUDGMENT_Y : note_y, now);
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
    }
    for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
        rd->score_popups[i].life = 0;
    }
    for (int i = 0; i < MAX_RHYTHM_PARTICLES; i++) {
        rd->rhythm_particles[i].life = 0;
    }
}
