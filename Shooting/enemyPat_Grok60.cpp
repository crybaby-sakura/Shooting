// enemyPat_Tmp.cpp
// 弾幕パターン：「傷の残響」
// ボスがダメージを受けるたびに高速の反撃弾を放ち、
// 一定ダメージごとに円形弾幕を追加で発生させる。
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 通常反撃弾：ヒット時にプレイヤー方向へ高速の細い弾を4発
// 使用素材：img_enemyShotBullet（銃弾・細い形状）、赤色[0]
// 効果音：sound_enemyShot_medium
// ------------------------------------------------------------
static void ShotRetaliate(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (enemy.hp % 2 == 1) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 発生時点のプレイヤー位置を狙う（わずかに散らす）
            double dx = player.x - pEnemyShot->x;
            double dy = player.y - pEnemyShot->y;
            pEnemyShot->muki = atan2(dy, dx);
            pEnemyShot->muki += (GetRand(30) - 15) / 180.0 * DX_PI; // ±15度程度

            pEnemyShot->speed = 5.0 + GetRand(10) / 10.0; // 5.0〜6.0
            pEnemyShot->kind = img_enemyShotBullet[0]; // 赤の銃弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動（count のインクリメントと画面外消去はメイン側）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 蓄積反撃弾：一定ダメージごとに全周へ中玉を放射
// 使用素材：img_enemyShotMediumBall（中玉）、シアン[3]
// 効果音：sound_enemyShot_heavy
// ------------------------------------------------------------
static void ShotCircleBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int n = 18;
        for (int i = 0; i < n; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * (2.0 * DX_PI / n);
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアンの中玉

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
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Counter_Grok()
{
    static int muki;
    static int prev_hp;
    static int burst_threshold; // 次の円形弾幕を発生させるHP閾値

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        prev_hp = 200;
        burst_threshold = 190; // 10HPごと（5%）
    }
    else {
        // 左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;

        // ダメージ検知（メイン側でhpが減った場合）
        if (enemy.hp < prev_hp) {
            // ---- 通常反撃（毎ダメージフレーム） ----
            {
                sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                pEnemyShotSet->count = 0;
                pEnemyShotSet->patternFunc = ShotRetaliate;
                pEnemyShotSet->x = enemy.x;
                pEnemyShotSet->y = enemy.y + 10.0;
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

            // ---- 蓄積反撃（10HPごと） ----
            if (enemy.hp <= burst_threshold) {
                sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                pEnemyShotSet->count = 0;
                pEnemyShotSet->patternFunc = ShotCircleBurst;
                pEnemyShotSet->x = enemy.x;
                pEnemyShotSet->y = enemy.y + 10.0;
                // 開始角度をランダムにして毎回少し変化させる
                pEnemyShotSet->muki = GetRand(359) / 180.0 * DX_PI;
                pEnemyShotSet->kind = 0;
                pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->prev = enemyShotSetHead.prev;
                pEnemyShotSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pEnemyShotSet;
                enemyShotSetHead.prev = pEnemyShotSet;

                // 次の閾値を現在のHPより下の10の倍数に設定
                burst_threshold = ((enemy.hp - 1) / 10) * 10;
            }

            prev_hp = enemy.hp;
        }
    }
}