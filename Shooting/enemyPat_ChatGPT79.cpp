// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 自機へ向かいながらリサジュー状に揺れる弾
// ============================================================
static void ShotLissajousHoming(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 18発を同時発射
        for (int i = 0; i < 18 * 2; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->x = pSet->x;
            pShot->y = pSet->y;

            // 発射時の自機方向を保存
            pShot->muki = pSet->muki;
            pShot->speed = 1.7;

            // 位相
            pShot->param_d[0] = DX_PI * 2 * i / 18.0 / 2;

            // 横方向の振幅
            pShot->param_d[1] = 70.0;

            // 前後方向の振幅
            pShot->param_d[2] = 42.0;

            // 時間倍率
            pShot->param_d[3] = 0.020;

            // 中玉を使用
            pShot->kind = img_enemyShotMediumBall[i % 8];
            pShot->margin = 100;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;

            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 発射時点で狙った方向へ進み続ける
    const double fx = cos(pSet->muki);
    const double fy = sin(pSet->muki);

    // 進行方向と直角な方向
    const double sx = -fy;
    const double sy = fx;

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;

    while (pShot != pSet->pEnemyShotHead) {
        const double phase = pShot->param_d[0];
        const double ampSide = pShot->param_d[1];
        const double ampForward = pShot->param_d[2];
        const double w = pShot->param_d[3];

        const double t = pShot->count;

        // リサジュー曲線
        //
        // 横方向：sin(3t)
        // 前方向：sin(2t)
        //
        // ただし全体として自機方向へ進むよう、
        // 前方向へ一定速度の直進成分を追加する。
        const double side =
            ampSide *
            (sin(3.0 * w * t + phase) - sin(phase));

        const double forward =
            pShot->speed * t
            + ampForward *
            (sin(2.0 * w * t + phase) - sin(phase));

        pShot->x = pSet->x
            + fx * forward
            + sx * side;

        pShot->y = pSet->y
            + fy * forward
            + sy * side;

        pShot = pShot->next;
    }
}


// ============================================================
// 敵本体
// ============================================================
void EnemyPat_Lissajous_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 45.0;

        enemy.maxHp = enemy.hp = 200;

        muki = 1;
    }
    else {
        // 敵は左右にゆっくり移動
        enemy.x += 0.8 * muki;

        if (enemy.x < 100.0)
            muki = 1;

        if (enemy.x > 380.0)
            muki = -1;
    }

    // 50フレームごとに自機を狙って発射
    if (count % 50 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = ShotLissajousHoming;

        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;

        // 発射時点の自機位置へ向ける
        pSet->muki =
            atan2(
                player.y - pSet->y,
                player.x - pSet->x
            );

        pSet->kind = count / 50;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;

        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}