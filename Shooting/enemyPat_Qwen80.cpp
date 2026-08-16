#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕生成関数
// ============================================================

// フェーズ1: 北風 (速い弾、直線的、渦巻き)
static void ShotNorthWind(sEnemyShotSet* pEnemyShotSet)
{
    // 自機狙い角度の更新
    pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 1. 突風の矢 (自機狙い3way) - 青
    if (pEnemyShotSet->count % 12 == 0) {
        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * 0.12; // 広がり
            pEnemyShot->speed = 3.8; // 速め
            pEnemyShot->kind = img_enemyShotBullet[4]; // 色ID:4 (青)

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 2. 冷気の渦 (回転弾) - シアン
    if (pEnemyShotSet->count % 4 == 0) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        // 回転角度 (時間経過で回転)
        pEnemyShot->muki = pEnemyShotSet->count * 0.25;
        pEnemyShot->speed = 2.2;
        pEnemyShot->kind = img_enemyShotSmallBall[3]; // 色ID:3 (シアン)

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// フェーズ2: 太陽 (遅い弾、包み込むリング)
static void ShotSun(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 1. 陽光リング (全方位) - 橙
    // 予兆音 (発射20フレーム前)
    if (pEnemyShotSet->count % 120 == 100) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    // 発射
    if (pEnemyShotSet->count % 120 == 0 && pEnemyShotSet->count > 0) {
        int num = 24;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 2.0 * DX_PI * i / num;
            pEnemyShot->speed = 1.6; // ゆったり
            pEnemyShot->kind = img_enemyShotMediumOval[8]; // 色ID:8 (橙)

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 2. 陽だまりの粒 (ランダム方向) - 黄
    if (pEnemyShotSet->count % 20 == 0) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = GetRand(628) / 100.0; // ランダムな角度
        pEnemyShot->speed = 1.2 + GetRand(5) / 10.0;
        pEnemyShot->kind = img_enemyShotLargeBall[1]; // 色ID:1 (黄)

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// フェーズ3: 融合 (北風と太陽のミックス)
static void ShotFusion(sEnemyShotSet* pEnemyShotSet)
{
    pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 北風の矢 (自機狙い)
    if (pEnemyShotSet->count % 10 == 0) {
        for (int i = -2; i <= 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * 0.1;
            pEnemyShot->speed = 4.0;
            pEnemyShot->kind = img_enemyShotBullet[4]; // 青

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 太陽のリング (回転しながら拡散)
    if (pEnemyShotSet->count % 40 == 0) {
        int num = 16;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 2.0 * DX_PI * i / num + pEnemyShotSet->count * 0.05;
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotMediumOval[8]; // 橙

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動処理
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

void EnemyPat_NorthWindAndSun_Qwen()
{
    static int phase = 0;
    static sEnemyShotSet* pCurrentSet = nullptr;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200; // フェーズが長いためHP多め
        phase = 0;
        pCurrentSet = nullptr;
    }

    // フェーズ判定 (各フェーズ約10秒)
    int nextPhase = -1;
    if (count < 600)       nextPhase = 0; // 北風
    else if (count < 1200) nextPhase = 1; // 太陽
    else                   nextPhase = 2; // 融合

    // フェーズ切り替え処理
    if (nextPhase != phase) {
        // 古いセットをリストから外して削除
        if (pCurrentSet != nullptr) {
            pCurrentSet->prev->next = pCurrentSet->next;
            pCurrentSet->next->prev = pCurrentSet->prev;
            delete pCurrentSet;
            pCurrentSet = nullptr;
        }
        phase = nextPhase;
    }

    // ボスの移動制御
    if (phase == 0) {
        // 北風：左右に素早く往復
        enemy.x = 240.0 + 180.0 * sin(count * 0.03);
        enemy.y = 100.0 + 20.0 * sin(count * 0.05);
    }
    else if (phase == 1) {
        // 太陽：中央上部で円運動
        enemy.x = 240.0 + 120.0 * cos(count * 0.015);
        enemy.y = 120.0 + 60.0 * sin(count * 0.015);
    }
    else {
        // 融合：8の字運動
        enemy.x = 240.0 + 150.0 * sin(count * 0.02);
        enemy.y = 150.0 + 80.0 * sin(count * 0.04);
    }

    // 弾幕セットの生成・維持
    if (pCurrentSet == nullptr) {
        pCurrentSet = new sEnemyShotSet;
        pCurrentSet->count = 0;
        pCurrentSet->muki = 0.0;

        // フェーズに応じた関数をセット
        if (phase == 0)      pCurrentSet->patternFunc = ShotNorthWind;
        else if (phase == 1) pCurrentSet->patternFunc = ShotSun;
        else                 pCurrentSet->patternFunc = ShotFusion;

        // ダミーヘッドを持つリストへ追加
        pCurrentSet->pEnemyShotHead = new sEnemyShot;
        pCurrentSet->pEnemyShotHead->prev = pCurrentSet->pEnemyShotHead;
        pCurrentSet->pEnemyShotHead->next = pCurrentSet->pEnemyShotHead;

        // グローバルリストへ追加
        pCurrentSet->prev = enemyShotSetHead.prev;
        pCurrentSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pCurrentSet;
        enemyShotSetHead.prev = pCurrentSet;
    }

    // 弾の発生元をボスの現在位置に追従させる
    pCurrentSet->x = enemy.x;
    pCurrentSet->y = enemy.y;

    // 強制終了 (デバッグ用)
    if (count > 1800) {
        //enemy.hp = 0;
    }
}