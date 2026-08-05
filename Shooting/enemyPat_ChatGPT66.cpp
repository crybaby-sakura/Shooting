#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotGravityCore(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    //-------------------------------------------------------------------------
    // 初期生成
    //-------------------------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_d[0] = 0.95;

        //=====================================================================
        // 超巨大弾（大型弾リング）
        //=====================================================================
        const int CORE_NUM = 24;

        for (int i = 0; i < CORE_NUM; i++)
        {
            pShot = new sEnemyShot;

            pShot->kind = img_enemyShotLargeBall[6];

            pShot->speed = 0.45;
            pShot->muki = pEnemyShotSet->muki;

            pShot->margin = 260.0;


            // 巨大弾リング
            pShot->param_i[0] = 2;


            // 初期角度
            pShot->param_d[0] =
                DX_PI * 2.0 * i / CORE_NUM;


            // 中心からの距離
            pShot->param_d[1] = 22.0 * 5;


            // 回転速度
            pShot->param_d[2] = 0.015;


            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;


            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        //=====================================================================
        // 公転弾
        //=====================================================================
        const int NUM = 32;

        for (int i = 0; i < NUM; i++)
        {
            pShot = new sEnemyShot;

            pShot->kind = img_enemyShotSmallBall[3];
            pShot->speed = 0.0;

            pShot->margin = 120.0;

            pShot->param_i[0] = 1;

            // 開始角
            pShot->param_d[0] = DX_PI * 2.0 * i / NUM;

            // 初期半径
            pShot->param_d[1] = 70.0 * 3;

            // 半径縮小量
            pShot->param_d[2] = 0.07;

            // 発射済み
            pShot->param_i[1] = 0;

            pShot->margin = 480;

            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    pEnemyShotSet->x += cos(pEnemyShotSet->muki) * pEnemyShotSet->param_d[0];
    pEnemyShotSet->y += sin(pEnemyShotSet->muki) * pEnemyShotSet->param_d[0];

    //---------------------------------------------------------------------
    // 公転弾
    //---------------------------------------------------------------------
    pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead)
    {
        if (pShot->param_i[0] == 2)
        {
            double ang =
                pShot->param_d[0]
                + pEnemyShotSet->count * pShot->param_d[2];


            double r =
                pShot->param_d[1]
                + 10.0 * sin(pEnemyShotSet->count * 0.05);


            pShot->x =
                pEnemyShotSet->x
                + cos(ang) * r;


            pShot->y =
                pEnemyShotSet->y
                + sin(ang) * r;


            pShot = pShot->next;
            continue;
        }

        if (pShot->param_i[0] == 1)
        {
            //-------------------------------------------------------------
            // まだ公転中
            //-------------------------------------------------------------
            if (pShot->param_i[1] == 0)
            {
                // 半径を少しずつ縮める
                double r = pShot->param_d[1] -
                    pShot->count * pShot->param_d[2];

                if (r < 18.0)
                    r = 18.0;

                // 回転角
                double ang =
                    pShot->param_d[0] +
                    pShot->count * 0.055;

                pShot->x = pEnemyShotSet->x + cos(ang) * r;
                pShot->y = pEnemyShotSet->y + sin(ang) * r;

                //---------------------------------------------------------
                // 一斉射出
                //---------------------------------------------------------
                if (pEnemyShotSet->count >= 180)
                {
                    if (CheckSoundMem(sound_enemyShot_medium))
                        StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium,
                        DX_PLAYTYPE_BACK);

                    pShot->param_i[1] = 1;

                    // 接線方向へ飛ばす
                    pShot->muki = ang + DX_PI / 2.0;
                    pShot->speed = 2.8;

                    // 発射後は通常弾になる
                    pShot->kind = img_enemyShotMediumBall[1];
                }
            }
            //-------------------------------------------------------------
            // 射出後
            //-------------------------------------------------------------
            else
            {
                pShot->x += cos(pShot->muki) * pShot->speed;
                pShot->y += sin(pShot->muki) * pShot->speed;
            }
        }

        pShot = pShot->next;
    }

    //---------------------------------------------------------------------
    // 巨大弾も最後は通常弾として飛ばす
    //---------------------------------------------------------------------
    //if (pCore->count >= 240)
    //{
    //    pCore->x += cos(pCore->muki) * 2.2;
    //    pCore->y += sin(pCore->muki) * 2.2;
    //}
}

//=============================================================================
// 敵本体
//=============================================================================
void EnemyPat_HugeBullet_ChatGPT()
{
    static int moveDir;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 45.0;

        enemy.maxHp = enemy.hp = 200;

        moveDir = 1;
    }
    else
    {
        // ゆっくり左右移動
        enemy.x += moveDir * 0.8;

        if (enemy.x > 360.0)
            moveDir = -1;
        else if (enemy.x < 120.0)
            moveDir = 1;
    }

    //---------------------------------------------------------------------
    // 重力核発射
    //---------------------------------------------------------------------
    if (count % 240 == 1)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGravityCore;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;

        // プレイヤー方向へ発射
        pEnemyShotSet->muki =
            atan2(player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);

        pEnemyShotSet->kind = 0;

        //-----------------------------------------------------------------
        // リスト初期化
        //-----------------------------------------------------------------
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;

        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        //-----------------------------------------------------------------
        // ShotSet登録
        //-----------------------------------------------------------------
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;

        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}