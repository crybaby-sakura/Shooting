#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：焼き栗がはじけるパターン
static void ShotChestnutBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ===== 出現フレーム =====
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 4個の焼き栗を横に並べて出現させる
        const int numChestnut = 4;
        for (int i = 0; i < numChestnut; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (i - 1.5) * 48.0 + (GetRand(20) - 10);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(16) - 8);
            pEnemyShot->muki = DX_PI / 2.0;                 // 真下
            pEnemyShot->speed = 1.3 + GetRand(40) / 100.0;  // ゆっくり落下
            pEnemyShot->kind = img_enemyShotMediumBall[8];  // 橙の中玉 = 栗
            pEnemyShot->param_i[0] = 1;                     // 1 = まだはじけていない栗

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ===== 弾の移動 =====
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }

    // ===== はじける瞬間（約50フレーム後） =====
    if (pEnemyShotSet->count == 50) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 現在存在する「栗」から放射状に弾を生成
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {  // まだはじけていない栗だけ処理
                // 中身：黄の小玉を12方向に放射
                for (int i = 0; i < 12*2; i++) {
                    pEnemyShot = new sEnemyShot;
                    double ang = i * (2.0 * DX_PI / 12.0/2) + (GetRand(20) - 10) / 180.0 * DX_PI;
                    pEnemyShot->x = pShot->x;
                    pEnemyShot->y = pShot->y;
                    pEnemyShot->muki = ang;
                    pEnemyShot->speed = 2.8 + GetRand(60) / 100.0;
                    pEnemyShot->kind = img_enemyShotSmallBall[1];  // 黄の小玉
                    pEnemyShot->param_i[0] = 0;

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }

                // 皮の破片：橙の鱗弾を6方向に少し遅めに飛ばす
                for (int i = 0; i < 6*2; i++) {
                    pEnemyShot = new sEnemyShot;
                    double ang = i * (2.0 * DX_PI / 6.0/2) + DX_PI / 6.0/2 + (GetRand(30) - 15) / 180.0 * DX_PI;
                    pEnemyShot->x = pShot->x;
                    pEnemyShot->y = pShot->y;
                    pEnemyShot->muki = ang;
                    pEnemyShot->speed = 2.0 + GetRand(50) / 100.0;
                    pEnemyShot->kind = img_enemyShotScale[8];  // 橙の鱗弾
                    pEnemyShot->param_i[0] = 0;

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }

                // 元の栗は破片として外側へ飛ばす（見た目を変えて速度アップ）
                pShot->kind = img_enemyShotMediumBall[8];
                pShot->muki = (GetRand(360) - 180) / 180.0 * DX_PI;
                pShot->speed = 3.5 + GetRand(100) / 100.0;
                pShot->param_i[0] = 2;  // はじけ済み
            }
            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_RoastChestnut_Grok()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 150.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.7 * (double)muki;
        if (count % 150 == 75) muki *= -1;

        // 画面端で反転
        if (enemy.x < 80.0)  muki = 1;
        if (enemy.x > 400.0) muki = -1;
    }

    // 一定間隔で焼き栗の波を発生
    if (count % 110 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotChestnutBurst;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = DX_PI / 2.0;  // 下向き基準
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