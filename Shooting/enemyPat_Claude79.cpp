// enemyPat_IsouTsuruMonyou.cpp
// 位相蔓紋 -Lissajous-
//
// リサジュー曲線 x = AX・sin(a・θ+δ), y = AY・sin(b・θ) をモチーフにした4フェーズパターン。
// 1. 描筆   : θ=0→2πの軌跡上に弾を1発ずつ配置し、曲線を描き上げる
// 2. 定常周回: 完成した曲線上を全弾が一定角速度で流れ続け、5つの節点から自機狙い3wayを周期発射
// 3. 位相偏移: 位相差δをcosで滑らかに揺らし、曲線がハーモノグラフのように呼吸する
// 4. 遷移放散: 周波数比a:bを3:2→4:3→5:4へ連続的にモーフィングさせながら密度を上げ、
//              最終形で静止・予告点滅した後、全弾が中心から放射状に加速飛散して終幕
//
// count, pEnemyShotSet->count, pEnemyShot->count のインクリメントおよび画面外弾の削除は
// メインルーチン側の仕様のため、本ファイル内では一切行わない。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 曲線パラメータ
// ------------------------------------------------------------
static const double CX = 240.0;          // 曲線中心X
static const double CY_ = 260.0;          // 曲線中心Y
static const double AX = 240.0;          // X振幅
static const double AY = 220.0;          // Y振幅

static const int    NB = 200;                // 曲線を構成する弾の総数
static const int    NUM_MARKERS = 5;                   // 節点(自機狙い発射点)の数
static const int    MARKER_SPACING = NB / NUM_MARKERS;    // 節点の間隔(インデックス)

static const int    FORMATION_FRAMES = 220;   // 描筆フェーズの長さ
static const int    SHIFT_START = 500;   // 位相偏移(δ揺らぎ)が始まるタイミング
static const double RAMP_FRAMES = 60.0;  // δ揺らぎのイーズイン時間
static const double DELTA_AMP = 0.6;   // δ揺らぎの振幅
static const double DELTA_PERIOD = 240.0; // δ揺らぎの周期

static const int    FREQ_START = 780;  // 周波数比モーフィング開始 (3:2→4:3)
static const int    FREQ_MID = 930;  // (4:3→5:4)
static const int    FREQ_END = 1080; // モーフィング完了・予告点滅開始
static const int    BURST_START = 1120; // 崩壊放散開始

static const double FLOW_OMEGA = 2.0 * DX_PI / 900.0 / 2; // 曲線上を流れる角速度(定常周回)

static const int    EMIT_START = 300;
static const int    EMIT_INTERVAL = 110;

static const double NODE_SHOT_SPEED = 2.6;
static const double FINALE_SHOT_SPEED = 3.4;
static const double FLEE_SPEED = 2.2;
static const double FLEE_ACCEL = 0.012;

// 虹色パレット(白・黒は予告点滅/フィナーレ用に温存)
static const int RAINBOW[7] = { 0, 8, 1, 2, 3, 4, 5 };

// ------------------------------------------------------------
// a(t), b(t) : 周波数比の連続モーフィング
// ------------------------------------------------------------
static void ComputeAB(double t, double& a, double& b)
{
    if (t < FREQ_START) {
        a = 3.0; b = 2.0;
    }
    else if (t < FREQ_MID) {
        double r = (t - FREQ_START) / (double)(FREQ_MID - FREQ_START);
        a = 3.0 + 1.0 * r; b = 2.0 + 1.0 * r;
    }
    else if (t < FREQ_END) {
        double r = (t - FREQ_MID) / (double)(FREQ_END - FREQ_MID);
        a = 4.0 + 1.0 * r; b = 3.0 + 1.0 * r;
    }
    else {
        a = 5.0; b = 4.0;
    }
}

// δ(t) : 位相偏移の揺らぎ
static double ComputeDelta(double t)
{
    if (t < SHIFT_START) return 0.0;
    double ramp = (t - SHIFT_START) / RAMP_FRAMES;
    if (ramp > 1.0) ramp = 1.0;
    return DELTA_AMP * ramp * cos(2.0 * DX_PI * (t - SHIFT_START) / DELTA_PERIOD);
}

// ------------------------------------------------------------
// 自機狙いの直進弾を1発生成して pEnemyShotSet のリストへ連結する
// ------------------------------------------------------------
static void SpawnAimedShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[0] = 1;   // 種別: 自機狙い直進弾
    pEnemyShot->param_d[2] = x;   // 発射時座標(位置計算の基準点)
    pEnemyShot->param_d[3] = y;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// ------------------------------------------------------------
// 弾幕：位相蔓紋
// ------------------------------------------------------------
static void ShotLissajous(sEnemyShotSet* pEnemyShotSet)
{
    double t = (double)pEnemyShotSet->count;
    double tClamped = (t > FREQ_END) ? (double)FREQ_END : t; // 収束後は形状を凍結

    // --- 描筆フェーズ: 曲線上の弾を段階的に配置 ---
    if (pEnemyShotSet->param_i[0] < NB) {
        if (pEnemyShotSet->count == 0) {
            // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        int target = (int)(t * NB / (double)FORMATION_FRAMES);
        if (target > NB) target = NB;

        while (pEnemyShotSet->param_i[0] < target) {
            int index = pEnemyShotSet->param_i[0];

            sEnemyShot* pEnemyShot = new sEnemyShot;
            int colorIdx = RAINBOW[index % 7];
            // 弾の種類一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
            // 弾の色一覧:   0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];
            pEnemyShot->x = CX;
            pEnemyShot->y = CY_;
            pEnemyShot->param_i[0] = 0;                // 種別: 曲線上の点
            pEnemyShot->param_i[1] = pEnemyShot->kind;  // 元の色(予告点滅の復元用)
            pEnemyShot->param_i[2] = index;             // 曲線上でのインデックス
            pEnemyShot->param_d[0] = 2.0 * DX_PI * index / (double)NB; // θの基準値
            pEnemyShot->margin = 100;
            if (pEnemyShot->param_i[2] % MARKER_SPACING == 0) pEnemyShot->kind = img_enemyShotMediumBall[colorIdx];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            pEnemyShotSet->param_i[0]++;
        }
    }

    // --- 予告点滅の開始音 ---
    if (pEnemyShotSet->count == FREQ_END) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --- 崩壊放散フェーズへの移行(1回だけ実行) ---
    if (pEnemyShotSet->count == BURST_START) {
        double a, b;
        ComputeAB(tClamped, a, b);
        double delta = ComputeDelta(tClamped);
        double flow = FLOW_OMEGA * tClamped;

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                double theta = pShot->param_d[0] + flow;
                double fx = CX + AX * sin(a * theta + delta);
                double fy = CY_ + AY * sin(b * theta);

                pShot->param_i[0] = 3; // 種別: 崩壊放散する曲線片
                pShot->param_d[2] = fx;
                pShot->param_d[3] = fy;
                pShot->param_d[4] = atan2(fy - CY_, fx - CX);
                pShot->kind = pShot->param_i[1]; // 元の色に戻す
                pShot->x = fx;
                pShot->y = fy;
            }
            pShot = pShot->next;
        }

        // 中心から自機狙い3wayの大玉フィナーレ
        double baseMuki = atan2(player.y - CY_, player.x - CX);
        SpawnAimedShot(pEnemyShotSet, CX, CY_, baseMuki - 0.2, FINALE_SHOT_SPEED, img_enemyShotLargeBall[6]);
        SpawnAimedShot(pEnemyShotSet, CX, CY_, baseMuki, FINALE_SHOT_SPEED, img_enemyShotLargeBall[6]);
        SpawnAimedShot(pEnemyShotSet, CX, CY_, baseMuki + 0.2, FINALE_SHOT_SPEED, img_enemyShotLargeBall[6]);

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // --- 節点からの自機狙い3way(定常周回〜遷移放散中に周期発射) ---
    bool doEmit = (t >= EMIT_START) && (t < FREQ_END)
        && (((int)t - EMIT_START) % EMIT_INTERVAL == 0);

    if (doEmit) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // --- 毎フレームの位置更新 ---
    double a, b;
    ComputeAB(tClamped, a, b);
    double delta = ComputeDelta(tClamped);
    double flow = FLOW_OMEGA * tClamped;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 曲線上の点: θ = 基準値 + 流れオフセット
            double theta = pShot->param_d[0] + flow;
            double x = CX + AX * sin(a * theta + delta);
            double y = CY_ + AY * sin(b * theta);
            pShot->x = x;
            pShot->y = y;

            // 予告点滅(周波数モーフィング完了後、崩壊放散直前)
            if (t >= FREQ_END && t < BURST_START) {
                int blink = ((int)(t - FREQ_END) / 6) % 2;
                pShot->kind = (blink == 0) ? img_enemyShotSmallBall[6] : pShot->param_i[1];
            }

            // 節点(マーカー)からの自機狙い3way
            if (doEmit && (pShot->param_i[2] % MARKER_SPACING == 0)) {
                double baseMuki = atan2(player.y - y, player.x - x);
                SpawnAimedShot(pEnemyShotSet, x, y, baseMuki - 0.18, NODE_SHOT_SPEED, img_enemyShotBullet[6]);
                SpawnAimedShot(pEnemyShotSet, x, y, baseMuki, NODE_SHOT_SPEED, img_enemyShotBullet[6]);
                SpawnAimedShot(pEnemyShotSet, x, y, baseMuki + 0.18, NODE_SHOT_SPEED, img_enemyShotBullet[6]);
            }
        }
        else if (pShot->param_i[0] == 1) {
            // 自機狙い直進弾: 発射座標 + 速度×経過フレームで直線移動(速度積分を行わない)
            double n = (double)pShot->count;
            pShot->x = pShot->param_d[2] + pShot->speed * cos(pShot->muki) * n;
            pShot->y = pShot->param_d[3] + pShot->speed * sin(pShot->muki) * n;
        }
        else if (pShot->param_i[0] == 3) {
            // 崩壊放散する曲線片: 凍結位置から外向きに加速しながら飛散
            double n = (double)pEnemyShotSet->count - BURST_START;
            if (n < 0.0) n = 0.0;
            double dist = FLEE_SPEED * n + 0.5 * FLEE_ACCEL * n * n;
            pShot->x = pShot->param_d[2] + dist * cos(pShot->param_d[4]);
            pShot->y = pShot->param_d[3] + dist * sin(pShot->param_d[4]);
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Lissajous_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    else {
        // 曲線の演出を主役にするため、本体はゆるやかに上下へ揺れるのみ
        enemy.y = 60.0 + 8.0 * sin(count / 90.0);
    }

    if (count % 1200 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLissajous;
        pEnemyShotSet->x = CX;
        pEnemyShotSet->y = CY_;
        pEnemyShotSet->param_i[0] = 0; // 描筆済み弾数のカウンタ

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}