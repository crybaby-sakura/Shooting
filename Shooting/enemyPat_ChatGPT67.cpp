// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>


// ============================================================
// カオス・イベントホライズン
// 第1-1回
// 弾生成処理
// ============================================================


static void ShotChaos(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;


    // --------------------------------------------------------
    // 初回のみ大量の渦弾を生成
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_extreme))
            StopSoundMem(sound_enemyShot_extreme);

        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);


        // 渦を構成する弾
        for (int i = 0; i < 96; i++)
        {
            pEnemyShot = new sEnemyShot;


            // 中心位置
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;


            // 初期角度
            double angle = DX_PI * 2.0 * i / 96.0;


            // 初期半径
            double radius = 20.0 + i * 1.2;


            // 後の更新で使用する値
            pEnemyShot->param_d[0] = angle;   // 基本角度
            pEnemyShot->param_d[1] = radius;  // 基本半径


            // 回転方向
            pEnemyShot->param_i[0] = (i % 2 == 0) ? 1 : -1;


            // 回転速度
            pEnemyShot->param_d[2] =
                0.018 + (i % 6) * 0.002;


            // 弾速
            pEnemyShot->speed = 1.0;


            // 弾種類
            // 小玉を基本に、一定間隔で種類を変える
            switch (i % 4)
            {
            case 0:
                pEnemyShot->kind =
                    img_enemyShotSmallBall[3];
                break;

            case 1:
                pEnemyShot->kind =
                    img_enemyShotSmallBall[4];
                break;

            case 2:
                pEnemyShot->kind =
                    img_enemyShotMediumBall[3];
                break;

            case 3:
                pEnemyShot->kind =
                    img_enemyShotDiamond[3];
                break;
            }

            pEnemyShot->margin = 360;

            // リストへ追加
            pEnemyShot->prev =
                pEnemyShotSet->pEnemyShotHead->prev;

            pEnemyShot->next =
                pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->pEnemyShotHead->prev->next =
                pEnemyShot;

            pEnemyShotSet->pEnemyShotHead->prev =
                pEnemyShot;
        }
    }


    sEnemyShot* pShot;

    pShot = pEnemyShotSet->pEnemyShotHead->next;


    // 徐々に回転方向を反転させる
    double reverse =
        cos(pEnemyShotSet->count * DX_PI / 120.0);


    while (pShot != pEnemyShotSet->pEnemyShotHead)
    {
        double baseAngle = pShot->param_d[0];
        double radius = pShot->param_d[1];


        // ----------------------------------------------------
        // 半径が周期的に変化
        // 安全地帯が固定されないようにする
        // ----------------------------------------------------
        double wave =
            sin(pEnemyShotSet->count * 0.035 +
                baseAngle * 3.0);


        double r =
            radius + wave * 35.0;


        if (r < 10.0)
            r = 10.0;


        // ----------------------------------------------------
        // 渦回転
        // ----------------------------------------------------
        double rot =
            pShot->param_d[2] *
            pShot->param_i[0] *
            (pEnemyShotSet->count +
                sin(pEnemyShotSet->count * DX_PI / 120.0) * 30.0);


        double angle =
            baseAngle + rot;


        pShot->x =
            pEnemyShotSet->x +
            cos(angle) * r;


        pShot->y =
            pEnemyShotSet->y +
            sin(angle) * r;

        double nextAngle =
            angle +
            pShot->param_d[2] *
            pShot->param_i[0] *
            reverse;

        pShot->muki = nextAngle + DX_PI / 2.0;

        // ----------------------------------------------------
        // 徐々に外側へ流れる
        // ----------------------------------------------------
        pShot->param_d[1] += 0.015 * 100;


        // 一定距離で内側へ戻す
        // 永遠に増殖しないようにする
        //if (pShot->param_d[1] > 250.0)
        //    pShot->param_d[1] = 20.0;


        pShot = pShot->next;
    }
}


// ============================================================
// カオス・イベントホライズン
// 第2回
// EnemyPat_TheHardest_ChatGPT()
// 敵本体制御
// ============================================================


void EnemyPat_TheHardest_ChatGPT()
{
    static int shot_count;


    // --------------------------------------------------------
    // 初期化
    // --------------------------------------------------------
    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 70.0;

        enemy.maxHp = enemy.hp = 200;

        shot_count = 0;
    }
    else
    {
        // 左右へゆっくり移動
        enemy.x =
            240.0 +
            sin(count * 0.015) * 150.0;
    }


    // --------------------------------------------------------
    // 巨大渦弾生成
    // --------------------------------------------------------
    if (count % 240 == 1)
    {
        sEnemyShotSet* pEnemyShotSet =
            new sEnemyShotSet;


        pEnemyShotSet->count = 0;


        // 初回生成
        // 更新関数を直接指定
        pEnemyShotSet->patternFunc =
            ShotChaos;


        pEnemyShotSet->x =
            enemy.x;

        pEnemyShotSet->y =
            enemy.y + 20.0;


        pEnemyShotSet->muki =
            0.0;


        pEnemyShotSet->kind =
            shot_count++;


        // 弾リスト初期化
        pEnemyShotSet->pEnemyShotHead =
            new sEnemyShot;


        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;



        // EnemyShotSetをリストへ追加
        pEnemyShotSet->prev =
            enemyShotSetHead.prev;

        pEnemyShotSet->next =
            &enemyShotSetHead;


        enemyShotSetHead.prev->next =
            pEnemyShotSet;

        enemyShotSetHead.prev =
            pEnemyShotSet;
    }


    // --------------------------------------------------------
    // 中央へ向かう追加弾
    // 渦の中に常に圧力をかける
    // --------------------------------------------------------
    if (count % 30 == 0)
    {
        sEnemyShotSet* pEnemyShotSet =
            new sEnemyShotSet;


        pEnemyShotSet->count = 0;


        pEnemyShotSet->patternFunc =
            ShotChaos;


        pEnemyShotSet->x =
            enemy.x;

        pEnemyShotSet->y =
            enemy.y + 10.0;


        pEnemyShotSet->muki =
            atan2(player.y - enemy.y,
                player.x - enemy.x);


        pEnemyShotSet->kind =
            shot_count++;


        pEnemyShotSet->pEnemyShotHead =
            new sEnemyShot;


        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;



        pEnemyShotSet->prev =
            enemyShotSetHead.prev;

        pEnemyShotSet->next =
            &enemyShotSetHead;


        enemyShotSetHead.prev->next =
            pEnemyShotSet;

        enemyShotSetHead.prev =
            pEnemyShotSet;
    }
}