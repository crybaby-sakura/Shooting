// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>


// ============================================================
// 三色団子スパイラル
// ============================================================
static void ShotThreeColorDango(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 1回の発射で複数の団子を生成
        for (int i = 0; i < 7; i++) {

            double angle =
                pEnemyShotSet->muki
                + (i - 3) * 0.18
                + 0.5 * sin(pEnemyShotSet->kind * 0.52);

            // 三色団子 3個
            for (int j = 0; j < 3; j++) {

                sEnemyShot* pEnemyShot = new sEnemyShot;

                // 串の方向に少しずらす
                double offset = (j - 1) * 12.0;

                pEnemyShot->x =
                    pEnemyShotSet->x
                    + cos(angle + DX_PI / 2) * offset;

                pEnemyShot->y =
                    pEnemyShotSet->y
                    + sin(angle + DX_PI / 2) * offset;


                // 団子の速度
                pEnemyShot->speed = 1.8;

                // 串方向
                pEnemyShot->muki = angle;


                // 色
                // 0:赤 2:緑 6:白
                if (j == 0) {
                    // ピンク（マゼンタ）
                    pEnemyShot->kind =
                        img_enemyShotMediumBall[5];
                }
                else if (j == 1) {
                    // 白
                    pEnemyShot->kind =
                        img_enemyShotMediumBall[6];
                }
                else {
                    // 緑
                    pEnemyShot->kind =
                        img_enemyShotMediumBall[2];
                }


                // 色番号を保存
                pEnemyShot->param_i[0] = j;

                // 初期角度
                pEnemyShot->param_d[0] = angle;

                // 初期距離
                pEnemyShot->param_d[1] = offset;


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
    }


    sEnemyShot* pShot =
        pEnemyShotSet->pEnemyShotHead->next;


    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        int color = pShot->param_i[0];

        double angle = pShot->param_d[0];


        // ----------------------------------------------------
        // 三色ごとの速度変化
        // ピンク：減速
        // 白：一定
        // 緑：加速
        // ----------------------------------------------------
        double speed = 1.8;

        if (color == 0) {
            speed *= max(0.25, 1.0 - pShot->count / 180.0);
        }
        else if (color == 2) {
            speed *= 1.0 + pShot->count / 180.0;
        }


        // ----------------------------------------------------
        // 団子がほどける動き
        // ----------------------------------------------------
        double spread = 0.08 * pShot->count;

        double offset;

        if (color == 0)
            offset = -spread;
        else if (color == 1)
            offset = 0;
        else
            offset = spread;


        pShot->x +=
            cos(angle) * speed
            - sin(angle) * offset * 0.02;

        pShot->y +=
            sin(angle) * speed
            + cos(angle) * offset * 0.02;


        // スパイラル回転
        pShot->muki =
            angle + sin(pShot->count / 50.0) * 0.2;


        pShot = pShot->next;
    }
}



// ============================================================
// 敵本体
// ============================================================
void EnemyPat_TricolorDango_ChatGPT()
{
    static int move = 1;
    static int shotCount;


    if (count == 1) {

        enemy.x = 240.0;
        enemy.y = 50.0;

        enemy.maxHp = enemy.hp = 200;

        move = 1;
        shotCount = 0;
    }
    else {

        enemy.x += move * 0.8;

        if (count % 140 == 70)
            move *= -1;
    }



    // 三色団子発射
    if (count % 35 == 1) {

        sEnemyShotSet* pEnemyShotSet =
            new sEnemyShotSet;


        pEnemyShotSet->count = 0;

        pEnemyShotSet->patternFunc =
            ShotThreeColorDango;


        pEnemyShotSet->x =
            enemy.x;

        pEnemyShotSet->y =
            enemy.y + 10;


        // プレイヤー方向を基準
        pEnemyShotSet->muki =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x
            );


        pEnemyShotSet->kind =
            shotCount++;


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