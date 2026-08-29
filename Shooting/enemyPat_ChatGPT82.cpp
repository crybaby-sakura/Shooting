// enemyPat_futon.cpp
// 「布団が吹っ飛んだ」をモチーフにした弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotFuton(sEnemyShotSet* pEnemyShotSet)
{
    const int W = 9 + 3;          // 横方向の弾数
    const int H = 5 + 2;          // 縦方向の弾数
    const double DX = 24.0;
    const double DY = 24.0;
    const double FLY_START = 90.0;

    if (pEnemyShotSet->count == 0) {
        // 布団の出現音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 長方形の輪郭を中玉で作る
        // 内側は小玉で縫い目を表現する
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                bool edge = (x == 0 || x == W - 1 || y == 0 || y == H - 1);
                bool seam = (y == H / 2 && x > 0 && x < W - 1) ||
                    (x == W / 2 && y > 0 && y < H - 1);

                if (!edge && !seam) continue;

                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = 0.0;
                pShot->speed = 0.0;

                pShot->kind = edge
                    ? img_enemyShotMediumOval[(x + y) % 8]
                    : img_enemyShotSmallBall[(x + y) % 8];

                pShot->param_d[0] = (x - (W - 1) * 0.5) * DX;
                pShot->param_d[1] = (y - (H - 1) * 0.5) * DY;
                pShot->param_d[2] = 0.0; // 初期角度差
                pShot->param_d[3] = 1.0 + 0.08 * ((x + y) % 4);
                pShot->param_i[0] = 0;
                pShot->margin = 240;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 初期状態では「布団らしく」ゆっくり揺れる
    // 一定時間後、一気に吹っ飛ぶ
    const bool flying = pEnemyShotSet->count >= FLY_START;
    const double t = flying ? pEnemyShotSet->count - FLY_START : pEnemyShotSet->count;

    double flyAngle = pEnemyShotSet->muki;
    double push = flying ? 2.4 : 0.18;
    double rot = flying ? 0.012 : 0.0025;

    // 飛び始めに大きく加速する演出
    double flyDistance = flying
        ? 0.55 * t * t + 0.8 * t
        : 0.0;

    double baseX = pEnemyShotSet->x + cos(flyAngle) * flyDistance;
    double baseY = pEnemyShotSet->y + sin(flyAngle) * flyDistance;
    double angle = pEnemyShotSet->param_d[2] + rot * t;

    if (flying && pEnemyShotSet->count == (int)FLY_START) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            double x = pShot->param_d[0];
            double y = pShot->param_d[1];

            // 布団が柔らかく揺れているような微小変形
            double wobble = sin((pEnemyShotSet->count * 0.08) + x * 0.03) *
                cos((pEnemyShotSet->count * 0.06) + y * 0.02) *
                (flying ? 0.8 : 2.2);

            double s = sin(angle);
            double c = cos(angle);
            double rx = x * c - y * s;
            double ry = x * s + y * c;

            // 「吹っ飛ぶ」瞬間だけ、布団の端を少し遅らせて布のしなりを出す
            double lag = flying ? sin((y + 80.0) * 0.04 + t * 0.08) * 2.5 : 0.0;
            rx += wobble;
            ry += lag;

            pShot->x = baseX + rx;
            pShot->y = baseY + ry;

            // 飛翔中は布団の各部が少しだけ違う速度で動く
            if (flying) {
                pShot->x += cos(flyAngle + 0.04 * (pShot->param_d[1] / DY)) *
                    push * t * (pShot->param_d[3] - 1.0);
                pShot->y += sin(flyAngle + 0.04 * (pShot->param_d[0] / DX)) *
                    push * t * (pShot->param_d[3] - 1.0);

                if (pShot->x < 0 || pShot->x > 480 || pShot->y < 0 || pShot->y > 480) {
                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                    pShot->param_i[0] = 1;
                    pShot->speed = 2.0 + GetRand(30) / 10.0;
                    pShot->muki = GetRand(100) / 100.0 * 2.0 * DX_PI;
                }
            }
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

void EnemyPat_FutonFlewAway_ChatGPT()
{
    static int direction;
    static int futonIndex;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        direction = 1;
        futonIndex = 0;
    }
    else {
        // ボス自身は画面上部をゆっくり往復
        enemy.x += 0.75 * direction;
        if (enemy.x < 120.0 || enemy.x > 360.0) direction *= -1;
    }

    // 布団を一定間隔で出現させる
    if (count % 100 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFuton;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 55.0;

        // 吹っ飛ぶ方向は、左右交互にして画面全体へ広げる
        double dir = (futonIndex % 2 == 0) ? -1.0 : 1.0;
        pSet->muki = atan2(0.9, dir);
        pSet->kind = futonIndex++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}