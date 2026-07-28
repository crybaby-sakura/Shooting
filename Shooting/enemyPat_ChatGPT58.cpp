// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const double HOLE_X = 240.0;
static const double HOLE_Y = 240.0;

static void ShotAntlion(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int ring = 0; ring < 8; ring++)
        {
            double angle = ring / 8.0 * DX_PI * 2.0;

            for (int i = 0; i < 18; i++)
            {
                sEnemyShot* pShot = new sEnemyShot;

                pShot->kind = img_enemyShotSmallBall[1];

                pShot->param_d[0] = angle;
                pShot->param_d[1] = 260.0 + i * 6.0;
                pShot->param_d[2] = (GetRand(100) - 50) / 5000.0;

                pShot->param_i[0] = GetRand(60);

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
        if (pShot->param_i[1] == 0)
        {
            double t = pShot->count + pShot->param_i[0];

            double r = pShot->param_d[1] - t * 0.95;

            if (r < 12.0)
            {
                pShot->param_i[1] = 1;

                pShot->param_i[2] = pShot->count;
                pShot->param_d[3] = pShot->param_d[0] + t * 0.08;
            }
            else
            {
                double rotate =
                    0.005 + (260.0 - r) * 0.00008;

                double ang =
                    pShot->param_d[0]
                    + t * (rotate + pShot->param_d[2]);

                pShot->x = HOLE_X + cos(ang) * r;
                pShot->y = HOLE_Y + sin(ang) * r;
            }
        }

        if (pShot->param_i[1])
        {
            double r = (pShot->count - pShot->param_i[2]) * 5.0;

            if (r < 0.0)
                r = 0.0;

            pShot->x = HOLE_X + cos(pShot->param_d[3]) * r;
            pShot->y = HOLE_Y + sin(pShot->param_d[3]) * r;
        }

        pShot = pShot->next;
    }
}

void EnemyPat_Antlion_ChatGPT()
{
    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 50.0;

        enemy.maxHp = 200;
        enemy.hp = 200;
    }

    enemy.x = 240.0 + sin(count * 0.02) * 90.0;

    if (count % 420 == 1)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAntlion;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;

        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}