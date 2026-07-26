// enemyPat_rule30.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotRule30(sEnemyShotSet* pEnemyShotSet)
{
    // count==0 のときだけ弾生成
    if (pEnemyShotSet->count == 0) {

        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int CELL_NUM = 31;
        const double CELL_W = 14.0;
        const double SPEED = 2.2;

        for (int i = 0; i < CELL_NUM; i++) {

            if (((pEnemyShotSet->param_i[0] >> i) & 1) == 0)
                continue;

            sEnemyShot* pShot = new sEnemyShot;

            pShot->x = pEnemyShotSet->x + (i - CELL_NUM / 2) * CELL_W;
            pShot->y = pEnemyShotSet->y;

            pShot->muki = DX_PI / 2.0;
            pShot->speed = SPEED;
            pShot->kind = img_enemyShotSmallBall[1];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        pShot->x += cos(pShot->muki) * pShot->speed;
        pShot->y += sin(pShot->muki) * pShot->speed;

        pShot = pShot->next;
    }
}

void EnemyPat_Rule30_ChatGPT()
{
    static int muki;

    static const int CELL_NUM = 31;
    static int cell[CELL_NUM];
    static int nextCell[CELL_NUM];
    if (count == 1) {

        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        muki = 1;

        for (int i = 0; i < CELL_NUM; i++)
            cell[i] = 0;

        // 初期状態：中央のみ生存
        cell[CELL_NUM / 2] = 1;
    }
    else {

        enemy.x += muki * 0.8;

        if (enemy.x < 80.0) {
            enemy.x = 80.0;
            muki = 1;
        }
        if (enemy.x > 400.0) {
            enemy.x = 400.0;
            muki = -1;
        }
    }

    // 12フレームごとに1世代発射
    if (count % 12 == 1) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRule30;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;

        int bits = 0;

        for (int i = 0; i < CELL_NUM; i++) {
            if (cell[i])
                bits |= (1 << i);
        }

        pEnemyShotSet->param_i[0] = bits;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // Rule30更新
        for (int i = 0; i < CELL_NUM; i++) {

            int L = (i == 0) ? 0 : cell[i - 1];
            int C = cell[i];
            int R = (i == CELL_NUM - 1) ? 0 : cell[i + 1];

            int pat = (L << 2) | (C << 1) | R;
            // Rule30
            // 111 110 101 100 011 010 001 000
            //  0   0   0   1   1   1   1   0
            switch (pat) {
            case 7: nextCell[i] = 0; break;
            case 6: nextCell[i] = 0; break;
            case 5: nextCell[i] = 0; break;
            case 4: nextCell[i] = 1; break;
            case 3: nextCell[i] = 1; break;
            case 2: nextCell[i] = 1; break;
            case 1: nextCell[i] = 1; break;
            default: nextCell[i] = 0; break;
            }
        }

        for (int i = 0; i < CELL_NUM; i++)
            cell[i] = nextCell[i];
    }
}