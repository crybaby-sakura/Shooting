// enemyPat_Tmp.cpp
// サインポール・スパイラル（第一回）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

#ifndef DX_TWO_PI
#define DX_TWO_PI (DX_PI * 2.0)
#endif

//----------------------------------------------------------
// サインポールの二重らせん
//----------------------------------------------------------
static void ShotSignPole(sEnemyShotSet* pEnemyShotSet)
{
    //------------------------------------------------------
    // 初回のみ弾生成
    //------------------------------------------------------
    if (pEnemyShotSet->count == 0)
    {
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int DIV = 42;

        for (int i = 0; i < DIV; i++)
        {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->kind = img_enemyShotMediumBall[0]; // 赤
            pShot->speed = 0.0;
            pShot->margin = 200;

            // 基準角
            pShot->param_d[0] = DX_TWO_PI * i / DIV;

            // 半径
            pShot->param_d[1] = 26.0;

            // 進行方向
            pShot->param_d[2] = pEnemyShotSet->muki;

            // 軸方向距離
            pShot->param_d[3] = -180.0 + i * 8.5;

            // 位相
            pShot->param_d[4] = 0.0;

            // 赤側
            pShot->param_i[0] = 0;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        for (int i = 0; i < DIV; i++)
        {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->kind = img_enemyShotMediumBall[4]; // 青
            pShot->speed = 0.0;
            pShot->margin = 200;

            pShot->param_d[0] = DX_TWO_PI * i / DIV;
            pShot->param_d[1] = 26.0;
            pShot->param_d[2] = pEnemyShotSet->muki;
            pShot->param_d[3] = -180.0 + i * 8.5;

            // 180度ずらす
            pShot->param_d[4] = DX_PI;

            // 青側
            pShot->param_i[0] = 1;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        pEnemyShotSet->param_d[0] = 0.0;
    }

    //------------------------------------------------------
    // サインポール回転
    //------------------------------------------------------
    double dir = pEnemyShotSet->muki;

    // 180フレームごとに回転方向反転
    double rotSign =
        ((pEnemyShotSet->count / 180) & 1) ? -1.0 : 1.0;
    pEnemyShotSet->param_d[0] += rotSign * 0.05;

    // 半径をゆっくり変化
    double radius =
        26.0 +
        8.0 * sin(pEnemyShotSet->count / 70.0);

    double vx = cos(dir);
    double vy = sin(dir);

    double px = -sin(dir);
    double py = cos(dir);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead)
    {
        double t =
            pShot->param_d[3]
            + pEnemyShotSet->count * 2.4;

        double ang =
            pShot->param_d[0]
            + pEnemyShotSet->param_d[0]
            + pShot->param_d[4];
        
        double cx = pEnemyShotSet->x + vx * t;
        double cy = pEnemyShotSet->y + vy * t;

        pShot->x =
            cx +
            cos(ang) * radius * px;

        pShot->y =
            cy +
            cos(ang) * radius * py;

        // 螺旋を立体的に見せるため、上下方向にも振る
        pShot->x += sin(ang) * radius * 0.35 * vx;
        pShot->y += sin(ang) * radius * 0.35 * vy;

        // 回転に合わせて色を切り替え、サインポールらしい
        // 赤・青・白の流れを作る
        double phase = fmod(ang, DX_TWO_PI);
        if (phase < 0.0) phase += DX_TWO_PI;

        if (phase < DX_PI / 3.0)
            pShot->kind = img_enemyShotMediumBall[6];      // 白
        else if (pShot->param_i[0] == 0)
            pShot->kind = img_enemyShotMediumBall[0];      // 赤
        else
            pShot->kind = img_enemyShotMediumBall[4];      // 青

        pShot = pShot->next;
    }
}

//----------------------------------------------------------
// 敵本体
//----------------------------------------------------------
void EnemyPat_SignPole_ChatGPT()
{
    static int moveDir;
    static int shotCount;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;

        moveDir = 1;
        shotCount = 0;
    }
    else
    {
        enemy.x += moveDir * 0.9;

        if (enemy.x > 340.0)
            moveDir = -1;
        if (enemy.x < 140.0)
            moveDir = 1;
    }

    //------------------------------------------------------
    // 新しいサインポールを生成
    //------------------------------------------------------
    if (count % 120 == 1)
    {
        for (int i = -1; i <= 1; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSignPole;

            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 12.0;

            pEnemyShotSet->muki =
                atan2(
                    player.y - pEnemyShotSet->y,
                    player.x - pEnemyShotSet->x);
            pEnemyShotSet->muki += i * DX_PI / 6;

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
    }
}