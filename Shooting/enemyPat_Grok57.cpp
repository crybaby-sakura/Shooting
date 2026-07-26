// enemyPat_Tmp.cpp
// Rule30（セル・オートマトン）をモチーフにした弾幕パターン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// -------------------------------------------------
// ルール30弾幕パターン本体
// -------------------------------------------------
static void ShotRule30(sEnemyShotSet* pEnemyShotSet)
{
    // セル幅（奇数推奨）。画面幅480に合わせて間隔約7.5px
    const int W = 61;
    const int CENTER = 30;
    const double SPACING = 8.0;

    // 1セットだけ使う前提で静的に状態を保持
    static int cells[W];
    static int gen = 0;

    // ショットセット生成直後（count==0）で初期化
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < W; i++) cells[i] = 0;
        cells[CENTER] = 1;          // 中央1つだけON
        gen = 0;
    }

    // 敵の現在位置に合わせて発生源を追従
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 18.0;

    // 一定間隔で次世代を計算して弾を発射
    if (pEnemyShotSet->count % 14 == 0) {
        // 軽めの発射音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 現在の世代で「1」の位置から弾を生成
        for (int i = 0; i < W; i++) {
            if (cells[i] == 0) continue;

            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (i - CENTER) * SPACING;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI / 2.0;          // 真下
            pEnemyShot->speed = 1.7 + (gen % 4) * 0.12;

            // 小玉を使用。色は世代で変化させて視覚的に区別しやすく
            int col = gen % 9;  // 0:赤 ～ 8:橙
            pEnemyShot->kind = img_enemyShotSmallBall[col];

            // リンクリストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 次世代を計算（ルール30）
        // ルール番号30 = 00011110b → (30 >> pattern) & 1 で判定
        int next[W];
        for (int i = 0; i < W; i++) {
            int left = (i == 0) ? 0 : cells[i - 1];
            int self = cells[i];
            int right = (i == W - 1) ? 0 : cells[i + 1];
            int pattern = (left << 2) | (self << 1) | right;
            next[i] = (30 >> pattern) & 1;
        }
        for (int i = 0; i < W; i++) cells[i] = next[i];
        gen++;

        // あまり長く続けると密度が上がり過ぎるので一定世代でリセット
        if (gen >= 44) {
            for (int i = 0; i < W; i++) cells[i] = 0;
            cells[CENTER] = 1;
            gen = 0;
        }
    }

    // 既存弾の移動（メインルーチン側でcount++と画面外消去を行う）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// -------------------------------------------------
// 敵本体パターン
// -------------------------------------------------
void EnemyPat_Rule30_Grok()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;   // 200固定

        muki = 1;

        // ルール30弾幕用ショットセットを1つだけ生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRule30;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;
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
    else {
        // ゆっくり左右移動
        enemy.x += 0.55 * (double)muki;
        if (count % 200 == 100) muki *= -1;

        // 端に寄り過ぎないようクランプ
        if (enemy.x < 80.0) { enemy.x = 80.0;  muki = 1; }
        if (enemy.x > 400.0) { enemy.x = 400.0; muki = -1; }
    }
}