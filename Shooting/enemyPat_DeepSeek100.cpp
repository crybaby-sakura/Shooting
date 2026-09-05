// EnemyPat_HilbertCurve_DeepSeek.cpp
// ヒルベルト曲線をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 整数インデックスからグリッド座標を求める（従来のロジック）
static void HilbertD2xy_Int(int n, int idx, int& x, int& y)
{
    int rx, ry, s, t = idx;
    x = y = 0;
    int size = 1 << n;

    for (s = 1; s < size; s *= 2) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);

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
        t /= 4;
    }
}

// 半整数インデックスに対応した座標取得（線形補間）
static void HilbertD2xy(int n, double idx, double& x, double& y)
{
    int total = 1 << (2 * n);      // 4^n
    double maxIdx = (double)(total - 1);

    // 範囲をクランプ
    if (idx < 0.0) idx = 0.0;
    if (idx > maxIdx) idx = maxIdx;

    double floorIdx = floor(idx);
    double frac = idx - floorIdx;
    int i0 = (int)floorIdx;

    int x0, y0, x1, y1;
    HilbertD2xy_Int(n, i0, x0, y0);

    if (frac == 0.0 || i0 == total - 1) {
        // 整数インデックス or 最後の点
        x = (double)x0;
        y = (double)y0;
    }
    else {
        int i1 = i0 + 1;
        HilbertD2xy_Int(n, i1, x1, y1);
        // 線形補間
        x = x0 + frac * (x1 - x0);
        y = y0 + frac * (y1 - y0);
    }
}

// ------------------------------------------------------------
// 弾幕パターン：ヒルベルトスネーク
// 各フレーム、弾の位置をヒルベルト曲線のノード座標に合わせて更新する
// ------------------------------------------------------------
static void HilbertSnake(sEnemyShotSet* pEnemyShotSet)
{
    int total = pEnemyShotSet->param_i[0];  // 弾の総数
    int n = pEnemyShotSet->param_i[1];  // 次数

    double baseScale = pEnemyShotSet->param_d[0];  // 基本スケール（グリッド1マス分の長さ）
    double rot = pEnemyShotSet->param_d[1] + pEnemyShotSet->param_d[2] * pEnemyShotSet->count; // 回転角
    double scale = baseScale * (1.0 + pEnemyShotSet->param_d[3] * sin(pEnemyShotSet->param_d[4] * pEnemyShotSet->count)); // 拡大縮小

    double cx = pEnemyShotSet->x;  // 曲線の中心座標
    double cy = pEnemyShotSet->y;

    double cosr = cos(rot);
    double sinr = sin(rot);

    int gridSize = (1 << n) - 1;  // 2^n - 1

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double idx = pShot->param_i[0] / 2.0;  // 半整数インデックスを復元

        double hx, hy;
        HilbertD2xy(n, idx, hx, hy);

        // 中心を (0,0) に正規化
        double fx = hx - gridSize / 2.0;
        double fy = hy - gridSize / 2.0;

        // スケール
        fx *= scale;
        fy *= scale;

        // 回転
        double rx = fx * cosr - fy * sinr;
        double ry = fx * sinr + fy * cosr;

        // 配置
        pShot->x = cx + rx;
        pShot->y = cy + ry;
        pShot->speed = 0.0;  // 位置直接指定のため速度は0

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_HilbertCurve_DeepSeek()
{
    static int muki;
    static int shot_kind;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_kind = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 180フレームごとにヒルベルトスネークを1セット生成
    if (count % 210 == 1) {
        int n = 3;                      // 次数（8×8 = 64ノード）
        int total = 2 * (1 << (2 * n)) - 1;  // 2*4^n - 1

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = HilbertSnake;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 200.0;   // 敵の少し下に曲線中心を置く
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_kind++;

        // パラメータ設定
        pEnemyShotSet->param_i[0] = total;   // 弾数
        pEnemyShotSet->param_i[1] = n;       // 次数
        pEnemyShotSet->param_d[0] = 26.0 * 2;    // 基本スケール（1マスあたりの長さ）
        pEnemyShotSet->param_d[1] = 0.0;     // 初期回転角
        pEnemyShotSet->param_d[2] = 0.015 / 2;   // 回転速度（ラジアン/フレーム）
        pEnemyShotSet->param_d[3] = 0.15;    // スケールの振動振幅
        pEnemyShotSet->param_d[4] = 0.04;    // スケールの振動周波数

        // 弾リストのヘッダーを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 効果音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // ノードごとに弾を生成
        for (int i = 0; i < total; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->count = 0;
            pEnemyShot->kind = img_enemyShotSmallBall[i % 6];  // 色を順番に
            pEnemyShot->param_i[0] = i;  // ノード番号を記憶
            pEnemyShot->margin = 240;

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 敵弾セットをグローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}