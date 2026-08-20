// enemyPat_rainbow.cpp
// 虹環「スペクトラル・ハレーション」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 虹色のカラーインデックス（0:赤,8:橙,1:黄,2:緑,3:シアン,4:青,5:マゼンタ）
static const int rainbowColor[7] = { 0, 8, 1, 2, 3, 4, 5 };

// 弾追加ヘルパー
static void AddShot(sEnemyShotSet* pSet,
    double x, double y,
    double muki, double speed,
    int kind,
    int colorIndex,
    int bulletIndex,
    int type,
    double baseAngle = 0.0,
    double baseRadius = 0.0,
    double angularSpeed = 0.0,
    double radiusAmp = 0.0,
    double radiusFreq = 0.0,
    double radiusPhase = 0.0)
{
    sEnemyShot* p = new sEnemyShot;

    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;

    p->param_i[0] = colorIndex;
    p->param_i[1] = bulletIndex;
    p->param_i[2] = type; // 0=リング弾, 1=レーザー

    p->param_d[0] = baseAngle;
    p->param_d[1] = baseRadius;
    p->param_d[2] = angularSpeed;
    p->param_d[3] = radiusAmp;
    p->param_d[4] = radiusFreq;
    p->param_d[5] = radiusPhase;
    p->margin = 100;

    // リンクリスト末尾に追加
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// 虹環パターン本体
static void RainbowRing(sEnemyShotSet* pSet)
{
    const int T = pSet->count;          // このパターン開始からのフレーム数
    const double cx = enemy.x;          // 敵の現在位置を中心に使う
    const double cy = enemy.y;

    // --- イベント（弾生成・状態変化） ---
    if (T == 0) {
        // 7色のリングを生成
        const int bulletsPerRing = 24;
        for (int j = 0; j < 7; ++j) {
            const int color = rainbowColor[j];

            // 外側ほど半径が大きく、ゆっくり回る
            const double baseRadius = 180.0 - j * 20.0;
            const double omega = (0.004 + 0.002 * j) * (j % 2 == 0 ? 1.0 : -1.0);
            const double radiusAmp = 8.0 + j * 4.0;
            const double radiusFreq = 0.02 + j * 0.003;
            const double radiusPhase = j * 0.8;

            for (int k = 0; k < bulletsPerRing; ++k) {
                const double baseAngle = k * 2.0 * DX_PI / bulletsPerRing + j * 0.15;
                const int idx = j * bulletsPerRing + k;

                AddShot(pSet,
                    cx, cy,
                    baseAngle + DX_PI / 2.0,  // 接線方向（予備）
                    0.0,
                    img_enemyShotSmallBall[color],
                    color,
                    idx,
                    0,                        // リング弾
                    baseAngle, baseRadius, omega,
                    radiusAmp, radiusFreq, radiusPhase);
            }
        }
    }
    else if (T == 120) {
        // 各リング弾から接線方向へ短レーザーを発射
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            sEnemyShot* next = p->next; // 挿入前に次を保存

            if (p->param_i[2] == 0) {
                const int color = p->param_i[0];
                const int idx = p->param_i[1];
                const double tangent = p->muki; // 接線方向
                AddShot(pSet,
                    p->x, p->y,
                    tangent,
                    150.0 / 100,
                    img_enemyShotLaser[color],
                    color,
                    idx,
                    1); // レーザー
            }
            p = next;
        }
    }
    else if (T == 210) {
        // 全弾を中心に集め、8方向へ放射
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[2] == 0) {
                p->muki = (p->param_i[1] % 8) * (2.0 * DX_PI / 8.0) + DX_PI / 8.0;
                p->speed = 200.0 + (p->param_i[1] % 3) * 40.0; p->speed /= 100;
                p->x = cx;
                p->y = cy;
            }
            p = p->next;
        }
    }

    // --- 弾の移動 ---
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (T < 180) {
            if (p->param_i[2] == 0) {
                // リング弾：回転＋半径の伸縮
                const double angle = p->param_d[0] + p->param_d[2] * T;
                const double radius = p->param_d[1]
                    + p->param_d[3] * sin(p->param_d[4] * T + p->param_d[5]);

                p->x = cx + radius * cos(angle);
                p->y = cy + radius * sin(angle);
                p->muki = angle + DX_PI / 2.0; // 接線方向を更新
            }
            else {
                // レーザー弾：直進
                p->x += p->speed * cos(p->muki);
                p->y += p->speed * sin(p->muki);
            }
        }
        else if (T < 210) {
            if (p->param_i[2] == 0) {
                // 中心へ吸い込まれる
                const double dx = cx - p->x;
                const double dy = cy - p->y;
                p->x += dx * 0.2;
                p->y += dy * 0.2;
            }
            else {
                // レーザー弾：直進
                p->x += p->speed * cos(p->muki);
                p->y += p->speed * sin(p->muki);
            }
        }
        else {
            // 放射弾として直進
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }

        p = p->next;
    }
}

// 敵本体パターン
void EnemyPat_Rainbow_DeepSeek()
{
    static int muki;
    static int shot_kind;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 200.0;   // 虹の輪が画面内に収まりやすい位置
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_kind = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;

        // 左右の画面端で折り返す
        if (enemy.x < 30.0) { enemy.x = 30.0;  muki = 1; }
        if (enemy.x > 450.0) { enemy.x = 450.0; muki = -1; }
    }

    // 一定間隔で虹環パターンを開始
    if (count % 240 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = RainbowRing;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_kind++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
}