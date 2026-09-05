// enemyPat_hilbert.cpp
// 弾幕：ヒルベルト迷宮

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// ヒルベルト曲線の1次元インデックス -> 2次元座標
// order = 4, grid = 16 x 16
// ------------------------------------------------------------
static void HilbertIndexToGrid(int index, int order, int& x, int& y)
{
    x = 0;
    y = 0;

    int n = 1 << order;
    for (int s = 1; s < n; s <<= 1) {
        int rx = 1 & (index >> 1);
        int ry = 1 & (index ^ rx);

        if (ry == 0) {
            if (rx == 1) {
                x = s - 1 - x;
                y = s - 1 - y;
            }
            int tmp = x;
            x = y;
            y = tmp;
        }

        x += s * rx;
        y += s * ry;
        index >>= 2;
    }
}

// ヒルベルト曲線上の連続座標を取得
static void GetHilbertPos(double t, int phase, double& x, double& y)
{
    const int order = 4;
    const int grid = 16;
    const double left = 5.0;
    const double top = 5.0;
    const double step = 470.0 / (grid - 1);

    // 曲線を往復させつつ、位相で別の走査位置を作る
    double u = (t + phase * 17.0) * 0.1;
    u = u - floor(u / 256.0) * 256.0;

    int i0 = (int)floor(u);
    int i1 = (i0 + 1) & 255;
    double f = u - floor(u);

    int x0, y0, x1, y1;
    HilbertIndexToGrid(i0, order, x0, y0);
    HilbertIndexToGrid(i1, order, x1, y1);

    double gx = x0 + (x1 - x0) * f;
    double gy = y0 + (y1 - y0) * f;

    // 位相ごとに鏡映を入れて迷路の接続方向を変える
    int mode = phase & 3;
    if (mode == 1) gx = (grid - 1) - gx;
    if (mode == 2) gy = (grid - 1) - gy;
    if (mode == 3) {
        double tmp = gx;
        gx = (grid - 1) - gy;
        gy = tmp;
    }

    x = left + gx * step;
    y = top + gy * step;
}

// 弾の挙動：ヒルベルト曲線に沿って移動
static void ShotHilbert(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 一定間隔で複数本の線を追加し、画面全体を埋める
    if (pEnemyShotSet->count % 4 == 0 && pEnemyShotSet->param_i[0] < 720) {
        for (int k = 0; k < 2; k++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            int phase = pEnemyShotSet->param_i[1]++;

            pEnemyShot->x = 240.0;
            pEnemyShot->y = 240.0;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 1.0;
            pEnemyShot->kind = (k == 0) ? img_enemyShotSmallBall[3] : img_enemyShotDiamond[4];

            pEnemyShot->param_i[0] = phase;
            pEnemyShot->param_i[1] = (phase / 4) % 256;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        pEnemyShotSet->param_i[0]++;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count * 0.55 + pShot->param_i[1];
        double x, y;
        GetHilbertPos(t, pShot->param_i[0], x, y);

        pShot->x = x;
        pShot->y = y;

        // 描画上の向きも曲線の接線方向へ合わせる
        double x2, y2;
        GetHilbertPos(t + 0.8, pShot->param_i[0], x2, y2);
        pShot->muki = atan2(y2 - y, x2 - x);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_HilbertCurve_ChatGPT()
{
    static int muki;
    static sEnemyShotSet* pSet;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        pSet = nullptr;
    }
    else {
        enemy.x += 0.85 * (double)muki;
        if (enemy.x < 70.0 || enemy.x > 410.0) muki *= -1;
    }

    if (count == 1) {
        pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHilbert;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}
