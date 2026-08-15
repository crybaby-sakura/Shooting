// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：フラクタル・フォレスト（分岐する弾幕の森）
static void ShotFractalForest(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 予告音を鳴らし、プレイヤーに「これから森が生えるぞ」と意識させる
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 画面を4x4のブロックに区切り、重ならないように種弾を配置する（計16個）
        // これにより、確実に「道」が存在する開拓パズルになる
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                pEnemyShot = new sEnemyShot;
                // GetRand(60) は 0〜60 を返すので、各ブロック(120x120)内でランダムに配置
                pEnemyShot->x = 30.0 + x * 120.0 + GetRand(60);
                pEnemyShot->y = 30.0 + y * 120.0 + GetRand(60);
                pEnemyShot->speed = 0.0; // 完全に静止
                pEnemyShot->muki = 0.0;
                pEnemyShot->kind = img_enemyShotLargeBall[1]; // 大玉・黄（これから伸びる予感）

                // param_i[0] = 世代 (0:種, 1:幹, 2:枝)
                pEnemyShot->param_i[0] = 0;
                // param_i[1] = 発芽するまでのフレーム数 (90〜210フレーム)
                pEnemyShot->param_i[1] = 60 + GetRand(60);
                // 消去予約時に座標が使えなくなるため、生成時の座標を退避
                pEnemyShot->param_d[0] = pEnemyShot->x;
                pEnemyShot->param_d[1] = pEnemyShot->y;

                // 双方向リストへの接続
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレームの処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 処理中にリスト構造が変わるため、次のポインタを事前に保持
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 0) {
            // === 世代0：種弾 ===
            // 発芽タイミングに達したら、上下左右に幹弾を生成
            if (pShot->count == pShot->param_i[1]) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                double sx = pShot->param_d[0];
                double sy = pShot->param_d[1];
                pShot->x = -1000.0; // メインルーチンに消去させる

                double dirs[4] = { -DX_PI / 2.0, DX_PI / 2.0, DX_PI, 0.0 }; // 上下左右
                for (int d = 0; d < 4; d++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = sx;
                    pEnemyShot->y = sy;
                    pEnemyShot->muki = dirs[d];
                    pEnemyShot->speed = 2.0; // ゆっくり伸びる幹
                    pEnemyShot->kind = img_enemyShotMediumBall[8]; // 中玉・橙（木の幹の色）
                    pEnemyShot->param_i[0] = 1; // 世代：幹
                    pEnemyShot->param_i[1] = 30 + GetRand(30); // 枝分かれタイミング

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
        else if (pShot->param_i[0] == 1) {
            // === 世代1：幹弾 ===
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 画面端に達するか、寿命に達したら枝弾を生成
            if (pShot->count == pShot->param_i[1] ||
                pShot->x < 30.0 || pShot->x > 450.0 ||
                pShot->y < 30.0 || pShot->y > 450.0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                double bx = pShot->x;
                double by = pShot->y;
                double bm = pShot->muki;
                pShot->x = -1000.0; // メインルーチンに消去させる

                // 親の幹から ±45度の方向に枝弾を生成
                for (int d = 0; d < 2; d++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = bx;
                    pEnemyShot->y = by;
                    pEnemyShot->muki = bm + (d == 0 ? -DX_PI / 4.0 : DX_PI / 4.0);
                    pEnemyShot->speed = 2.5; // 幹より少し速く開く
                    pEnemyShot->kind = img_enemyShotSmallBall[2]; // 小玉・緑（葉の色）
                    pEnemyShot->param_i[0] = 2; // 世代：枝

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
        else if (pShot->param_i[0] == 2) {
            // === 世代2：枝弾 ===
            // 移動のみ（これ以上の分岐は行わない）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_TheMostFun_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // フラクタル・フォレストの展開
    // 前のセットが完了する頃（約6.6秒後）に次のセットを展開し、リセットの心地よさを生む
    if (count % 200 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFractalForest;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
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