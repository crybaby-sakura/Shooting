// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：三色だんごの串刺し（バースト付き）
static void ShotDango(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 3色の団子を生成 (0:桜, 1:白, 2:緑)
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;

            // 串刺しに見えるよう、進行方向に対して垂直にわずかに位置をずらして配置
            pEnemyShot->x = pEnemyShotSet->x + i * 4.0 * cos(pEnemyShotSet->muki + DX_PI / 2.0);
            pEnemyShot->y = pEnemyShotSet->y + i * 4.0 * sin(pEnemyShotSet->muki + DX_PI / 2.0);
            pEnemyShot->muki = pEnemyShotSet->muki;
            pEnemyShot->param_i[0] = i; // 種類の識別用

            if (i == 0) {
                // 桜色：マゼンタの中楕円弾
                pEnemyShot->kind = img_enemyShotMediumOval[5];
                pEnemyShot->speed = 1.5; // 最初はゆっくり（串刺し状態）
            }
            else if (i == 1) {
                // 白色：白の中玉
                pEnemyShot->kind = img_enemyShotMediumBall[6];
                pEnemyShot->speed = 1.5;
            }
            else {
                // 緑色：緑の鱗弾
                pEnemyShot->kind = img_enemyShotScale[2];
                pEnemyShot->speed = 1.5;
                // 曲がり具合に個体差を持たせる (GetRand(100)は0~100を返す)
                pEnemyShot->param_d[1] = (GetRand(100) - 50) / 1000.0;
            }
            pEnemyShot->param_d[0] = pEnemyShot->muki; // 初期向きを保存

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int type = pShot->param_i[0];
        double base_muki = pShot->param_d[0];

        // count が 40 になったら「パリン！」と割れて固有の動きへ遷移
        if (pEnemyShotSet->count == 40) {
            if (type == 1) { // 代表して白の弾のタイミングで破裂音を鳴らす
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
        }

        if (pEnemyShotSet->count >= 40) {
            // 【弾けた後の固有の動き】
            if (type == 0) {
                // 桜：ひらひらと舞う
                pShot->speed = 2.5;
                double sway = sin(pEnemyShotSet->count * 0.15) * 3.0;
                pShot->x += pShot->speed * cos(base_muki) + sway * cos(base_muki + DX_PI / 2.0);
                pShot->y += pShot->speed * sin(base_muki) + sway * sin(base_muki + DX_PI / 2.0);
            }
            else if (type == 1) {
                // 白：直進性が強く高速になる
                pShot->speed = 5.5;
                pShot->x += pShot->speed * cos(base_muki);
                pShot->y += pShot->speed * sin(base_muki);
            }
            else {
                // 緑：緩やかな曲線を描く
                pShot->speed = 4.0;
                pShot->muki = base_muki + sin(pEnemyShotSet->count * 0.05 + pShot->param_d[1]) * 0.8;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else {
            // 【串刺し状態】：ゆっくり直進
            pShot->x += pShot->speed * cos(base_muki);
            pShot->y += pShot->speed * sin(base_muki);
        }

        pShot = pShot->next;
    }
}

// 弾幕：竹串レーザー（軌道予告用）
static void ShotBambooStick(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 竹串は緑色の短レーザーで表現
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 8.0; // 画面外へ速く伸ばす
        pEnemyShot->kind = img_enemyShotLaser[2]; // 緑の短レーザー

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_TricolorDango_Qwen()
{
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
    }
    else {
        // 敵の動き：左右にゆっくり往復しつつ、上下にも少し揺れる
        enemy.x = 240.0 + sin(count * 0.02) * 160.0;
        enemy.y = 70.0 + cos(count * 0.03) * 20.0;
    }

    const int T = 150;

    // 1. 予告 (count 60)
    if (count % T == 30) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 2. 竹串レーザー発射 (count 90, 100, 110)
    if (count % T >= 60 && count % T <= 120 && count % T % 5 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBambooStick;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;

        // 自機狙い + 左右に角度をずらす
        double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double offset = (count % T - 90) * 0.015;

        pEnemyShotSet->muki = base_muki + offset;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 3. だんご弾発射 (count 150, 160, 170) - 竹串の軌道を追うように遅れて発射
    if (count % T >= 60 && count % T <= 120 && count % T % 5 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDango;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;

        // 対応する竹串と同じ角度で発射
        double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double offset = (count % T - 90) * 0.015;

        pEnemyShotSet->muki = base_muki + offset;
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