// enemyPat_337Beat.cpp
// 三三七拍子の弾幕（高難易度版）
// 位置更新ループ追加 & 弾密度・速度を大幅強化

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// リズムタイミング定数（60fps想定）
static const int BEAT_3_INTERVAL = 12;   // 0.2秒
static const int BEAT_7_INTERVAL = 9;    // 0.15秒
static const int PHASE_A1 = 0;
static const int PHASE_A2 = 12;
static const int PHASE_A3 = 24;
static const int PHASE_B1 = 60;
static const int PHASE_B2 = 72;
static const int PHASE_B3 = 84;
static const int PHASE_C_START = 120;
static const int CYCLE_LENGTH = 198;     // 3.3秒でループ

// ------------------------------------------------------------
// 弾幕パターン：三三七拍子（高難易度）
// ------------------------------------------------------------
static void ShotPattern337Beat(sEnemyShotSet* pEnemyShotSet)
{
    // 発射位置を敵の現在位置に更新
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;

    int phase = pEnemyShotSet->count % CYCLE_LENGTH;

    // ----- パートA：3拍（濃密な自機狙い扇弾） -----
    if (phase == PHASE_A1 || phase == PHASE_A2 || phase == PHASE_A3) {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);

        // 1拍あたり20発、±30°の扇型ランダム弾
        for (int i = 0; i < 20; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            double offsetDeg = (GetRand(60) - 30);          // -30°～+30°
            pShot->muki = baseAngle + offsetDeg * DX_PI / 180.0;
            pShot->speed = (200 + GetRand(200)) / 100.0;   // 2.0 ～ 4.0
            pShot->count = 0;
            pShot->kind = img_enemyShotBullet[0];           // 赤い銃弾

            // リスト接続
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // ----- パートB：3拍（三重の24方向リング、速度漸増） -----
    if (phase == PHASE_B1 || phase == PHASE_B2 || phase == PHASE_B3) {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 各拍で3種類のリング速度（低速・中速・高速）
        double speedLow, speedMid, speedHigh;
        if (phase == PHASE_B1) {
            speedLow = 1.5; speedMid = 3.0; speedHigh = 4.5;
        }
        else if (phase == PHASE_B2) {
            speedLow = 2.0; speedMid = 3.5; speedHigh = 5.0;
        }
        else {
            speedLow = 2.5; speedMid = 4.0; speedHigh = 5.5;
        }

        for (int ring = 0; ring < 3; ring++) {
            double spd;
            int kindBase;
            if (ring == 0) {
                spd = speedLow;
                kindBase = img_enemyShotDiamond[1];   // 黄色 菱形
            }
            else if (ring == 1) {
                spd = speedMid;
                kindBase = img_enemyShotDiamond[2];   // 緑色 菱形
            }
            else {
                spd = speedHigh;
                kindBase = img_enemyShotDiamond[3];   // シアン 菱形
            }

            // 24方向（15度刻み）でリングを構成
            for (int i = 0; i < 24; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = (i * 15.0) * DX_PI / 180.0;
                pShot->speed = spd;
                pShot->count = 0;
                pShot->kind = kindBase;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // ----- パートC：7拍（狙い撃ち密集弾＋ランダム円弾） -----
    if (phase >= PHASE_C_START && phase < PHASE_C_START + 7 * BEAT_7_INTERVAL) {
        int beatIndex = (phase - PHASE_C_START) / BEAT_7_INTERVAL;
        if ((phase - PHASE_C_START) % BEAT_7_INTERVAL == 0 && beatIndex < 7) {
            // 効果音：6拍までは heavy、最終拍だけ extreme
            if (beatIndex < 6) {
                if (CheckSoundMem(sound_enemyShot_heavy))
                    StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            }
            else {
                if (CheckSoundMem(sound_enemyShot_extreme))
                    StopSoundMem(sound_enemyShot_extreme);
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
            }

            double baseAngle = atan2(player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x);

            // (1) 自機を狙った5発の密集弾（角度±8°）
            static const double aimOffsets[5] = { -8.0, -4.0, 0.0, 4.0, 8.0 };
            for (int i = 0; i < 5; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = baseAngle + aimOffsets[i] * DX_PI / 180.0;
                pShot->speed = 3.5 + (GetRand(150) / 100.0);   // 3.5 ～ 5.0
                pShot->count = 0;
                // 最終拍の中央弾だけ大玉マゼンタ、他は青い銃弾
                pShot->kind = (beatIndex == 6 && i == 2)
                    ? img_enemyShotLargeBall[5]
                    : img_enemyShotBullet[4];

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }

            // (2) 撹乱用のランダム8方向円弾（小玉、ランダム色）
            for (int i = 0; i < 8; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                // 45度刻み＋最大±10度のジッター
                pShot->muki = (i * 45.0 + GetRand(20) - 10) * DX_PI / 180.0;
                pShot->speed = 2.0 + (GetRand(100) / 100.0); // 2.0 ～ 3.0
                pShot->count = 0;
                pShot->kind = img_enemyShotSmallBall[GetRand(7)]; // 0～7のランダム色

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // ========================================================
    // ★ 全ての弾の位置を毎フレーム更新（追加）
    // ========================================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_337Beat_DeepSeek()
{
    static int moveDir;

    if (count == 1) {
        // ボス初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;

        // 三三七拍子の弾幕セットを1つだけ生成（永続稼働）
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPattern337Beat;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;   // ダミーヘッド
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルリストに登録
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ボスの動き：左右往復＋上下ゆらぎ（やや速め）
    enemy.x += 1.5 * moveDir;
    if (enemy.x > 430.0) moveDir = -1;
    if (enemy.x < 50.0)  moveDir = 1;
    enemy.y = 40.0 + 15.0 * sin(count * 0.06);
}