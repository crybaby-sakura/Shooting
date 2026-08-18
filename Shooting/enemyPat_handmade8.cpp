// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "MidiFile.h"
#include <math.h>
#include <vector>
#include <algorithm>

// ----------------------------------------------------------------
// 17msフレーム駆動および音階設定
// ----------------------------------------------------------------
static const double FRAME_TIME_SEC = 0.017;  // 1フレーム = 17ms
static const double SYNC_OFFSET_SEC = 0.020; // 遅延補正（秒）

static const int minNote = 21;  // ピアノ88鍵盤の最低音 (A0) に変更
static const int maxNote = 108; // ピアノ88鍵盤の最高音 (C8) に変更

// 画面配置・移動速度の定数定義
static const double KEY_Y = 440.0;           // 鍵盤のY座標（画面下側）
static const double SPAWN_Y = 0.0;           // ノーツ(弾)の発射Y座標（画面上側） ※0に変更
static const double X_MIN = 10.0;            // 鍵盤の左端 ※10に変更
static const double X_MAX = 470.0;           // 鍵盤の右端 ※470に変更
static const double FALL_SPEED = 2.5;        // ノーツ(弾)の落下速度
// 到達にかかる時間を秒に換算（前倒し発射用）
static const double FALL_TIME_SEC = ((KEY_Y - SPAWN_Y) / FALL_SPEED) * FRAME_TIME_SEC;

static int g_bgmHandle = -1;

// ドレミファソラシ (C, D, E, F, G, A, B) に対応する 0〜6 のカラーインデックス変換表
// ※ばら撒き弾の色分けには引き続き使用
static const int noteToColor[12] = { 0, 0, 8, 8, 1, 2, 2, 3, 3, 4, 4, 5 };

struct MidiNoteOnEvent {
    double seconds;
    int note;
    int velocity;
};

static std::vector<MidiNoteOnEvent> g_midiNoteEvents;
static size_t g_eventIndex = 0;
static bool g_midiLoaded = false;

// ----------------------------------------------------------------
// 移動処理関数群
// ----------------------------------------------------------------

// 全弾共通の直線移動処理関数
static void ShotMoveLinear(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 鍵盤用の移動停止処理関数（位置固定）
static void ShotMoveLaserKey(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int n = minNote; n <= maxNote; ++n) {
            double norm = (double)(n - minNote) / (double)(maxNote - minNote);
            double spawnX = X_MIN + norm * (X_MAX - X_MIN);
            int pitchClass = n % 12;

            // 黒鍵(#付き)判定: C#, D#, F#, G#, A#
            bool isBlackKey = (pitchClass == 1 || pitchClass == 3 || pitchClass == 6 || pitchClass == 8 || pitchClass == 10);

            for (int i = 0; i < 2; i++) {
                sEnemyShot* pKey = new sEnemyShot;
                pKey->x = spawnX;
                pKey->y = KEY_Y + i * 10;
                pKey->muki = DX_PI / 2.0;  // 見栄えのため真下に設定
                pKey->speed = 0.0;         // 鍵盤として配置するため速度0

                if (isBlackKey && i == 0) {
                    pKey->kind = img_enemyShotBullet[7];

                    pKey->prev = pSet->pEnemyShotHead->prev;
                    pKey->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pKey;
                    pSet->pEnemyShotHead->prev = pKey;
                }
                else {
                    pKey->kind = img_enemyShotBullet[6];

                    pKey->next = pSet->pEnemyShotHead->next;
                    pKey->prev = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->next->prev = pKey;
                    pSet->pEnemyShotHead->next = pKey;
                }
            }
        }
    }

    // 鍵盤は移動させず初期位置に固定
}

// 鍵盤が光る演出用の移動関数（数フレーム後に画面外へ飛ばして消去）
static void ShotMoveKeyFlash(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        int pitchClass = pSet->param_i[0];
        bool isBlackKey = (pitchClass == 1 || pitchClass == 3 ||
            pitchClass == 6 || pitchClass == 8 || pitchClass == 10);

        sEnemyShot* pFlash = new sEnemyShot;
        pFlash->x = pSet->x;
        pFlash->y = pSet->y;
        pFlash->muki = DX_PI / 2.0;
        pFlash->speed = 0.0;
        int colorIdx = noteToColor[pitchClass];
        pFlash->kind = img_enemyShotBullet[colorIdx];
        pFlash->count = 0;

        pFlash->prev = pSet->pEnemyShotHead->prev;
        pFlash->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pFlash;
        pSet->pEnemyShotHead->prev = pFlash;

        if (!isBlackKey) {
            sEnemyShot* pFlash = new sEnemyShot;
            pFlash->x = pSet->x;
            pFlash->y = pSet->y + 10;
            pFlash->muki = DX_PI / 2.0;
            pFlash->speed = 0.0;
            int colorIdx = noteToColor[pitchClass];
            pFlash->kind = img_enemyShotBullet[colorIdx];
            pFlash->count = 0;

            pFlash->prev = pSet->pEnemyShotHead->prev;
            pFlash->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pFlash;
            pSet->pEnemyShotHead->prev = pFlash;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->count++;
        if (pShot->count > 30) { // 30フレームで消去
            pShot->y = 9999.0;  // 画面外に飛ばしエンジンの自動削除に任せる
        }
        pShot = pShot->next;
    }
}

// ----------------------------------------------------------------
// 到達時のイベント（音、光演出、ばら撒き弾）を発生させる関数
// ----------------------------------------------------------------
static void TriggerHitEffect(double x, double y, int note)
{
    // 1. 到達タイミングで音を鳴らす
    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

    int pitchClass = note % 12;

    // 2. 鍵盤が光る演出（短い寿命の静止弾を生成）
    //    白鍵／黒鍵に対応した enemyShotBullet を使用
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotMoveKeyFlash;
        pSet->x = x;
        pSet->y = y;
        pSet->param_i[0] = pitchClass;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 3. 上向きにばら撒き弾を射出（1発だけ）
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotMoveLinear; // 直線移動
        pSet->x = x;
        pSet->y = y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 必ず1発だけ発射
        const int way = 1;
        for (int i = 0; i < way; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = x;
            pShot->y = y;

            // 真上(-90度)を中心に、左右にランダムに散らす（ただし1発なので適度なバリエーションを出す）
            // 必要に応じて完全に真上に固定してもよい
            double angleOfs = (GetRand(100) - 50) / 50.0 * (DX_PI / 2.0);
            pShot->muki = -DX_PI / 2.0 + angleOfs;

            // 速度を遅めに設定（0.5 ～ 1.5）
            pShot->speed = 0.5 + (GetRand(50) / 100.0);

            // 色付きの小玉（従来通り）
            int colorIdx = noteToColor[pitchClass];
            pShot->kind = img_enemyShotSmallBall[colorIdx];

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}

// 降ってくるノーツ用移動処理（鍵盤ライン到達判定付き）
static void ShotMoveFallingNote(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 鍵盤ライン(KEY_Y)に到達した瞬間にアクション実行
        if (pShot->y >= KEY_Y) {
            // ノート番号をX座標から逆算して色などを決定
            double norm = (pShot->x - X_MIN) / (X_MAX - X_MIN);
            if (norm < 0.0) norm = 0.0;
            if (norm > 1.0) norm = 1.0;
            int note = minNote + (int)(norm * (maxNote - minNote) + 0.5); // 四捨五入

            // ヒット演出の発動
            TriggerHitEffect(pShot->x, KEY_Y, note);

            // 到達したノーツ弾は画面外へ飛ばして消去する
            pShot->y = 9999.0;
        }

        pShot = pShot->next;
    }
}

// ----------------------------------------------------------------
// 画面下部にピアノの鍵盤を生成・配置（白黒の enemyShotBullet を使用）
// ----------------------------------------------------------------
static void SpawnPianoKeysLaser()
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotMoveLaserKey; // 位置固定

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ----------------------------------------------------------------
// [ピアノロール弾幕] ノーツ(弾)の発射
// ----------------------------------------------------------------
static void SpawnPianoRollShot(int note, int velocity)
{
    // ※発射時の音は鳴らさず、到達時(TriggerHitEffect)に鳴らす

    // 音階(note)を画面横幅 (X_MIN ～ X_MAX) にマッピング
    double norm = (double)(note - minNote) / (double)(maxNote - minNote);
    if (norm < 0.0) norm = 0.0;
    if (norm > 1.0) norm = 1.0;

    double spawnX = X_MIN + norm * (X_MAX - X_MIN);
    double spawnY = SPAWN_Y; // 画面上部から発射

    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotMoveFallingNote; // 到達判定付きの移動関数を使用
    pSet->x = spawnX;
    pSet->y = spawnY;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = spawnX;
    pShot->y = spawnY;
    pShot->muki = DX_PI / 2.0; // 真下 (90度)に降る
    pShot->speed = FALL_SPEED; // 一定速度

    // ドレミファソラシの7色インデックス変換（ノーツ本体の色は中楕円弾のカラーをそのまま利用）
    int pitchClass = note % 12;
    int colorIdx = noteToColor[pitchClass];
    pShot->kind = img_enemyShotMediumOval[colorIdx];

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ----------------------------------------------------------------
// MIDI解析処理
// ----------------------------------------------------------------
static void LoadMidiEvents()
{
    g_midiNoteEvents.clear();
    g_eventIndex = 0;

    smf::MidiFile midifile;
    if (!midifile.read("AI_work/oldBGM/Circus Galop.mid")) {
        g_midiLoaded = false;
        return;
    }

    midifile.doTimeAnalysis();
    midifile.linkNotePairs();

    for (int track = 0; track < midifile.getTrackCount(); ++track) {
        for (int event = 0; event < midifile.getEventCount(track); ++event) {
            const smf::MidiEvent& mev = midifile[track][event];

            if (mev.isNoteOn() && mev.getVelocity() > 0) {
                MidiNoteOnEvent e;
                e.seconds = mev.seconds;
                e.note = mev.getKeyNumber();
                e.velocity = mev.getVelocity();
                g_midiNoteEvents.push_back(e);
            }
        }
    }

    std::sort(g_midiNoteEvents.begin(), g_midiNoteEvents.end(),
        [](const MidiNoteOnEvent& a, const MidiNoteOnEvent& b) {
        return a.seconds < b.seconds;
    });

    g_midiLoaded = true;
}

// ----------------------------------------------------------------
// 現在時刻取得（17ms精度 / BGMハンドル連動）
// ----------------------------------------------------------------
static double GetCurrentBgmTimeInSeconds()
{
    if (g_bgmHandle >= 0 && CheckSoundMem(g_bgmHandle) == 1) {
        LONGLONG currentMs = GetSoundCurrentTime(g_bgmHandle);
        if (currentMs >= 0) {
            return (double)currentMs / 1000.0;
        }
    }
    return count * FRAME_TIME_SEC;
}

// ----------------------------------------------------------------
// 敵本体のメイン制御関数 (関数名を変更)
// ----------------------------------------------------------------
void EnemyPat_CircusGalop()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 9999;
        muki = 1;

        // BGMハンドルの初期化
        g_bgmHandle = currentBGMHandle;

        // 鍵盤用弾の生成・配置 (画面下部、白黒bullet)
        SpawnPianoKeysLaser();

        LoadMidiEvents();
    }
    else {
        enemy.x += 2.0 * (double)muki;
        if (enemy.x > 380.0) { enemy.x = 380.0; muki = -1; }
        else if (enemy.x < 100.0) { enemy.x = 100.0; muki = 1; }
    }
    enemy.hp--;

    // ------------------------------------------------------------
    // 17ms精度同期・ピアノロール弾幕処理
    // ------------------------------------------------------------
    if (g_midiLoaded && !g_midiNoteEvents.empty()) {

        double currentBgmTime = GetCurrentBgmTimeInSeconds() + SYNC_OFFSET_SEC;
        // BGMの音のタイミングより、弾が落下する時間分だけ前倒し（ルックアヘッド）で弾を発射する
        double currentLookAheadTime = currentBgmTime + FALL_TIME_SEC;

        while (g_eventIndex < g_midiNoteEvents.size() &&
            g_midiNoteEvents[g_eventIndex].seconds <= currentLookAheadTime)
        {
            const auto& noteEv = g_midiNoteEvents[g_eventIndex];

            // ピアノロール中楕円弾を発射（この時点ではまだ音は鳴らしません）
            SpawnPianoRollShot(noteEv.note, noteEv.velocity);

            g_eventIndex++;
        }
    }
}