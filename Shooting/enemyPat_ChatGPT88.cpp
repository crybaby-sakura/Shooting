// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 蚊取り線香の螺旋を構成する通常弾。
// 緑の中玉を連続生成し、同じ軌道上を外側へ広げる。
static void ShotMosquitoCoil(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 一定時間ごとに螺旋の回転方向を反転
    const int flipPeriod = 150;
    int section = pEnemyShotSet->count / flipPeriod;

    // 螺旋の回転量を時間から求める
    auto RotateAmount = [&](int t) {
        double result = 0.0;
        const double omega = 0.075;

        int full = t / flipPeriod;
        int rest = t % flipPeriod;

        for (int i = 0; i < full; i++) {
            result += ((i & 1) ? -1.0 : 1.0) * omega * flipPeriod;
        }

        result += ((full & 1) ? -1.0 : 1.0) * omega * rest;
        return result;
    };

    // 螺旋本体
    if (pEnemyShotSet->count < 360 && pEnemyShotSet->count % 4 == 0) {
        sEnemyShot* pEnemyShot = new sEnemyShot;

        pEnemyShot->param_i[0] = pEnemyShotSet->count;
        pEnemyShot->param_d[0] = pEnemyShotSet->count * 0.115;

        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotMediumBall[2]; // 緑の中玉

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 現在存在する螺旋弾を更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int spawnTime = pShot->param_i[0];
        int absoluteTime = spawnTime + pShot->count;

        double radius = 7.0 + pShot->count * 0.82;
        double angle =
            pShot->param_d[0] +
            RotateAmount(absoluteTime) -
            RotateAmount(spawnTime);

        pShot->x = pEnemyShotSet->x + radius * cos(angle);
        pShot->y = pEnemyShotSet->y + radius * sin(angle);

        pShot = pShot->next;
    }
}

// 蚊取り線香の先端。
// 大きな橙色の弾を螺旋の先頭として動かす。
static void ShotMosquitoCoilEmber(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;

        pEnemyShot->kind = img_enemyShotLargeBall[8]; // 橙の大玉
        pEnemyShot->speed = 0.0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    if (pShot == pEnemyShotSet->pEnemyShotHead)
        return;

    const int flipPeriod = 150;
    const double omega = 0.075;

    auto RotateAmount = [&](int t) {
        double result = 0.0;
        int full = t / flipPeriod;
        int rest = t % flipPeriod;

        for (int i = 0; i < full; i++) {
            result += ((i & 1) ? -1.0 : 1.0) * omega * flipPeriod;
        }

        result += ((full & 1) ? -1.0 : 1.0) * omega * rest;
        return result;
    };

    double radius = 7.0 + pEnemyShotSet->count * 0.82;
    double angle = RotateAmount(pEnemyShotSet->count);

    pShot->x = pEnemyShotSet->x + radius * cos(angle);
    pShot->y = pEnemyShotSet->y + radius * sin(angle);
}

// 敵本体のパターン
void EnemyPat_MosquitoCoil_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.72 * muki;

        if (count % 120 == 60)
            muki *= -1;
    }

    // 螺旋本体を開始
    if (count % 450 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMosquitoCoil;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 240.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 螺旋の先端に燃焼部分を追加
    if (count % 450 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMosquitoCoilEmber;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 240.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}