// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotKaleidoscope(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int WAY = 6;
        const int NUM = 22;

        for (int w = 0; w < WAY; w++)
        {
            double base = w * DX_PI / 3.0;

            for (int i = 0; i < NUM; i++)
            {
                sEnemyShot* pShot = new sEnemyShot;

                pShot->kind = img_enemyShotSmallBall[(w + i) % 6];
                pShot->speed = 2.6;

                pShot->param_d[0] = pEnemyShotSet->x;
                pShot->param_d[1] = pEnemyShotSet->y;

                pShot->param_d[2] = base;
                pShot->param_d[3] = i * 10.0;

                pShot->param_i[0] = w;
                pShot->param_i[1] = 0;
                pShot->param_i[2] = (w & 1) ? -1 : 1;

                pShot->margin = 480;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead)
    {
        double dist = pShot->count * pShot->speed + pShot->param_d[3];
        int seg = (int)(dist / (32.0 * pShot->speed));
        if (seg > 6) seg = 6;

        double angle =
            pShot->param_d[2]
            + pShot->param_i[2] * seg * DX_PI / 3.0;

        double x = pShot->param_d[0];
        double y = pShot->param_d[1];

        double remain = pShot->count * pShot->speed + pShot->param_d[3];

        for (int k = 0; k < seg; k++)
        {
            x += cos(
                pShot->param_d[2]
                + pShot->param_i[2] * k * DX_PI / 3.0
            ) * 32.0 * pShot->speed;

            y += sin(
                pShot->param_d[2]
                + pShot->param_i[2] * k * DX_PI / 3.0
            ) * 32.0 * pShot->speed;

            remain -= 32.0 * pShot->speed;
        }

        if (remain < 0.0)
            remain = 0.0;

        x += cos(angle) * remain;
        y += sin(angle) * remain;

        pShot->x = x;
        pShot->y = y;

        // 4回折れ曲がったあと、万華鏡が開くように二股化
        if (pShot->count == 128 && pShot->param_i[1] == 0)
        {
            pShot->param_i[1] = 1;

            for (int dir = -1; dir <= 1; dir += 2)
            {
                sEnemyShot* pNew = new sEnemyShot;

                *pNew = *pShot;          // 自由パラメータも含めてコピー
                pNew->count = 0;         // 新しい軌道を開始
                pNew->param_d[0] = pShot->x;
                pNew->param_d[1] = pShot->y;
                pNew->param_d[2] = angle + dir * DX_PI / 6.0;
                pNew->param_d[3] = 0.0;
                pNew->param_i[1] = 2;
                pNew->param_i[2] *= -1;  // 回転方向を反転
                pNew->kind = img_enemyShotSmallBall[(pShot->param_i[0] + dir + 6) % 6];

                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
        }

        pShot = pShot->next;
    }
}

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_ThumbnailFriendly_ChatGPT()
{
    static int dir;
    static int shotCount;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 200.0;
        enemy.maxHp = enemy.hp = 200;

        dir = 1;
        shotCount = 0;
    }
    else
    {
        enemy.x += dir * 0.8;

        if (enemy.x > 360.0)
            dir = -1;
        if (enemy.x < 120.0)
            dir = 1;
    }

    // 約4.3秒ごとに万華鏡を生成
    if (count % 200 == 1)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKaleidoscope;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shotCount++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}