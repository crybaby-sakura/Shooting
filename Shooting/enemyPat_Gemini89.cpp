// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：偽りのフリーズと画面接近（スクリーンスマッシュ）
static void ShotJumpscare(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int t = pEnemyShotSet->count;

    // フェーズ1：恐怖の予兆（視野の狭窄）
    // 画面外周から自機に向けて、暗い黒の中玉がじわじわ迫る
    if (t >= 0 && t < 150) {
        // 音がうるさくなりすぎないよう間隔をあけて再生
        if (t % 15 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 自機を取り囲むように弾を発生させる
        if (t % 3 == 0) {
            pEnemyShot = new sEnemyShot;
            // 自機から半径400の位置にランダムな角度で生成
            // GetRand(359) は 0 から 359 の 360 種類の整数を返す
            double angle = GetRand(359) * DX_PI / 180.0;
            double r = 400.0;
            pEnemyShot->x = player.x + r * cos(angle);
            pEnemyShot->y = player.y + r * sin(angle);

            // 自機の方向へ進む
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            // 速度は 1.5 ~ 2.5 の低速
            pEnemyShot->speed = (150 + GetRand(100)) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumBall[7]; // 黒の中玉
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // フェーズ2：完全な静止（サスペンスの溜め）
    if (t == 150) {
        // サウンドを止めることで、フリーズしたかのような不気味な静寂を演出
        StopSoundMem(sound_enemyShot_light);
        StopSoundMem(sound_enemyShot_medium);
        StopSoundMem(sound_enemyShot_heavy);

        // 画面上のこのセットの全弾を強制停止
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->speed = 0.0;
            pShot = pShot->next;
        }
    }

    // 150〜189 は完全に停止した状態（約0.6秒のスキ）

    // フェーズ3：ジャンプスケア（画面手前への爆発）
    if (t == 190) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double jumpX = player.x;
        // 自機の少し上（プレイヤーの視線が集中しやすい位置）
        double jumpY = player.y - 150.0;

        // 突如現れる巨大な赤い玉（見開いた目のような印象）
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = jumpX;
        pEnemyShot->y = jumpY;
        pEnemyShot->muki = 0;
        pEnemyShot->speed = 0;
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤の大玉
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 白い菱形弾と中玉を超高速で全方位にばら撒く（3D的な画面接近感を表現）
        for (int i = 0; i < 90; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = jumpX;
            pEnemyShot->y = jumpY;
            pEnemyShot->muki = (i * 4) * DX_PI / 180.0; // 4度刻みで360度

            // 速度にバラつきを持たせて奥行きを表現 (8.0 ~ 18.0)
            pEnemyShot->speed = 8.0 + (GetRand(100) / 10.0) - 3;

            // 速度が速いものは菱形、少し遅いものは中玉にして視覚的な層を作る
            if (pEnemyShot->speed > 13.0) {
                pEnemyShot->kind = img_enemyShotDiamond[6]; // 白の菱形弾
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白の中玉
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // フェーズ3-2：直後の追撃
    if (t == 210) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 画面の四隅から、驚いて止まった自機を正確に狙う高速レーザー
        double cornerX[4] = { 0.0, 480.0, 0.0, 480.0 };
        double cornerY[4] = { 0.0, 0.0, 480.0, 480.0 };

        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cornerX[i];
            pEnemyShot->y = cornerY[i];
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->speed = 12.0; // レーザーなので高速
            pEnemyShot->kind = img_enemyShotLaser[0]; // 赤の短レーザー

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾の移動処理（フェーズ2でスピード0にされた弾は動かなくなる）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_JumpScare_Gemini()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 敵本体はフラフラ動かす（静止していると不自然なため）
    enemy.x += sin(count * DX_PI / 180.0) * 0.5;

    // 400フレーム周期で弾幕セットを起動
    if (count % 400 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotJumpscare;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0;
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