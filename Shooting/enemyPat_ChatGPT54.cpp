// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void AddEnemyShot(
    sEnemyShotSet* pEnemyShotSet,
    double x,
    double y,
    double muki,
    double speed,
    int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

//------------------------------------------------------------
// 泡の更新
//  白い泡がふわふわ落下する
//------------------------------------------------------------
static void UpdateBeerBubble(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // 白い小玉だけを泡として扱う
        if (pShot->kind == img_enemyShotSmallBall[6]) {

            // 横方向を徐々に減衰
            pShot->speed *= 0.992;

            // 重力
            pShot->param_d[0] += 0.045;

            // 少し左右に揺れる
            double sway =
                sin((pShot->count + pShot->param_i[0]) * 0.12)
                * 0.03;

            pShot->muki += sway;

            // 移動
            pShot->x += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed
                + pShot->param_d[0];

            // 落下するほど泡が広がるよう少しだけ外向きへ
            pShot->speed += 0.003;
        }

        pShot = pShot->next;
    }
}

//------------------------------------------------------------
// ビール噴射
//  黄色い大玉が帯状に飛び、一定時間後に泡へ分裂する
//------------------------------------------------------------
static void ShotBeerSpray(sEnemyShotSet* pEnemyShotSet)
{
    //--------------------------------------------------------
    // 初期生成
    //--------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        // スイング角
        double sweep =
            sin(pEnemyShotSet->param_d[0]) * (55.0 / 180.0 * DX_PI);

        // ビールの流れなので密度を高めにする
        for (int i = -6; i <= 6; i++) {
            if (i % 3 != 0) continue;

            double ang =
                pEnemyShotSet->muki
                + sweep
                + i * (3.2 / 180.0 * DX_PI);

            double spd =
                4.8 + fabs(i) * 0.08;

            AddEnemyShot(
                pEnemyShotSet,
                pEnemyShotSet->x,
                pEnemyShotSet->y,
                ang,
                spd,
                img_enemyShotLargeBall[1]      // 黄色大玉
            );
        }
    }

    //--------------------------------------------------------
    // 更新
    //--------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        sEnemyShot* pNext = pShot->next;

        // 黄色大玉（ビール）だけを直進させる
        if (pShot->kind == img_enemyShotLargeBall[1]) {
            pShot->x += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed;
        }

        //--------------------------------------------
        // 約0.5秒後に泡へ分裂
        //--------------------------------------------
        if (pShot->count == 30 && pShot->kind == img_enemyShotLargeBall[1]) {

            if (CheckSoundMem(sound_enemyShot_light))
                StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            const int N = 2;
            for (int i = 0; i < N; i++) {

                double ang =
                    i * (DX_PI * 2.0 / N)
                    + (GetRand(30) - 15) / 180.0 * DX_PI;

                double spd =
                    1.0 + GetRand(80) / 100.0;

                AddEnemyShot(
                    pEnemyShotSet,
                    pShot->x,
                    pShot->y,
                    ang,
                    spd,
                    img_enemyShotSmallBall[6]
                );

                sEnemyShot* pBubble =
                    pEnemyShotSet->pEnemyShotHead->prev;

                pBubble->param_i[0] = GetRand(359);
                pBubble->param_d[0] = 0.0;
            }

            // 泡へ変化したので本体を削除
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }

        pShot = pNext;
    }

    // 泡の更新
    UpdateBeerBubble(pEnemyShotSet);
}


//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_BeerSpray_ChatGPT()
{
    static int dir;
    static int shotCount;
    static double sweepPhase;

    if (count == 1) {

        enemy.x = 240.0;
        enemy.y = 40.0;

        enemy.maxHp = 200;
        enemy.hp = 200;

        dir = 1;
        shotCount = 0;
        sweepPhase = 0.0;
    }
    else {

        enemy.x += dir * 1.4;

        if (enemy.x > 380.0) dir = -1;
        if (enemy.x < 100.0) dir = 1;

        sweepPhase += 0.08;
    }

    //--------------------------------------------------------
    // ビール噴射
    //--------------------------------------------------------
    if (count % 8 == 1) {
        if (count % 16 == 1) {
            if (CheckSoundMem(sound_enemyShot_heavy))
                StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBeerSpray;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // 基本は自機狙い
        pEnemyShotSet->muki =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);

        // ShotBeerSpray が参照するスイング位相
        pEnemyShotSet->param_d[0] = sweepPhase;

        pEnemyShotSet->kind = shotCount++;

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

    //--------------------------------------------------------
    // 噴射方向を周期的に反転
    //--------------------------------------------------------
    if (count % 180 == 0) {
        sweepPhase += DX_PI;
    }

    //--------------------------------------------------------
    // 噴射前の予告音
    //--------------------------------------------------------
    if (count % 180 == 120) {

        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);

        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}