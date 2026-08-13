// enemyPat_Yoyo.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ヨーヨー弾
// 外へ進む -> 一定距離で反転 -> 中心へ戻る -> 再び外へ進む、を繰り返す。
// pEnemyShot->count はメインルーチン側で毎フレーム+1される前提。
// ============================================================
static void ShotYoyo(sEnemyShotSet* pEnemyShotSet)
{
    // 弾の生成はセット生成直後の1回だけ。
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 7本を少しずつ角度をずらして配置。
        // 中心に近い弾から外側まで、ヨーヨーの長さを変える。
        for (int i = 0; i < 21; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (i - 3) * 0.055;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[pEnemyShotSet->kind % 6]; // シアン
            pEnemyShot->margin = 240;

            pEnemyShot->param_i[0] = i;       // 弾番号
            pEnemyShot->param_i[1] = 48 + i * 13; // 往復半径
            pEnemyShot->param_i[2] = 34 + i * 2 + 100;  // 周期

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 1周期の前半で外へ、後半で中心へ戻る。
        // cos の 0 -> -1 -> 0 を利用し、距離を滑らかに往復させる。
        const double period = (double)pShot->param_i[2];
        const double radius = (double)pShot->param_i[1];
        const double phase = (double)(pShot->count % (int)period) / period;
        const double dist = radius * (0.5 - 0.5 * cos(2.0 * DX_PI * phase));

        pShot->x = pEnemyShotSet->x + cos(pShot->muki) * dist;
        pShot->y = pEnemyShotSet->y + sin(pShot->muki) * dist;

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_Yoyo_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // ボスは左右へゆっくり往復。
        enemy.x += 0.72 * (double)muki;
        if (enemy.x <= 120.0 || enemy.x >= 360.0)
            muki *= -1;
    }

    // 48フレームごとに新しいヨーヨーを発生。
    // 発射方向を少しずつ回して、画面全体を覆う。
    if (count % 48 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotYoyo;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        const int n = shot_count++;
        pEnemyShotSet->muki = -DX_PI * 0.5 + (n % 8) * (DX_PI * 2.0 / 8.0) + 0.12 * sin(n * 0.7);
        pEnemyShotSet->kind = n;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}