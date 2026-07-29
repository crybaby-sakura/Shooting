// enemyPat_Tmp.cpp
// 三三七拍子弾幕「祝い撃ち」（高難易度版）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：三三七拍子（祝い撃ち）高難易度
// count の増加と画面外消去はメインルーチン側で行われる
static void ShotSanSanNana(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射タイミング（フレーム）を少し詰めて密度を上げる
    // 3発（間隔8f） → 休符22f → 3発（間隔8f） → 休符28f → 7発（間隔7f）
    static const int fireFrames[] = {
        0, 8, 16,           // 第1の3拍
        38, 46, 54,         // 第2の3拍
        82, 89, 96, 103, 110, 117, 124  // 7拍
    };
    static const int fireCount = sizeof(fireFrames) / sizeof(fireFrames[0]);

    int fireIndex = -1;
    for (int i = 0; i < fireCount; i++) {
        if (pEnemyShotSet->count == fireFrames[i]) {
            fireIndex = i;
            break;
        }
    }

    if (fireIndex >= 0) {
        // 効果音（リズムを保つため light を継続使用）
        if (CheckSoundMem(sound_enemyShot_light)) {
            StopSoundMem(sound_enemyShot_light);
        }
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 基準角度（プレイヤー狙い）
        double baseMuki = atan2(player.y - (enemy.y + 12.0), player.x - enemy.x);

        // グループごとに弾数と扇の幅を変えて難易度を上げる
        int numBullets = 1;
        double fanHalfDeg = 0.0;   // 扇の片側最大角度
        int color = 0;
        int useMedium = 1;         // 1:中玉  0:小玉

        if (fireIndex < 3) {
            // 第1の3拍：各拍で3発の狭い扇
            numBullets = 3*2+1;
            fanHalfDeg = 14.0;
            color = 0;             // 赤
            useMedium = 1;
        }
        else if (fireIndex < 6) {
            // 第2の3拍：各拍で5発の中扇 + 少し速度差
            numBullets = 5*2+1;
            fanHalfDeg = 28.0;
            color = 1;             // 黄
            useMedium = 1;
        }
        else {
            // 7拍：各拍で7発の広い扇（かなり密集）
            numBullets = 7*2+1;
            fanHalfDeg = 42.0;
            color = 8;             // 橙
            useMedium = 0;         // 小玉で密度を稼ぐ
        }

        for (int b = 0; b < numBullets; b++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = enemy.x;
            pEnemyShot->y = enemy.y + 12.0;

            // 扇状に均等配置
            double t = (numBullets == 1) ? 0.0 : (double)b / (numBullets - 1);  // 0〜1
            double angleOffsetDeg = -fanHalfDeg + t * (fanHalfDeg * 2.0);

            // 第2の3拍以降は少しランダム性を加えて読みにくくする
            if (fireIndex >= 3) {
                angleOffsetDeg += (GetRand(6) - 3);  // ±3度程度
            }

            pEnemyShot->muki = baseMuki + angleOffsetDeg / 180.0 * DX_PI;

            // 速度を全体的に上げ、拍ごとに少し変化
            if (fireIndex < 3) {
                pEnemyShot->speed = 3.4 + GetRand(3) / 10.0;          // 3.4〜3.6
            }
            else if (fireIndex < 6) {
                pEnemyShot->speed = 3.6 + GetRand(5) / 10.0;          // 3.6〜4.0
            }
            else {
                // 7拍は外側ほど少し速くして逃げ道を狭める
                double speedBonus = fabs(angleOffsetDeg) / 40.0;
                pEnemyShot->speed = 3.8 + speedBonus + GetRand(4) / 10.0;
            }

            // 弾種
            if (useMedium) {
                pEnemyShot->kind = img_enemyShotMediumBall[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[color];
            }

            // リスト連結
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_337Beat_Grok()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 移動速度を上げて位置取りを難しく
        enemy.x += 1.15 * (double)muki;
        if (enemy.x < 60.0) {
            enemy.x = 60.0;
            muki = 1;
        }
        else if (enemy.x > 420.0) {
            enemy.x = 420.0;
            muki = -1;
        }
        // 反転タイミングを早めに
        if (count % 120 == 60) {
            muki *= -1;
        }
    }

    // セット開始間隔を短縮（約3秒ごと）して重なりやすくする
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSanSanNana;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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