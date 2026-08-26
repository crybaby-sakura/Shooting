// enemyPat_katori.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：蚊取り線香（渦巻燻煙）
static void ShotKatori(sEnemyShotSet* pEnemyShotSet)
{
    // 汎用パラメータの用途定義:
    // param_d[0] : 渦巻き全体のベース回転角
    // param_d[1] : 燃焼の現在の角度(theta)。最大値から0に向かって減る。

    // 螺旋の設定
    const double a = 15.0; // 初期の半径(内側の空白)
    const double b = 60.0 / (2.0 * DX_PI); // 1巻きごとに50ピクセル広がる
    const double max_theta = 7.0 * DX_PI; // 3.5巻き

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_d[0] = 0.0;
        pEnemyShotSet->param_d[1] = max_theta;

        // 1. 線香の帯（渦巻き）を展開
        // 角度0.04ラジアン刻みで小玉を配置
        for (double th = 0.0; th <= max_theta; th += 0.06) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->param_i[0] = 0; // 種類(0:線香, 1:火種, 2:煙)
            pEnemyShot->param_d[0] = th; // 自身が配置される極座標系の角度theta
            pEnemyShot->param_d[1] = a + b * th; // 自身が配置される極座標系の半径r

            pEnemyShot->kind = img_enemyShotSmallBall[2]; // 緑色の小玉
            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 2. 火種の生成
        sEnemyShot* pFire = new sEnemyShot;
        pFire->param_i[0] = 1;
        pFire->kind = img_enemyShotLargeBall[8]; // 橙色の大玉
        pFire->margin = 240;

        pFire->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pFire->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pFire;
        pEnemyShotSet->pEnemyShotHead->prev = pFire;
    }

    // 全体をゆっくり回転させる
    pEnemyShotSet->param_d[0] += 0.006;
    // 火種を外側から内側へ移動させる（燃焼）
    pEnemyShotSet->param_d[1] -= 0.025;

    double base_angle = pEnemyShotSet->param_d[0];
    double fire_theta = pEnemyShotSet->param_d[1];

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // ----- 線香の帯の動作 -----
            double th = pShot->param_d[0];
            double r = pShot->param_d[1];

            // 回転に合わせて位置を更新
            pShot->x = pEnemyShotSet->x + r * cos(th + base_angle);
            pShot->y = pEnemyShotSet->y + r * sin(th + base_angle);

            // 燃焼判定 (火種が通過したら煙に変容)
            if (th > fire_theta) {
                pShot->param_i[0] = 2; // 煙フラグへ変更

                // 煙の見た目をランダムにばらつかせる (白系の弾を使用)
                if (GetRand(1) == 0) pShot->kind = img_enemyShotMediumOval[6]; // 白の中楕円
                else                 pShot->kind = img_enemyShotMediumBall[6]; // 白の中玉

                // 煙の拡散方向 (接線から少し外側へランダムに膨らむように)
                pShot->muki = th + base_angle + DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
                pShot->speed = (60 + GetRand(40)) / 100.0; // 0.6 ~ 1.0 の低速

                // 燃え広がる音 (毎フレーム鳴るとうるさいので適度に間引く)
                if (GetRand(15) == 0) {
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }
            }
        }
        else if (pShot->param_i[0] == 1) {
            // ----- 火種の動作 -----
            if (fire_theta > 0) {
                // 現在の燃焼位置へ移動
                double r = a + b * fire_theta;
                pShot->x = pEnemyShotSet->x + r * cos(fire_theta + base_angle);
                pShot->y = pEnemyShotSet->y + r * sin(fire_theta + base_angle);
            }
            else {
                // 燃え尽きたら火種を画面外へ飛ばして消去させる
                pShot->y -= 1000.0;
            }
        }
        else if (pShot->param_i[0] == 2) {
            // ----- 煙の動作 -----
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 少しずつ減速して画面内に滞留させる
            if (pShot->speed > 0.15) {
                pShot->speed -= 0.005;
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_MosquitoCoil_Gemini()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        // 渦巻きを画面全体に大きく展開するため、敵を少し下げて画面中央付近に配置
        enemy.x = 240.0;
        enemy.y = 200.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // ゆっくり左右に揺れる（線香のゆらぎ）
        enemy.x = 240.0 + 100.0 * sin(count * 0.015);
        enemy.y = 200.0 + 50.0 * sin(count * 0.025);
    }

    // カウント60で一度だけ渦巻き弾幕セットを生成
    if (count % 330 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKatori;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}