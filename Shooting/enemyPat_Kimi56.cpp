// enemyPat_ebbinghaus.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// エビングハウス錯視弾幕：「相対視の欺罔」
// ------------------------------------------------------------
// 【仕様】
// ・核弾（実弾）1個を中心に、伴弾（装飾弾）8個が周回する
// ・大伴弾パターン：核弾(中玉)の周りを大玉が遠くを周回 → 核弾が小さく見える
// ・小伴弾パターン：核弾(中玉)の周りを小玉が近くを周回 → 核弾が大きく見える
// ・伴弾は param_i[0]==1 でマーク。メインルーチンの当たり判定で無視推奨
// ------------------------------------------------------------
static void ShotEbbinghaus(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 0:大伴弾（核弾が小さく見える錯視） / 1:小伴弾（核弾が大きく見える錯視）
        int illusionType = pEnemyShotSet->kind % 2;

        // --- 核弾（実弾）を1つ生成 ---
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = (180 + GetRand(80)) / 100.0;   // 1.8 ～ 2.6
        pEnemyShot->kind = img_enemyShotMediumBall[6];      // 白の中玉
        pEnemyShot->param_i[0] = 0;                         // 0=核弾, 1=伴弾
        pEnemyShot->param_i[1] = illusionType;
        pEnemyShot->margin = 100;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // --- 伴弾（装飾弾）を8つ生成 ---
        int companionCount = 8;
        double radius = (illusionType == 0) ? 46.0 : 16.0;  // 大伴弾は遠く、小伴弾は近く
        double angularSpeed = (GetRand(1) == 0 ? 1.0 : -1.0)
            * (0.03 + GetRand(30) / 1000.0); // ±0.03 ～ ±0.06

        for (int i = 0; i < companionCount; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = (DX_PI * 2.0 / companionCount) * i;

            pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);
            pEnemyShot->muki = pEnemyShotSet->muki;
            pEnemyShot->speed = 0.0;        // 伴弾は核弾に追従（speedは不使用）
            pEnemyShot->param_i[0] = 1;     // 伴弾フラグ
            pEnemyShot->param_i[1] = i;     // 伴弾インデックス
            pEnemyShot->param_d[0] = angle; // 現在の周回角度
            pEnemyShot->param_d[1] = radius;// 周回半径
            pEnemyShot->param_d[2] = angularSpeed; // 角速度
            pEnemyShot->margin = 100;

            if (illusionType == 0) {
                // 大伴弾：大玉、赤(0)と橙(8)を交互
                pEnemyShot->kind = img_enemyShotLargeBall[(i % 2 == 0) ? 0 : 8];
            }
            else {
                // 小伴弾：小玉、橙(8)と赤(0)を交互
                pEnemyShot->kind = img_enemyShotSmallBall[(i % 2 == 0) ? 8 : 0];
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 核弾の移動 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    double coreX = 0.0, coreY = 0.0;
    bool coreFound = false;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            coreX = pShot->x;
            coreY = pShot->y;
            coreFound = true;
        }
        pShot = pShot->next;
    }

    if (!coreFound) return; // 核弾が消滅していれば伴弾更新不要

    // --- 伴弾の位置更新（核弾周囲を周回） ---
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            pShot->param_d[0] += pShot->param_d[2];
            pShot->x = coreX + pShot->param_d[1] * cos(pShot->param_d[0]);
            pShot->y = coreY + pShot->param_d[1] * sin(pShot->param_d[0]);
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Ebbinghaus_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 45フレーム毎にエビングハウス錯視弾を生成
    if (count % 15 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotEbbinghaus;
        pEnemyShotSet->x = enemy.x + GetRand(400) - 200;
        pEnemyShotSet->y = 0.0;
        pEnemyShotSet->muki = DX_PI / 2 + (GetRand(60) - 30) * 0.01;
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