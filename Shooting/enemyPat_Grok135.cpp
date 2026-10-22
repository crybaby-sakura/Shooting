#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// バックドラフト弾幕パターン
// 既存素材の組み合わせで「酸素欠乏 → 急激な噴出」を表現
// ============================================================
// 使用素材:
//   ・煙（酸素欠乏）: 中楕円弾 白 / 鱗弾 白・黒（低速・広がり）
//   ・火花（高速）  : 小玉 黄 / 鱗弾 黄
//   ・火の玉        : 中玉 橙
//   ・炎（外側）    : 中玉 赤 / 大玉 赤（低速）
// 効果音: sound_enemyCharge（溜め）, sound_enemyShot_heavy / extreme（爆発）
// ============================================================

static void ShotBackdraft(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int c = pEnemyShotSet->count;

    // -------------------------------------------------------
    // Phase 0: 酸素欠乏開始（溜め・煙の発生）
    // -------------------------------------------------------
    if (c == 0) {
        // 予告音（チャージ）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 初期煙を放射状に低速で放出（白の中楕円弾 + 白・黒の鱗弾）
        for (int i = 0; i < 10; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (2.0 * DX_PI * i / 10.0) + (GetRand(30) - 15) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.45 + GetRand(35) / 100.0; // 非常に遅い
            // 白の中楕円弾をメインの煙として使用
            pEnemyShot->kind = img_enemyShotMediumOval[6];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        // 追加の薄い煙（鱗弾）
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(20) - 10);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(20) - 10);
            pEnemyShot->muki = (2.0 * DX_PI * i / 8.0) + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.3 + GetRand(40) / 100.0;
            pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotScale[6] : img_enemyShotScale[7]; // 白 / 黒
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // -------------------------------------------------------
    // Phase 1: 煙の追加発生（徐々に空気が薄くなる演出）
    // -------------------------------------------------------
    if (c > 0 && c < 55 && c % 7 == 0) {
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(16) - 8);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(16) - 8);
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.35 + GetRand(30) / 100.0;
            pEnemyShot->kind = img_enemyShotScale[6 + (GetRand(1))]; // 白 or 黒
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // -------------------------------------------------------
    // Phase 2: バックドラフト爆発（酸素流入 → 一気に噴出）
    // -------------------------------------------------------
    if (c == 70) {
        // 爆発音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // --- 内側：高速黄色い火花（小玉 + 鱗弾） ---
        for (int i = 0; i < 20*5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (2.0 * DX_PI * i / 20.0/5) + (GetRand(25) - 12) / 180.0 * DX_PI;
            pEnemyShot->speed = 4.2 + GetRand(180) / 100.0; // 高速
            pEnemyShot->kind = (i % 3 == 0) ? img_enemyShotScale[1] : img_enemyShotSmallBall[1]; // 黄
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        // 追加のランダム火花
        for (int i = 0; i < 12*5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(10) - 5);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(10) - 5);
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.8 + GetRand(220) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[1];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // --- 中間層：オレンジの火の玉（中玉） ---
        for (int i = 0; i < 12*5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (2.0 * DX_PI * i / 12.0/5) + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.3 + GetRand(120) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // --- 外側：低速赤い炎（中玉 + 一部大玉）で逃げ道を塞ぐ ---
        for (int i = 0; i < 14*5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (2.0 * DX_PI * i / 14.0/5) + (GetRand(18) - 9) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.1 + GetRand(90) / 100.0; // 低速
            pEnemyShot->kind = (i % 5 == 0) ? img_enemyShotLargeBall[0] : img_enemyShotMediumBall[0]; // 赤
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // -------------------------------------------------------
    // Phase 3: 残火（爆発後の残り火と追加煙）
    // -------------------------------------------------------
    if (c == 95) {
        // 残りの炎と煙を少し追加
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(30) - 15);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(30) - 15);
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.7 + GetRand(50) / 100.0;
            pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotMediumBall[0] : img_enemyShotMediumOval[6];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // -------------------------------------------------------
    // 全弾の移動（毎フレーム）
    // count のインクリメントや画面外消去はメインルーチン側で行う
    // -------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_Backdraft_Grok()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        // 緩やかに左右移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 120.0 || enemy.x > 360.0) {
            muki *= -1;
        }
        // 上下もわずかに揺らす
        enemy.y = 70.0 + 8.0 * sin(count / 40.0);
    }

    // 一定間隔でバックドラフトを発動
    // （約3秒周期：溜め約1.2秒 + 爆発 + 余韻）
    if (count % 180 == 40) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBackdraft;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
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
}