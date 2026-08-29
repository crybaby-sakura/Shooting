// enemyPat_futonGaFuttonda.cpp
//
// 「布団が吹っ飛んだ」モチーフの弾幕パターン。
//
// 構成(無限ループ、1サイクル520フレーム):
//   1. 就寝    : 2枚の布団(赤白市松模様のグリッド)が中心から広がって敷かれる。Zzz演出付き。
//   2. そよ風  : 布団がさざ波状に揺れ、四隅から自機狙いの警告弾が飛ぶ。
//   3. 突風一閃: 布団がグリッド形状を保ったまま2回転しながら対角線上を加速で吹っ飛ぶ。
//                左右から出た2枚がすれ違う。
//   4. 綿吹雪  : 吹っ飛び終わった瞬間、布団を構成していた弾がその場から放射状に飛散し、
//                追い打ちの自機狙い3wayで締める。
//
// 位置計算は全てpShot->count(生成からの経過フレーム)を引数とした純粋な式で行い、
// 速度の逐次加算(velocity integration)は一切行わない。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ==================== フェーズ境界(各弾自身のcount基準) ====================
static const int T_FORM = 80;   // 布団の形成が完了するフレーム
static const int T_HOLD = 130;  // 静止(就寝)が終わるフレーム
static const int T_RIPPLE = 270;  // そよ風(さざ波)が終わるフレーム
static const int T_LAUNCH = 380;  // 突風での吹っ飛びが終わり、綿吹雪が始まるフレーム

// ==================== 布団グリッドの形状 ====================
static const int    GRID_COL = 14;
static const int    GRID_ROW = 6;
static const double GRID_SPACING_X = 22.0;
static const double GRID_SPACING_Y = 20.0;
static const double HALF_W = (GRID_COL - 1) / 2.0 * GRID_SPACING_X;
static const double HALF_H = (GRID_ROW - 1) / 2.0 * GRID_SPACING_Y;

// ==================== 布団A・Bの配置と吹っ飛び方向 ====================
static const double FUTON_A_X = 150.0, FUTON_A_Y = 150.0;
static const double FUTON_A_TRAVEL_X = 360.0, FUTON_A_TRAVEL_Y = 420.0; // 右下へ
static const double FUTON_B_X = 330.0, FUTON_B_Y = 150.0;
static const double FUTON_B_TRAVEL_X = -360.0, FUTON_B_TRAVEL_Y = 420.0; // 左下へ(Aとクロスする)

// 1サイクルの長さ(この後 count は巻き戻さずcycleStartだけ更新してループする)
static const int CYCLE_LENGTH = 520;

// ==================== イージング関数 ====================
static double Clamp01(double x)
{
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}
static double EaseOutCubic(double x)
{
    x = Clamp01(x);
    return 1.0 - pow(1.0 - x, 3.0);
}
static double EaseInQuad(double x)
{
    x = Clamp01(x);
    return x * x;
}

// ==================== 布団本体(グリッド弾) ====================
// pEnemyShotSet->x, y      : 敷き位置(アンカー開始座標)
// pEnemyShotSet->param_d[0]: 吹っ飛び方向 travelX
// pEnemyShotSet->param_d[1]: 吹っ飛び方向 travelY
// pEnemyShotSet->param_i[0]: 回転方向(+1 or -1)
static void ShotFutonBody(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int row = 0; row < GRID_ROW; row++) {
            for (int col = 0; col < GRID_COL; col++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                double targetLocalX = (col - (GRID_COL - 1) / 2.0) * GRID_SPACING_X;
                double targetLocalY = (row - (GRID_ROW - 1) / 2.0) * GRID_SPACING_Y;

                pEnemyShot->param_d[0] = targetLocalX;
                pEnemyShot->param_d[1] = targetLocalY;
                pEnemyShot->param_i[0] = col;                      // さざ波の位相計算に使用
                pEnemyShot->param_d[2] = 2.0 + GetRand(200) / 100.0; // 綿吹雪の飛散速度(2.0〜4.0)

                // 市松模様(赤・白)で布団っぽく
                int colorIdx = ((row + col) % 2 == 0) ? 0 : 6;
                pEnemyShot->kind = img_enemyShotMediumOval[colorIdx];

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->margin = 480;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    double travelX = pEnemyShotSet->param_d[0];
    double travelY = pEnemyShotSet->param_d[1];
    double rotDir = (double)pEnemyShotSet->param_i[0];

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int t = pShot->count;
        double targetLocalX = pShot->param_d[0];
        double targetLocalY = pShot->param_d[1];
        int    col = pShot->param_i[0];
        double burstSpeed = pShot->param_d[2];

        // ---- アンカー座標(布団全体の重心) ----
        double anchorX, anchorY;
        if (t < T_RIPPLE) {
            anchorX = pEnemyShotSet->x;
            anchorY = pEnemyShotSet->y;
        }
        else {
            double p = (t < T_LAUNCH)
                ? EaseInQuad((double)(t - T_RIPPLE) / (double)(T_LAUNCH - T_RIPPLE))
                : 1.0;
            anchorX = pEnemyShotSet->x + travelX * p;
            anchorY = pEnemyShotSet->y + travelY * p;
        }

        // ---- 重心から見た局所座標 ----
        double localX, localY, muki;

        if (t < T_FORM) {
            // 就寝フェーズ:中心から広がって敷かれる
            double p = EaseOutCubic((double)t / (double)T_FORM);
            localX = targetLocalX * p;
            localY = targetLocalY * p;
            muki = 0.0;
        }
        else if (t < T_HOLD) {
            // 静止(就寝キープ)
            localX = targetLocalX;
            localY = targetLocalY;
            muki = 0.0;
        }
        else if (t < T_RIPPLE) {
            // そよ風フェーズ:列ごとの位相でさざ波
            double p = (double)(t - T_HOLD) / (double)(T_RIPPLE - T_HOLD);
            double amp = 10.0 * p;
            double wave = amp * sin(col * 0.6 + t * 0.15);
            localX = targetLocalX;
            localY = targetLocalY + wave;
            muki = 0.0;
        }
        else if (t < T_LAUNCH) {
            // 突風フェーズ:形状を保ったまま2回転しながら加速移動
            double p = EaseInQuad((double)(t - T_RIPPLE) / (double)(T_LAUNCH - T_RIPPLE));
            double theta = rotDir * p * (2.0 * DX_PI * 2.0); // 2回転
            localX = targetLocalX * cos(theta) - targetLocalY * sin(theta);
            localY = targetLocalX * sin(theta) + targetLocalY * cos(theta);
            muki = theta;
        }
        else {
            // 綿吹雪フェーズ:ちょうど2回転(4π)で元の向きに戻っているため、
            // targetLocalX/Yをそのまま飛散方向として再利用できる。
            double len = sqrt(targetLocalX * targetLocalX + targetLocalY * targetLocalY);
            double nx = (len > 0.0001) ? targetLocalX / len : 1.0;
            double ny = (len > 0.0001) ? targetLocalY / len : 0.0;
            double burstElapsed = (double)(t - T_LAUNCH);
            localX = targetLocalX + nx * burstSpeed * burstElapsed;
            localY = targetLocalY + ny * burstSpeed * burstElapsed;
            muki = atan2(ny, nx);
        }

        pShot->x = anchorX + localX;
        pShot->y = anchorY + localY;
        pShot->muki = muki;

        pShot = pShot->next;
    }
}

// ==================== 就寝中のZzz演出 ====================
static void ShotZzz(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->param_d[0] = pEnemyShotSet->x + (i - 1) * 6.0; // spawnX
            pEnemyShot->param_d[1] = pEnemyShotSet->y;                 // spawnY
            pEnemyShot->muki = -DX_PI / 2.0 + (GetRand(50) - 25) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.5 + GetRand(30) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白

            pEnemyShot->x = pEnemyShot->param_d[0];
            pEnemyShot->y = pEnemyShot->param_d[1];
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->speed * cos(pShot->muki) * t;
        pShot->y = pShot->param_d[1] + pShot->speed * sin(pShot->muki) * t;
        pShot = pShot->next;
    }
}

// ==================== そよ風フェーズ:四隅からの自機狙い警告弾 ====================
static void ShotCornerWarn(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->param_d[0] = pEnemyShotSet->x;
        pEnemyShot->param_d[1] = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki; // 生成側で計算済みの自機狙い角度
        pEnemyShot->speed = 2.2;
        pEnemyShot->kind = img_enemyShotBullet[8]; // 橙

        pEnemyShot->x = pEnemyShot->param_d[0];
        pEnemyShot->y = pEnemyShot->param_d[1];
        pEnemyShot->margin = 480;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->speed * cos(pShot->muki) * t;
        pShot->y = pShot->param_d[1] + pShot->speed * sin(pShot->muki) * t;
        pShot = pShot->next;
    }
}

// ==================== 綿吹雪の締め:追い打ち自機狙い3way ====================
static void ShotFinalBurst3Way(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        static const double spreadAngles[3] = { -0.18, 0.0, 0.18 }; // ラジアン係数(×DX_PI)
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + spreadAngles[i] * DX_PI;
            pEnemyShot->speed = 2.6;
            pEnemyShot->kind = img_enemyShotDiamond[8]; // 橙

            pEnemyShot->x = pEnemyShot->param_d[0];
            pEnemyShot->y = pEnemyShot->param_d[1];
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->speed * cos(pShot->muki) * t;
        pShot->y = pShot->param_d[1] + pShot->speed * sin(pShot->muki) * t;
        pShot = pShot->next;
    }
}

// ==================== ShotSet生成用の共通ヘルパー ====================
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    return pEnemyShotSet;
}

// ==================== 敵本体のパターン ====================
void EnemyPat_FutonFlewAway_Claude()
{
    static int cycleStart = 1;
    static int cornerCounter = 0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
        cycleStart = 1;
        cornerCounter = 0;
    }
    else {
        // 布団を見守る主(あるじ)のゆったりとした横揺れ
        enemy.x = 240.0 + 16.0 * sin((count - cycleStart) * 0.015);
    }

    int t = count - cycleStart + 1;
    if (t > CYCLE_LENGTH) {
        cycleStart = count;
        t = 1;
    }

    // ---- 布団A(左寄りに出現、右下へ吹っ飛ぶ) ----
    if (t == 1) {
        sEnemyShotSet* pSet = CreateShotSet(ShotFutonBody, FUTON_A_X, FUTON_A_Y);
        pSet->param_d[0] = FUTON_A_TRAVEL_X;
        pSet->param_d[1] = FUTON_A_TRAVEL_Y;
        pSet->param_i[0] = 1; // 時計回り
    }

    // ---- 布団B(右寄りに少し遅れて出現、左下へ吹っ飛びAとクロスする) ----
    if (t == 70) {
        sEnemyShotSet* pSet = CreateShotSet(ShotFutonBody, FUTON_B_X, FUTON_B_Y);
        pSet->param_d[0] = FUTON_B_TRAVEL_X;
        pSet->param_d[1] = FUTON_B_TRAVEL_Y;
        pSet->param_i[0] = -1; // 反時計回り
    }

    // ---- 就寝中のZzz演出 ----
    if (t == 20 || t == 45 || t == 105) {
        CreateShotSet(ShotZzz, enemy.x, enemy.y - 30.0);
    }

    // ---- そよ風フェーズ:四隅からの自機狙い警告弾(布団A・Bの角を交互に) ----
    if (t >= 140 && t < 260 && t % 24 == 0) {
        static const double cornerOffsetX[4] = { -HALF_W,  HALF_W, -HALF_W,  HALF_W };
        static const double cornerOffsetY[4] = { -HALF_H, -HALF_H,  HALF_H,  HALF_H };

        bool useFutonA = (cornerCounter % 2 == 0);
        int  cornerIdx = (cornerCounter / 2) % 4;
        double baseX = useFutonA ? FUTON_A_X : FUTON_B_X;
        double baseY = useFutonA ? FUTON_A_Y : FUTON_B_Y;
        double cx = baseX + cornerOffsetX[cornerIdx];
        double cy = baseY + cornerOffsetY[cornerIdx];

        sEnemyShotSet* pSet = CreateShotSet(ShotCornerWarn, cx, cy);
        pSet->muki = atan2(player.y - cy, player.x - cx);

        cornerCounter++;
    }

    // ---- 布団Aの綿吹雪:追い打ち自機狙い3way ----
    if (t == T_LAUNCH + 1) {
        double x = FUTON_A_X + FUTON_A_TRAVEL_X;
        double y = FUTON_A_Y + FUTON_A_TRAVEL_Y;
        sEnemyShotSet* pSet = CreateShotSet(ShotFinalBurst3Way, x, y);
        pSet->muki = atan2(player.y - y, player.x - x);
    }

    // ---- 布団Bの綿吹雪:追い打ち自機狙い3way ----
    if (t == 70 + T_LAUNCH + 1) {
        double x = FUTON_B_X + FUTON_B_TRAVEL_X;
        double y = FUTON_B_Y + FUTON_B_TRAVEL_Y;
        sEnemyShotSet* pSet = CreateShotSet(ShotFinalBurst3Way, x, y);
        pSet->muki = atan2(player.y - y, player.x - x);
    }
}