// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：蚊取り線香
static void ShotSpiralIncense(sEnemyShotSet* pEnemyShotSet)
{
    // --- 初期化フェーズ ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int shotsPerLoop = 15 * 3;   // 1周あたりの弾数
        int loops = 5 - 2;           // 渦巻きの周数
        double radiusStep = 8.0; // 1周ごとの半径の増加量
        int totalShots = shotsPerLoop * loops;

        for (int i = 0; i < totalShots; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 渦巻きの配置計算 (アルキメデスの渦巻き)
            double angle = (double)i / shotsPerLoop * 2.0 * DX_PI;
            double radius = (((double)i / shotsPerLoop) + 1) * radiusStep; // 中心を空けるため+1

            // パラメータに保持
            pEnemyShot->param_d[0] = angle;          // 現在の角度
            pEnemyShot->param_d[1] = radius;         // 半径
            pEnemyShot->param_d[2] = 0.015;          // 回転速度
            // 外側から燃えるようにするための遅延フレーム数 (外側ほど0に近い)
            pEnemyShot->param_i[0] = (totalShots - 1 - i) * 5;
            pEnemyShot->param_i[1] = 0;              // 0:渦巻き中, 1:飛散済み

            // 初期位置をセット
            pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);
            pEnemyShot->speed = 0.0;
            pEnemyShot->muki = 0.0;

            // 緑色の小玉(2.5x2.5)を線香の本体として使用
            pEnemyShot->kind = img_enemyShotSmallBall[2]; // 2:緑

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        return;
    }

    // --- 更新フェーズ ---

    // 線香全体をゆっくり下に落下させる
    pEnemyShotSet->y += 0.5;

    int burnStartFrame = 60; // 燃え始めるまでの待機時間

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        if (pShot->param_i[1] == 0) {
            // 燃焼判定 (一定フレーム経過後、外側から順に燃える)
            if (pEnemyShotSet->count >= burnStartFrame + pShot->param_i[0]) {
                // 燃焼！ランダムな方向に飛び散る
                pShot->param_i[1] = 1; // 飛散フラグを立てる

                // GetRand(x) は 0~x なので、角度を 0~6.28 にするには GetRand(628) / 100.0
                pShot->muki = GetRand(628) / 100.0;
                pShot->speed = 1.5 + GetRand(10) / 10.0; // 1.5 ~ 2.5
                pShot->kind = img_enemyShotSmallBall[0];  // 0:赤(火の粉)

                // 煙を生成 (白い極小弾をほぼ真上にゆっくり飛ばす)
                sEnemyShot* pSmoke = new sEnemyShot;
                pSmoke->x = pShot->x;
                pSmoke->y = pShot->y;
                // ほぼ真上(-PI/2)に、少しランダムな揺れを加える
                // GetRand(20) は 0~20 なので、-10~10 の揺れになる
                pSmoke->muki = -DX_PI / 2.0 + (GetRand(20) - 10) / 100.0;
                pSmoke->speed = 0.5 + GetRand(5) / 10.0; // 0.5 ~ 1.0
                pSmoke->kind = img_enemyShotSmallBall[6]; // 6:白(煙の代用)
                pSmoke->param_i[1] = 1; // 飛散済みとしてマーク(回転させないため)

                // 煙をリストに追加
                pSmoke->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSmoke->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSmoke;
                pEnemyShotSet->pEnemyShotHead->prev = pSmoke;

                // このフレームから直進を開始するため、下の直進処理に落とす
            }
            else {
                // 渦巻きの回転処理
                pShot->param_d[0] += pShot->param_d[2]; // 角度を加算
                pShot->x = pEnemyShotSet->x + pShot->param_d[1] * cos(pShot->param_d[0]);
                pShot->y = pEnemyShotSet->y + pShot->param_d[1] * sin(pShot->param_d[0]);

                pShot = pShot->next; // 次の弾へ進めてからcontinue
                continue;
            }
        }

        // 飛散済みの弾(火の粉と煙)の直進処理
        if (pShot->param_i[1] == 1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_MosquitoCoil_Zai()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 1.48 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 60フレーム目に蚊取り線香を1回だけ生成
    if (count % 135 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiralIncense;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0; // 敵の少し下に生成
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}