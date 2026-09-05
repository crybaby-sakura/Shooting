// enemyPat_SaikiRitsudou.cpp
// 再帰律動 -ヒルベルト-
// ヒルベルト曲線をモチーフにした4フェーズパターン
//   1. 敷設     : 次数2(4x4/16点)の曲線を1点ずつ配置して描き上げる
//   2. 定常流走 : 曲線上を全弾が往復しながら流れ、角(方向転換点)から
//                 絶え間なく自機狙い3wayを連射する
//   3. 再帰変容 : 各点が4分裂し、次数3(8x8/64点)の曲線へイーズ変形する
//                 (ヒルベルト曲線の自己相似性をそのまま密度増加として表現)
//   4. 崩壊     : 完成形が点滅予告した後、全弾がその場から放射状に加速飛散し
//                 自機狙い5wayフィニッシュが放たれる

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//======================================================================
// フェーズ境界・パラメータ（グローバル count 基準）
//======================================================================
static const int    SPAWN_INTERVAL = 5;                            // 敷設: 1点あたりの間隔
static const int    PHASE1_END = 1 + 15 * SPAWN_INTERVAL + 10; // 敷設フェーズ終了(=86)
static const double FLOW_SPEED = 0.1;                          // 流走: 経路インデックス/フレーム
static const int    PHASE2_LEN = 300;                          // 流走フェーズ長 (flowSpeed*len=30=ちょうど1往復)
static const int    PHASE2_END = PHASE1_END + PHASE2_LEN;      // 流走フェーズ終了
static const int    PHASE3_LEN = 100;                          // 再帰変容(次数上昇)の長さ
static const int    PHASE3_END = PHASE2_END + PHASE3_LEN;      // 再帰変容フェーズ終了
static const int    PHASE3_HOLD_LEN = 60;                           // 次数3静止・予告の長さ
static const int    PHASE3_HOLD_END = PHASE3_END + PHASE3_HOLD_LEN; // 静止予告フェーズ終了
static const int    PHASE4_START = PHASE3_HOLD_END + 1;          // 崩壊フェーズ開始

static const double BURST_BASE_SPEED = 1.2;   // 崩壊フェーズの初速
static const double BURST_ACCEL = 0.010; // 崩壊フェーズの加速度

static const int    CORNER_PULSE_INTERVAL = 20; // 角からの自機狙い3wayの周期
static const int    CORNER_PULSE_STAGGER = 3;  // 角ごとの発射タイミングのずらし幅

// エリア設定 (ゲーム画面 480x480 の中央付近に 300x300 の正方形領域を確保)
static const double AREA_LEFT = 90.0;
static const double AREA_TOP = 110.0;
static const double AREA_SIZE = 300.0;
static const double CELL2 = AREA_SIZE / 4.0; // 次数2: 4x4 マス
static const double CELL3 = AREA_SIZE / 8.0; // 次数3: 8x8 マス
static const double AREA_CX = AREA_LEFT + AREA_SIZE / 2.0;
static const double AREA_CY = AREA_TOP + AREA_SIZE / 2.0;

//======================================================================
// ヒルベルト曲線 座標テーブル
//======================================================================
static double g_h2x[16], g_h2y[16], g_muki2[16];
static bool   g_isCorner2[16];
static int    g_cornerList[16], g_cornerCount = 0;
static double g_h3x[64], g_h3y[64];
static bool   g_tableInit = false;

const int T = 650;
static int countT;

// d(0 ~ 4^order - 1) を 一辺 2^order マスのグリッド座標(x,y)へ変換する標準アルゴリズム
static void HilbertD2XY(int order, int d, int* px, int* py)
{
    int rx, ry, t = d;
    *px = *py = 0;
    for (int s = 1; s < (1 << order); s <<= 1) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);
        if (ry == 0) {
            if (rx == 1) {
                *px = s - 1 - *px;
                *py = s - 1 - *py;
            }
            int tmp = *px; *px = *py; *py = tmp;
        }
        *px += s * rx;
        *py += s * ry;
        t >>= 2;
    }
}

// 次数2/次数3の座標テーブルと、次数2曲線の角(方向転換点)を初回のみ計算しておく
// ※次数3の点 4*i, 4*i+1, 4*i+2, 4*i+3 は必ず次数2の点iのセル内に収まる
//   (ヒルベルト曲線の自己相似構造による)ので、フェーズ3の分裂演出にそのまま使える
static void InitHilbertTables()
{
    if (g_tableInit) return;

    for (int d = 0; d < 16; d++) {
        int gx, gy;
        HilbertD2XY(2, d, &gx, &gy);
        g_h2x[d] = AREA_LEFT + (gx + 0.5) * CELL2;
        g_h2y[d] = AREA_TOP + (gy + 0.5) * CELL2;
    }
    for (int d = 0; d < 64; d++) {
        int gx, gy;
        HilbertD2XY(3, d, &gx, &gy);
        g_h3x[d] = AREA_LEFT + (gx + 0.5) * CELL3;
        g_h3y[d] = AREA_TOP + (gy + 0.5) * CELL3;
    }

    g_cornerCount = 0;
    for (int i = 0; i < 16; i++) {
        if (i == 0) {
            g_muki2[i] = atan2(g_h2y[1] - g_h2y[0], g_h2x[1] - g_h2x[0]);
            g_isCorner2[i] = false;
            continue;
        }
        if (i == 15) {
            g_muki2[i] = atan2(g_h2y[15] - g_h2y[14], g_h2x[15] - g_h2x[14]);
            g_isCorner2[i] = false;
            continue;
        }
        double dx1 = g_h2x[i] - g_h2x[i - 1], dy1 = g_h2y[i] - g_h2y[i - 1];
        double dx2 = g_h2x[i + 1] - g_h2x[i], dy2 = g_h2y[i + 1] - g_h2y[i];
        g_muki2[i] = atan2(dy2, dx2);
        if (fabs(dx1 * dy2 - dy1 * dx2) > 0.1) {
            g_isCorner2[i] = true;
            g_cornerList[g_cornerCount++] = i;
        }
        else {
            g_isCorner2[i] = false;
        }
    }

    g_tableInit = true;
}

// 流走フェーズ: 経路0~15を往復(ピンポン)しながら移動する位置を返す
// flowSpeed*PHASE2_LEN がちょうど30(=1往復分)になるよう調整してあるので、
// フェーズ終了時には全弾が必ず自分のホーム位置(次数2の元の点)へ戻る
static void HilbertFlowPos(int homeIdx, int t, double* outX, double* outY)
{
    double flowT = (double)(t - PHASE1_END) * FLOW_SPEED;
    double phase = fmod((double)homeIdx + flowT, 30.0);
    if (phase < 0.0) phase += 30.0;
    double idxF = (phase > 15.0) ? (30.0 - phase) : phase;
    int i0 = (int)idxF;
    if (i0 < 0)  i0 = 0;
    if (i0 > 14) i0 = 14;
    int i1 = i0 + 1;
    double frac = idxF - i0;
    *outX = g_h2x[i0] + (g_h2x[i1] - g_h2x[i0]) * frac;
    *outY = g_h2y[i0] + (g_h2y[i1] - g_h2y[i0]) * frac;
}

static double EaseInOut(double t)
{
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return t * t * (3.0 - 2.0 * t);
}

//======================================================================
// 汎用ShotSet生成ヘルパ
//======================================================================
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = 0.0;
    pSet->kind = 0;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

//======================================================================
// 曲線本体の弾幕 (敷設→流走→再帰変容→崩壊の全フェーズを1つのShotSetで管理)
//   param_i[0] : 現在(または目標)の次数3インデックス(0~63)
//                ※次数2段階では次数2インデックスをそのまま暫定的に流用する
//   param_i[1] : 親となる次数2インデックス(流走のホーム位置／再帰変容の補間元)
//   param_i[2] : 表示色インデックス(次数3到達時に確定するレインボー配色)
//======================================================================
static void ShotHilbertCurve(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --- 敷設フェーズ: 一定間隔で次数2の点を先頭から1つずつ配置 ---
    if (countT <= PHASE1_END) {
        int nextIdx = (countT - 1) / SPAWN_INTERVAL;
        if (nextIdx < 16 && (countT - 1) % SPAWN_INTERVAL == 0) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = g_h2x[nextIdx];
            pEnemyShot->y = g_h2y[nextIdx];
            pEnemyShot->muki = g_muki2[nextIdx];
            pEnemyShot->speed = 0.0;
            pEnemyShot->param_i[0] = nextIdx;
            pEnemyShot->param_i[1] = nextIdx;
            pEnemyShot->param_i[2] = 3; // シアン(曲線本体色)
            pEnemyShot->kind = img_enemyShotSmallBall[3];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // --- 再帰変容フェーズ開始の瞬間: 既存16点の各点から子を3つずつ複製し計64点にする ---
    if (countT == PHASE2_END + 1) {
        sEnemyShot* firstBatch[16];
        int existing = 0;
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead && existing < 16) {
            firstBatch[existing] = pEnemyShot;
            existing++;
            pEnemyShot = pEnemyShot->next;
        }
        for (int i = 0; i < existing; i++) {
            int parentIdx = firstBatch[i]->param_i[1];
            firstBatch[i]->param_i[0] = parentIdx * 4 + 0;

            for (int r = 1; r <= 3; r++) {
                sEnemyShot* pChild = new sEnemyShot;
                pChild->x = g_h2x[parentIdx];
                pChild->y = g_h2y[parentIdx];
                pChild->speed = 0.0;
                pChild->param_i[0] = parentIdx * 4 + r;
                pChild->param_i[1] = parentIdx;
                pChild->kind = img_enemyShotSmallBall[3];

                pChild->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pChild->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pChild;
                pEnemyShotSet->pEnemyShotHead->prev = pChild;
            }
        }
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --- 全弾共通: 現在のフェーズに応じて位置・向き・色を式から算出 ---
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        int idx3 = pEnemyShot->param_i[0];
        int parentIdx = pEnemyShot->param_i[1];

        if (countT <= PHASE1_END) {
            // 敷設フェーズ: 配置時の位置のまま静止
        }
        else if (countT <= PHASE2_END) {
            // 定常流走フェーズ: ホーム位置(=parentIdx)を中心に経路上を往復
            double x0, y0, x1, y1;
            HilbertFlowPos(parentIdx, countT, &x0, &y0);
            HilbertFlowPos(parentIdx, countT + 1, &x1, &y1);
            pEnemyShot->x = x0;
            pEnemyShot->y = y0;
            if (fabs(x1 - x0) > 1e-6 || fabs(y1 - y0) > 1e-6) {
                pEnemyShot->muki = atan2(y1 - y0, x1 - x0);
            }
        }
        else if (countT <= PHASE3_END) {
            // 再帰変容フェーズ: 次数2の位置(ホーム)→次数3の目標位置へイーズ補間
            double t = EaseInOut((double)(countT - PHASE2_END) / (double)PHASE3_LEN);
            double sx = g_h2x[parentIdx], sy = g_h2y[parentIdx];
            double ex = g_h3x[idx3], ey = g_h3y[idx3];
            pEnemyShot->x = sx + (ex - sx) * t;
            pEnemyShot->y = sy + (ey - sy) * t;
            pEnemyShot->muki = atan2(ey - sy, ex - sx);
            pEnemyShot->param_i[2] = idx3 % 7;
            pEnemyShot->kind = img_enemyShotSmallBall[idx3 % 7];
        }
        else if (countT <= PHASE3_HOLD_END) {
            // 次数3静止・予告フェーズ: 完成形で静止し、終盤は白点滅で予告
            pEnemyShot->x = g_h3x[idx3];
            pEnemyShot->y = g_h3y[idx3];
            pEnemyShot->muki = atan2(g_h3y[idx3] - AREA_CY, g_h3x[idx3] - AREA_CX);
            if (countT >= PHASE3_HOLD_END - 24 && ((countT / 4) % 2 == 0)) {
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白点滅
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[pEnemyShot->param_i[2]];
            }
        }
        else {
            // 崩壊フェーズ: 完成位置(=各弾の現在地)から放射状に加速飛散
            double t = (double)(countT - PHASE4_START);
            double dir = atan2(g_h3y[idx3] - AREA_CY, g_h3x[idx3] - AREA_CX);
            double dist = BURST_BASE_SPEED * t + BURST_ACCEL * t * t;
            pEnemyShot->x = g_h3x[idx3] + dist * cos(dir);
            pEnemyShot->y = g_h3y[idx3] + dist * sin(dir);
            pEnemyShot->muki = dir;
            pEnemyShot->kind = img_enemyShotMediumBall[pEnemyShot->param_i[2]];
        }

        if (countT == T - 1) {
            pEnemyShot->margin = -9999;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

//======================================================================
// 角(方向転換点)からの自機狙い3way (定常流走フェーズ中、絶え間なくパルス発射)
//======================================================================
static void ShotCornerAim3Way(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        const double spread = 0.22;
        for (int i = -1; i <= 1; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + spread * i;
            pEnemyShot->speed = 2.6;
            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->kind = img_enemyShotBullet[6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x = pEnemyShot->param_d[0] + pEnemyShot->speed * pEnemyShot->count * cos(pEnemyShot->muki);
        pEnemyShot->y = pEnemyShot->param_d[1] + pEnemyShot->speed * pEnemyShot->count * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

//======================================================================
// フィナーレの自機狙い5way (崩壊フェーズ開始と同時に1回だけ発射)
//======================================================================
static void ShotFinaleAim5Way(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        const double spread = 0.16;
        for (int i = -2; i <= 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + spread * i;
            pEnemyShot->speed = 2.9;
            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->kind = img_enemyShotBullet[0];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x = pEnemyShot->param_d[0] + pEnemyShot->speed * pEnemyShot->count * cos(pEnemyShot->muki);
        pEnemyShot->y = pEnemyShot->param_d[1] + pEnemyShot->speed * pEnemyShot->count * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

//======================================================================
// 敵本体パターン: 再帰律動 -ヒルベルト-
//======================================================================
void EnemyPat_HilbertCurve_Claude()
{
    static int mukiSway;

    if (count == 1) {
        InitHilbertTables();

        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        mukiSway = 1;
    }
    else {
        enemy.x += 0.5 * (double)mukiSway;
        if (count % 160 == 80) mukiSway *= -1;
    }
    countT = count % T;

    if (countT == 1) {
        CreateShotSet(ShotHilbertCurve, 0.0, 0.0); // 各弾が個別に座標を持つためセット自体の座標は未使用
    }
    // 定常流走フェーズ: 角(方向転換点)から絶え間なく自機狙い3wayを連射する
    if (countT > PHASE1_END && countT <= PHASE2_END) {
        int t = countT - PHASE1_END;
        for (int k = 0; k < g_cornerCount; k++) {
            int stagger = (k * CORNER_PULSE_STAGGER) % CORNER_PULSE_INTERVAL;
            if (t % CORNER_PULSE_INTERVAL == stagger) {
                int idx = g_cornerList[k];
                CreateShotSet(ShotCornerAim3Way, g_h2x[idx], g_h2y[idx]);
            }
        }
    }

    // 崩壊フェーズ開始の瞬間、敵本体位置からフィナーレの自機狙い5wayを発射
    if (countT == PHASE4_START) {
        CreateShotSet(ShotFinaleAim5Way, enemy.x, enemy.y + 10.0);
    }
}