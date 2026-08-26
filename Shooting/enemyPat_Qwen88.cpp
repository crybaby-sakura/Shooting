// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：蚊取り線香（旋回する緑煙の渦）
static void ShotMosquitoCoil(sEnemyShotSet* pEnemyShotSet)
{
    // 初回のみ：螺旋状に弾を配置し、効果音を鳴らす
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double max_theta = 6.0 * DX_PI; // 3巻き
        double step = 0.10;             // 弾の間隔（ラジアン）

        for (double theta = 0; theta <= max_theta; theta += step) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->count = 0;

            // 半径は中心に近いほど小さく（燃え進む表現）
            double r = 30.0 + (400.0 - 30.0) * (theta / max_theta);

            pShot->param_d[0] = theta;      // 現在の角度
            pShot->param_d[1] = r;          // 現在の半径
            pShot->param_d[2] = 0.02;       // 回転速度
            pShot->param_d[3] = 1.5;        // 半径の縮小速度（燃焼速度）
            pShot->param_i[0] = 0;          // 0: 線香本体

            // 一番外側（theta が max_theta に近い）を「火種」にする
            if (theta >= max_theta - step) {
                pShot->kind = img_enemyShotMediumBall[0]; // 0:赤色の中玉
                pShot->param_i[0] = 1;                    // 1: 火種フラグ
            }
            else {
                pShot->kind = img_enemyShotMediumBall[2]; // 2:緑色の中玉
            }
            pShot->margin = 240;

            // リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // --- 線香本体 または 火種 の処理 ---
        if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
            if (pShot->param_d[1] > 0.0) {
                // 回転と縮小（燃焼）
                pShot->param_d[0] += pShot->param_d[2];
                pShot->param_d[1] -= pShot->param_d[3];

                pShot->x = pEnemyShotSet->x + pShot->param_d[1] * cos(pShot->param_d[0]);
                pShot->y = pEnemyShotSet->y + pShot->param_d[1] * sin(pShot->param_d[0]);

                // 【煙の生成】線香本体から一定確率で煙を発生させる
                if (pShot->param_i[0] == 0 && pEnemyShotSet->count % 80 == 0 && GetRand(3) == 0) {
                    sEnemyShot* pSmoke = new sEnemyShot;
                    pSmoke->count = pEnemyShotSet->count;
                    pSmoke->x = pShot->x;
                    pSmoke->y = pShot->y;
                    pSmoke->kind = img_enemyShotSmallBall[6]; // 6:白色の小玉
                    pSmoke->param_i[0] = 2;                   // 2: 煙フラグ
                    pSmoke->param_d[0] = pShot->x;            // 発生基準X
                    pSmoke->param_d[1] = pShot->y;            // 発生基準Y
                    pSmoke->param_d[2] = GetRand(628) / 100.0; // ゆらぎの位相 (0.00 ~ 6.28)
                    pSmoke->param_d[3] = 1.0 + GetRand(100) / 100.0; // 上昇速度 (1.0 ~ 2.0)
                    pSmoke->margin = 240;

                    pSmoke->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pSmoke->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pSmoke;
                    pEnemyShotSet->pEnemyShotHead->prev = pSmoke;
                }
            }
            else {
                // 燃え尽きた（半径が0以下）ら、メインルーチンの画面外消去処理に任せるため座標を遠くへ飛ばす
                pShot->x = -9999.0;
                pShot->y = -9999.0;
            }

            // 【火種の狙い撃ち】火種は定期的に自機を狙う弾を吐き出す
            if (pShot->param_i[0] == 1 && pEnemyShotSet->count % 45 == 0) {
                sEnemyShot* pAim = new sEnemyShot;
                pAim->count = 0;
                pAim->kind = img_enemyShotSmallBall[0]; // 0:赤色の小玉
                pAim->x = pShot->x;
                pAim->y = pShot->y;
                pAim->muki = atan2(player.y - pAim->y, player.x - pAim->x);
                pAim->speed = 3.5;
                pAim->param_i[0] = 3; // 3: 狙い撃ち弾フラグ
                pAim->margin = 240;

                pAim->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pAim->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pAim;
                pEnemyShotSet->pEnemyShotHead->prev = pAim;
            }
        }
        // --- 煙 の処理 ---
        else if (pShot->param_i[0] == 2) {
            int life = pEnemyShotSet->count - pShot->count;
            if (life > 120) {
                // 寿命が来たら画面外へ飛ばして消去対象にする
                pShot->x = -9999.0;
                pShot->y = -9999.0;
            }
            else {
                // ゆらぎながら上昇する
                pShot->y -= pShot->param_d[3];
                pShot->x = pShot->param_d[0] + sin(pEnemyShotSet->count * 0.05 + pShot->param_d[2]) * 15.0;
            }
        }
        // --- 狙い撃ち弾 の処理 ---
        else if (pShot->param_i[0] == 3) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_MosquitoCoil_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵は左右にゆっくり往復移動する
        enemy.x += 1.2 * (double)muki;
        if (enemy.x > 300.0 || enemy.x < 180.0) {
            muki *= -1;
        }
    }

    // 一定間隔で蚊取り線香弾幕を発射
    if (count % 360 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMosquitoCoil;
        pEnemyShotSet->x = 240;
        pEnemyShotSet->y = 240;
        pEnemyShotSet->muki = 0.0; // 螺旋の基準角度（今回は固定）
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}