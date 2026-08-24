// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：北風と太陽
// ============================================================
// 前半：青い銃弾による高速な左右への風（隙間を見つけて「やり過ごす」）
// 後半：橙の大玉による誘導弾と、赤の中玉による画面外からの closing レーザー状の熱波
//       （自ら動いて道を「切り開く」）
// ============================================================

// 弾幕セットをリストに追加するヘルパー関数
static void AddShotSet(sEnemyShotSet::PatternFunc func) {
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ------------------------------------------------------------
// 前半：北風の猛吹（高速な直線弾）
// ------------------------------------------------------------
static void ShotNorthWind(sEnemyShotSet* pEnemyShotSet) {
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int num = 12 * 3; // 片側の発射数
        for (int i = 0; i < num; i++) {
            // 右向きの風（-60度 ～ +60度 の扇状）
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = -DX_PI / 3.0 + (DX_PI * 2.0 / 3.0) / (num - 1) * i;
            // GetRand(30) は 0～30 の 31種類。10.0で割ることで 0.0 ～ 3.0 の速度変動
            pEnemyShot->speed = 9.0 + GetRand(30) / 10.0;
            pEnemyShot->kind = img_enemyShotBullet[4]; // 青い銃弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 左向きの風（反転）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI - (-DX_PI / 3.0 + (DX_PI * 2.0 / 3.0) / (num - 1) * i);
            pEnemyShot->speed = 9.0 + GetRand(30) / 10.0;
            pEnemyShot->kind = img_enemyShotBullet[4]; // 青い銃弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 後半：太陽の穏やかな熱（ゆっくり誘導してくる大玉）
// ------------------------------------------------------------
static void ShotSun(sEnemyShotSet* pEnemyShotSet) {
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
        pEnemyShot->speed = 2.0;
        pEnemyShot->kind = img_enemyShotLargeBall[8]; // 橙の大玉
        pEnemyShot->param_d[0] = 0.03; // 誘導の強さ
        
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // プレイヤーに向かって誘導する処理
        double targetMuki = atan2(player.y - pShot->y, player.x - pShot->x);
        double diff = targetMuki - pShot->muki;
        // 角度差を -PI ～ +PI の範囲に正規化
        while (diff > DX_PI) diff -= 2.0 * DX_PI;
        while (diff < -DX_PI) diff += 2.0 * DX_PI;
        pShot->muki += diff * pShot->param_d[0];

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 後半：熱波（画面外から中心に向かう遅いリング状の弾）
// ------------------------------------------------------------
static void ShotHeatWave(sEnemyShotSet* pEnemyShotSet) {
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int num = 30; // リングの弾数
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double angle = (2.0 * DX_PI / num) * i;

            // 画面外（半径400の円周上）から発射
            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * 400.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * 400.0;

            // 敵（太陽）の中心に向かう角度
            pEnemyShot->muki = atan2(pEnemyShotSet->y - pEnemyShot->y, pEnemyShotSet->x - pEnemyShot->x);
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤の中玉
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_NorthWindAndSun_Zai()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 移動処理
    if (count < 580) {
        // 前半は左右に大きく揺れながら風を送る
        enemy.x = 240.0 + sin(count * 0.035) * 150.0;
        enemy.y = 240.0 + sin(count * 0.045) * 100.0;
    }
    else {
        // 後半は画面中央上部にゆっくり定位置につく（太陽のイメージ）
        enemy.x += (240.0 - enemy.x) * 0.03;
        enemy.y += (80.0 - enemy.y) * 0.03;
    }

    // 予告音（太陽への移行直前）
    if (count == 580) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 弾幕の生成
    if (count >= 60 && count < 560) {
        // 北風：60フレーム間隔で発射
        if (count % 60 == 0) {
            AddShotSet(ShotNorthWind);
        }
    }

    if (count >= 620) {
        // 太陽（誘導弾）：120フレーム間隔で発射
        if (count % 120 == 0) {
            AddShotSet(ShotSun);
        }
        // 熱波（リング弾）：70フレーム間隔で発射
        if (count % 70 == 0) {
            AddShotSet(ShotHeatWave);
        }
    }
}