// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotCounter(sEnemyShotSet* pEnemyShotSet)
{
    // count==0 の瞬間だけ生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int num = pEnemyShotSet->param_i[0];
        double base = pEnemyShotSet->muki;
        double spread = DX_PI * 2.0 / 3.0; // 120°

        for (int i = 0; i < num; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            double t;
            if (num == 1) t = 0.5;
            else          t = (double)i / (double)(num - 1);

            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;

            pShot->muki = base - spread * 0.5 + spread * t;

            // 最初は遅く、徐々に加速
            pShot->speed = 0.8;

            // 白い中玉
            pShot->kind = img_enemyShotMediumBall[6];

            pShot->param_d[0] = 3.2; // 最終速度

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // 約20Fかけて加速
        if (pShot->count < 20) {
            double t = (double)pShot->count / 20.0;
            pShot->speed = 0.8 + (pShot->param_d[0] - 0.8) * t;
        }
        else {
            pShot->speed = pShot->param_d[0];
        }

        pShot->x += cos(pShot->muki) * pShot->speed;
        pShot->y += sin(pShot->muki) * pShot->speed;

        pShot = pShot->next;
    }
}

void EnemyPat_Counter_ChatGPT()
{
    static int moveDir;
    static int prevHp;
    static int revengeLevel;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;

        moveDir = 1;
        prevHp = enemy.hp;
        revengeLevel = 0;
    }

    // 左右移動
    enemy.x += moveDir * 1.0;
    if (enemy.x < 80.0) moveDir = 1;
    if (enemy.x > 400.0) moveDir = -1;

    // 一定時間被弾しなければ撃ち返し密度リセット
    if (count % 90 == 0) {
        revengeLevel = 0;
    }

    // ダメージを受けた瞬間
    if (enemy.hp < prevHp) {

        revengeLevel++;
        if (revengeLevel > 6) revengeLevel = 6;

        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = ShotCounter;

        pSet->x = enemy.x;
        pSet->y = enemy.y + 8.0;

        pSet->muki = atan2(
            player.y - pSet->y,
            player.x - pSet->x);

        // 5→7→9→11→13→15→17発
        pSet->param_i[0] = 5 + revengeLevel * 2;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    prevHp = enemy.hp;
}