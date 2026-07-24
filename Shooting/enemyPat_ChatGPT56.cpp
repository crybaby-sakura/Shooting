// enemyPat_Tmp.cpp
// エビングハウス錯視弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotEbbinghaus(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const double cx[2] = { 150.0,330.0 };
        const double cy = 170.0;

        // --------------------------------------------------
        // 中央の基準弾（実際は全く同じ大きさ）
        // --------------------------------------------------
        for (int i = 0; i < 2; i++)
        {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->kind = img_enemyShotLargeBall[6];
            pShot->speed = 0.0;

            pShot->param_i[0] = 0;          // 中央弾
            pShot->param_i[1] = i;          // 左右
            pShot->param_d[0] = cx[i];
            pShot->param_d[1] = cy;

            pShot->x = cx[i];
            pShot->y = cy;

            pShot->margin = 100;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // --------------------------------------------------
        // 左側：小さい弾を密集
        // --------------------------------------------------
        for (int i = 0; i < 16; i++)
        {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->kind = img_enemyShotSmallBall[2];
            pShot->speed = 0.0;

            pShot->param_i[0] = 1;
            pShot->param_i[1] = 0;

            pShot->param_d[0] = i / 16.0 * DX_PI * 2.0;
            pShot->param_d[1] = 28.0;

            pShot->x = cx[0];
            pShot->y = cy;

            pShot->margin = 100;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // --------------------------------------------------
        // 右側：大きい弾を疎に配置
        // --------------------------------------------------
        for (int i = 0; i < 10; i++)
        {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->kind = img_enemyShotLargeBall[1];
            pShot->speed = 0.0;

            pShot->param_i[0] = 2;
            pShot->param_i[1] = 1;

            pShot->param_d[0] = i / 10.0 * DX_PI * 2.0;
            pShot->param_d[1] = 60.0;

            pShot->x = cx[1];
            pShot->y = cy;
             
            pShot->margin = 100;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }
    pEnemyShotSet->y += 1.0;

    // 以下、各弾の挙動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead)
    {
        switch (pShot->param_i[0])
        {
        case 0:
        {
            // 基準弾
            if (pEnemyShotSet->count < 180)
            {
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];
            }
            else if (pEnemyShotSet->count < 300)
            {
                // 錯視が切り替わるタイミングで左右を入れ替える
                double t = (pEnemyShotSet->count - 180) / 120.0;
                double x0 = (pShot->param_i[1] == 0) ? 150.0 : 330.0;
                double x1 = (pShot->param_i[1] == 0) ? 330.0 : 150.0;

                pShot->x = x0 + (x1 - x0) * t;
                pShot->y = 170.0;
            }
            else
            {
                pShot->x = (pShot->param_i[1] == 0) ? 330.0 : 150.0;
                pShot->y = 170.0;
            }
            break;
        }
        case 1:
        {
            // 左側：小さい弾のリング
            double cx, cy = 170.0;

            if (pEnemyShotSet->count < 180)
                cx = 150.0;
            else if (pEnemyShotSet->count < 300)
            {
                double t = (pEnemyShotSet->count - 180) / 120.0;
                cx = 150.0 + (330.0 - 150.0) * t;
            }
            else
                cx = 330.0;

            double angle = pShot->param_d[0];

            // 回転方向を途中で反転
            if (pEnemyShotSet->count < 180)
                angle += pShot->count * 0.020;
            else
                angle -= pShot->count * 0.020;

            // 密集→疎へ徐々に変化
            double r;
            if (pEnemyShotSet->count < 180)
                r = 28.0;
            else if (pEnemyShotSet->count < 300)
            {
                double t = (pEnemyShotSet->count - 180) / 120.0;
                r = 28.0 + (60.0 - 28.0) * t;
            }
            else
                r = 60.0;

            pShot->x = cx + cos(angle) * r;
            pShot->y = cy + sin(angle) * r;
            break;
        }

        case 2:
        {
            // 右側：大きい弾のリング
            double cx, cy = 170.0;

            if (pEnemyShotSet->count < 180)
                cx = 330.0;
            else if (pEnemyShotSet->count < 300)
            {
                double t = (pEnemyShotSet->count - 180) / 120.0;
                cx = 330.0 + (150.0 - 330.0) * t;
            }
            else
                cx = 150.0;

            double angle = pShot->param_d[0];

            if (pEnemyShotSet->count < 180)
                angle -= pShot->count * 0.015;
            else
                angle += pShot->count * 0.015;

            // 疎→密へ徐々に変化
            double r;
            if (pEnemyShotSet->count < 180)
                r = 60.0;
            else if (pEnemyShotSet->count < 300)
            {
                double t = (pEnemyShotSet->count - 180) / 120.0;
                r = 60.0 + (28.0 - 60.0) * t;
            }
            else
                r = 28.0;

            pShot->x = cx + cos(angle) * r;
            pShot->y = cy + sin(angle) * r;
            break;
        }
        }

        pShot->x += pEnemyShotSet->x - 240;
        pShot->y += pEnemyShotSet->y;

        pShot = pShot->next;
    }
}

void EnemyPat_Ebbinghaus_ChatGPT()
{
    static int dir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        dir = 1;
    }
    else
    {
        enemy.x += dir * 0.8;

        if (enemy.x > 360.0)
            dir = -1;
        if (enemy.x < 120.0)
            dir = 1;
    }

    if (count % 90 == 1)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotEbbinghaus;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = -150;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // EnemyShotSet をリストへ接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}