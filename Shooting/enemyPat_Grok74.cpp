// enemyPat_Tmp.cpp
// たらこスパゲッティ弾幕パターン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン：たらこ絡みスパゲッティ
static void ShotTarakoSpaghetti(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初期化：スパゲッティの麺（短レーザー）を複数本生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 7本のスパゲッティ（短レーザー）を扇状＋少しカーブするように生成
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // プレイヤー方向を基準に扇状に広げる
            pEnemyShot->muki = pEnemyShotSet->muki + (i - 3) * 0.18;
            pEnemyShot->speed = 2.2 + GetRand(80) / 100.0;
            // たらこスパゲッティらしい橙色の短レーザー
            pEnemyShot->kind = img_enemyShotLaser[8];         // 8:橙
            // param_d[0] : カーブの強さ（左右交互）
            pEnemyShot->param_d[0] = ((i % 2) * 2 - 1) * (0.003 + GetRand(5) / 1000.0);
            // param_i[0] : 麺としての識別（0=スパゲッティ）
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // たらこが弾けるタイミング（麺が伸びた後）
    // count はメインルーチンでインクリメントされるので、ここでは参照のみ
    if (pEnemyShotSet->count == 35 || pEnemyShotSet->count == 65 || pEnemyShotSet->count == 95) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 現在存在するスパゲッティの位置からたらこ粒を散弾させる
        // （イテレーション中にリストを変更しないよう、先に位置を控える）
        double posX[16], posY[16];
        int num = 0;
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead && num < 16) {
            if (pShot->param_i[0] == 0) {   // スパゲッティのみ
                posX[num] = pShot->x;
                posY[num] = pShot->y;
                num++;
            }
            pShot = pShot->next;
        }

        // 控えた位置からたらこ（小玉）を放射
        for (int n = 0; n < num; n++) {
            int scatterNum = 4 + GetRand(2);   // 4〜6発
            for (int j = 0; j < scatterNum; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = posX[n];
                pEnemyShot->y = posY[n];
                // ほぼ全方向に散らす
                pEnemyShot->muki = (GetRand(360) / 180.0) * DX_PI;
                pEnemyShot->speed = 2.2 + GetRand(120) / 100.0;   // 2.2〜3.4
                // たらこのイメージでマゼンタ or 赤の小玉
                pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[5] : img_enemyShotSmallBall[0];
                pEnemyShot->param_i[0] = 1;   // たらこ識別

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 全弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // スパゲッティ：ゆるやかにカーブさせながら前進
            pShot->muki += pShot->param_d[0];
            // 速度を徐々に落とす（麺が伸びきった感じ）
            if (pShot->count > 20 && pShot->speed > 0.6) {
                pShot->speed -= 0.002;
            }
        }
        // 共通の移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_TarakoSpaghetti_Grok()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;   // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 80.0)  muki = 1;
        if (enemy.x > 400.0) muki = -1;
    }

    // 一定間隔でたらこスパゲッティ弾幕を発射
    if (count % 110 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTarakoSpaghetti;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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