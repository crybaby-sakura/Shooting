// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const int kFireTiming[13] = {
    0, 6, 12,
    30, 36, 42,
    60, 64, 68, 72, 76, 80, 84
};

static void AddShot(
    sEnemyShotSet* pSet,
    double x,
    double y,
    double angle,
    double speed,
    int kind)
{
    sEnemyShot* pShot = new sEnemyShot;

    pShot->x = x;
    pShot->y = y;
    pShot->muki = angle;
    pShot->speed = speed;
    pShot->kind = kind;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

static void FireFirstThree(sEnemyShotSet* pSet)
{
    if (CheckSoundMem(sound_enemyShot_medium))
        StopSoundMem(sound_enemyShot_medium);
    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

    double base = pSet->muki;

    // 5way大玉
    for (int i = -2; i <= 2; i++) {
        AddShot(
            pSet,
            pSet->x,
            pSet->y,
            base + i * 6.0 / 180.0 * DX_PI,
            2.2,
            img_enemyShotLargeBall[4]);
    }
}

static void FireSecondThree(sEnemyShotSet* pSet)
{
    if (CheckSoundMem(sound_enemyShot_medium))
        StopSoundMem(sound_enemyShot_medium);
    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

    double base = pSet->muki;

    // 少し角度をずらした5way
    for (int i = -2; i <= 2; i++) {
        AddShot(
            pSet,
            pSet->x,
            pSet->y,
            base + i * 12.0 / 180.0 * DX_PI,
            2.5,
            img_enemyShotLargeBall[1]);
    }
}

static void FireLastSeven(sEnemyShotSet* pSet)
{
    if (CheckSoundMem(sound_enemyShot_heavy))
        StopSoundMem(sound_enemyShot_heavy);
    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

    // param_d[0] : 現在の花びら回転角
    // param_i[0] : 7連の何発目か

    double base = pSet->muki + pSet->param_d[0];

    // 16way高速弾
    for (int i = 0; i < 32; i++) {
        AddShot(
            pSet,
            pSet->x,
            pSet->y,
            base + DX_PI * 2.0 * i / 32.0,
            4.4,
            img_enemyShotBullet[0]);
    }

    // 隙間埋め用8way
    for (int i = 0; i < 16; i++) {
        AddShot(
            pSet,
            pSet->x,
            pSet->y,
            base + 11.25 / 180.0 / 2 * DX_PI + DX_PI * 2.0 * i / 16.0,
            2.8,
            img_enemyShotMediumBall[8]);
    }

    // 次回の回転角
    pSet->param_d[0] += 9.0 / 180.0 * DX_PI / 2;
    pSet->param_i[0]++;
}

static void Shot337(sEnemyShotSet* pSet)
{
    for (int i = 0; i < 13; i++) {

        if (pSet->count != kFireTiming[i])
            continue;

        if (i < 3) {
            FireFirstThree(pSet);
        }
        else if (i < 6) {
            FireSecondThree(pSet);
        }
        else {
            FireLastSeven(pSet);
        }
    }

    // 全弾移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;

    while (pShot != pSet->pEnemyShotHead) {

        pShot->x += cos(pShot->muki) * pShot->speed;
        pShot->y += sin(pShot->muki) * pShot->speed;

        pShot = pShot->next;
    }
}

void EnemyPat_337Beat_ChatGPT()
{
    static int dir;
    static int cycle;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;

        dir = 1;
        cycle = 0;
    }

    // ゆっくり左右移動
    enemy.x += dir * 1.1;

    if (enemy.x > 390.0) dir = -1;
    if (enemy.x < 90.0)  dir = 1;

    // 337拍子の少し前に予告音
    if (count % 150 == 120) {
        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 337拍子開始
    if (count % 150 == 1) {

        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = Shot337;

        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;

        pSet->muki =
            atan2(
                player.y - pSet->y,
                player.x - pSet->x);

        // セットごとに開始角度を変える
        pSet->param_d[0] =
            cycle * 30.0 / 180.0 * DX_PI;

        pSet->param_i[0] = 0;

        cycle = (cycle + 1) % 12;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}