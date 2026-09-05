// enemyPat_Tmp.cpp
// ヒルベルト曲線弾幕 - 充填迷宮

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ========= ヒルベルト曲線ユーティリティ =========
static void hilbert_rot(int n, int* x, int* y, int rx, int ry)
{
    if (ry == 0) {
        if (rx == 1) {
            *x = n - 1 - *x;
            *y = n - 1 - *y;
        }
        int t = *x;
        *x = *y;
        *y = t;
    }
}

// n = 1<<order, d = 0..n*n-1 -> (x,y)
static void hilbert_d2xy(int n, int d, int* x, int* y)
{
    int rx, ry, s, t = d;
    *x = 0; *y = 0;
    for (s = 1; s < n; s *= 2) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);
        hilbert_rot(s, x, y, rx, ry);
        *x += s * rx;
        *y += s * ry;
        t /= 4;
    }
}

// 連続した d からワールド座標と進行方向を出す
static void hilbert_getPos(double d_cont, int order, double fieldSize, double cx, double cy, double* outX, double* outY, double* outMuki)
{
    int nSide = 1 << order;
    int total = nSide * nSide;
    double d = fmod(d_cont, (double)total);
    if (d < 0) d += total;
    int d0 = (int)floor(d);
    int d1 = (d0 + 1) % total;
    double frac = d - d0;

    int x0, y0, x1, y1;
    hilbert_d2xy(nSide, d0, &x0, &y0);
    hilbert_d2xy(nSide, d1, &x1, &y1);

    double step = fieldSize / (double)nSide;
    double half = (nSide - 1) * 0.5;

    double wx0 = cx + (x0 - half) * step;
    double wy0 = cy + (y0 - half) * step;
    double wx1 = cx + (x1 - half) * step;
    double wy1 = cy + (y1 - half) * step;

    *outX = wx0 + (wx1 - wx0) * frac;
    *outY = wy0 + (wy1 - wy0) * frac;
    *outMuki = atan2((double)(y1 - y0), (double)(x1 - x0));
}

// ========= 弾幕本体 =========
static void ShotHilbert(sEnemyShotSet* pSet)
{
    // param_i 割り当て
    // [0] phase 0:描画 1:移動 2:圧縮 3:開放
    // [1] order
    // [2] totalPoints
    // [3] nextD (描画用)
    // [4] move timer
    // [5] compress timer
    // param_d [0] fieldSize初期値 / [1] cx / [2] cy

    if (pSet->count == 0) {
        int order = (pSet->kind == 0) ? 3 : 4; // 3:64発で易、4:256発で難
        int total = 1 << (2 * order); // 4^order
        pSet->param_i[0] = 0;
        pSet->param_i[1] = order;
        pSet->param_i[2] = total;
        pSet->param_i[3] = 0;
        pSet->param_i[4] = 0;
        pSet->param_i[5] = 0;
        pSet->param_d[0] = 360.0; // 初期フィールド
        pSet->param_d[1] = 240.0; // 中心x
        pSet->param_d[2] = 260.0; // 中心y 少し下

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        return;
    }

    int phase = pSet->param_i[0];
    int order = pSet->param_i[1];
    int total = pSet->param_i[2];
    double cx = pSet->param_d[1];
    double cy = pSet->param_d[2];

    if (phase == 0) { // 描画フェーズ
        int spawnPerFrame = 3;
        double field = pSet->param_d[0];
        for (int k = 0; k < spawnPerFrame; k++) {
            int d = pSet->param_i[3];
            if (d >= total) break;

            double hx, hy, hm;
            hilbert_getPos((double)d, order, field, cx, cy, &hx, &hy, &hm);

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = hx;
            pShot->y = hy;
            pShot->muki = hm;
            pShot->speed = 0.0;
            // param_d[0] = ヒルベルト上の位置(連続)、param_d[1] = 速度の微揺らぎ
            pShot->param_d[0] = (double)d;
            // GetRandは0..xを返すので 0..20 -> -10..10 に変換
            pShot->param_d[1] = (GetRand(20) - 10) * 0.002;
            pShot->param_i[0] = (d % 6 == 0) ? 1 : 0; // 1が流弾

            if (pShot->param_i[0] == 1) {
                pShot->kind = img_enemyShotMediumBall[8]; // 橙 中玉
            }
            else {
                pShot->kind = img_enemyShotSmallBall[4]; // 青 小玉 隙間が見やすい
            }

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;

            pSet->param_i[3]++;
        }

        if (pSet->count % 12 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        if (pSet->param_i[3] >= total) {
            pSet->param_i[0] = 1; // 移動へ
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }
    else if (phase == 1) { // 移動フェーズ
        double field = pSet->param_d[0];
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            double add = pShot->param_i[0] ? 0.55 : 0.18;
            pShot->param_d[0] += add + pShot->param_d[1];
            if (pShot->param_d[0] >= total) pShot->param_d[0] -= total;

            double nx, ny, nm;
            hilbert_getPos(pShot->param_d[0], order, field, cx, cy, &nx, &ny, &nm);
            pShot->x = nx;
            pShot->y = ny;
            pShot->muki = nm;
            pShot = pShot->next;
        }
        pSet->param_i[4]++;
        if (pSet->param_i[4] > 90) {
            pSet->param_i[0] = 2;
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }
    else if (phase == 2) { // 圧縮フェーズ
        int ct = pSet->param_i[5];
        double t = ct / 90.0;
        if (t > 1.0) t = 1.0;
        double curField = 360.0 * (1.0 - 0.30 * t); // 360 -> 252

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            double add = pShot->param_i[0] ? 0.65 : 0.22;
            pShot->param_d[0] += add + pShot->param_d[1];
            if (pShot->param_d[0] >= total) pShot->param_d[0] -= total;
            double nx, ny, nm;
            hilbert_getPos(pShot->param_d[0], order, curField, cx, cy, &nx, &ny, &nm);
            pShot->x = nx;
            pShot->y = ny;
            pShot->muki = nm;
            pShot = pShot->next;
        }
        pSet->param_i[5]++;
        if (ct > 90) {
            pSet->param_i[0] = 3;
            sEnemyShot* s = pSet->pEnemyShotHead->next;
            while (s != pSet->pEnemyShotHead) {
                double ang = atan2(s->y - cy, s->x - cx);
                s->muki = ang + (GetRand(20) - 10) * 0.03; // 微ランダム
                s->speed = s->param_i[0] ? 2.8 : 1.6;
                s = s->next;
            }
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
    }
    else if (phase == 3) { // 開放：外に飛ばす
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
}

// ========= 敵本体 =========
void EnemyPat_HilbertCurve_MetaAI()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 7秒おきにヒルベルト迷宮を生成。重ならないように1つずつ
    if (count % 240 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHilbert;
        pSet->x = 240.0;
        pSet->y = 260.0;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}