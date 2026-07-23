// enemyPat_BeerShower.cpp
// ビールかけモチーフ弾幕「無限ビールかけ祭り」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 使える効果音: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge

// ------------------- 補助弾幕関数 -------------------

// ビール流（太めの直線/斜め流）
static void ShotBeerStream(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int numStreams = 5 + (pEnemyShotSet->kind % 3); // 5〜7本
        for (int i = 0; i < numStreams; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double baseAngle = pEnemyShotSet->muki;
            double offset = (i - numStreams / 2.0) * 0.15;
            pEnemyShot->muki = baseAngle + offset;

            pEnemyShot->speed = 2.8 + (GetRand(40) / 100.0);

            // ビール色: 黄(1) or 橙(8)
            int color = (GetRand(1) == 0) ? 1 : 8;
            pEnemyShot->kind = img_enemyShotMediumBall[color];  // 中玉でビール感

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 軽く波打たせる（ビール流らしい揺らぎ）
        if (pEnemyShotSet->count % 8 == 0) {
            pShot->muki += 0.02 * sin(pEnemyShotSet->count / 5.0);
        }

        pShot = pShot->next;
    }
}

// 白泡弾（ゆっくり降下＋左右揺れ）
static void ShotFoam(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 18; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(400) - 200;
            pEnemyShot->y = pEnemyShotSet->y - 20 + GetRand(40);

            pEnemyShot->muki = DX_PI / 2 + (GetRand(80) - 40) / 180.0 * DX_PI; // 下向き中心
            pEnemyShot->speed = 1.2 + GetRand(80) / 100.0;

            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白(6)

            // param_d[0] で揺れ振幅
            pEnemyShot->param_d[0] = 0.8 + GetRand(60) / 100.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki) * 0.6; // 横成分少なめ
        pShot->y += pShot->speed * sin(pShot->muki);

        // 左右に揺れる泡
        pShot->x += sin(pEnemyShotSet->count / 6.0 + pShot->param_d[0]) * pShot->param_d[0];

        pShot = pShot->next;
    }
}

// 着弾スプラッシュ＆ジョッキ弾
static void ShotSplash(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 扇状スプレー
        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double angle = DX_PI * (0.3 + i * 0.4 / 12.0); // 下向き扇
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.0 + GetRand(100) / 100.0;

            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白泡

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ビールジョッキ風追尾弾（中楕円 or 大玉）
        for (int i = 0; i < 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (i * 2 - 1) * 30;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->speed = 2.5;

            pEnemyShot->kind = img_enemyShotMediumOval[1]; // 黄中楕円でジョッキ感
            pEnemyShot->param_i[0] = 60; // 追尾持続フレーム

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] > 0) {
            // 弱いホーミング
            double targetAngle = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->muki = pShot->muki * 0.92 + targetAngle * 0.08;
            pShot->param_i[0]--;
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------- 敵本体 -------------------
void EnemyPat_BeerSpray_Grok()
{
    static int muki = 1;
    static int phase = 0;
    static int phaseTimer = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
        phaseTimer = 0;
    }
    else {
        // 左右移動
        enemy.x += 1.1 * (double)muki;
        if (enemy.x < 80 || enemy.x > 400) muki *= -1;

        // 軽い上下揺れ
        enemy.y = 60.0 + sin(count / 30.0) * 15.0;
    }

    phaseTimer++;

    // フェーズ管理
    if (phase == 0) { // ビール流降らし
        if (phaseTimer % 15 == 1) {
            sEnemyShotSet* p = new sEnemyShotSet;
            p->count = 0;
            p->patternFunc = ShotBeerStream;
            p->x = enemy.x;
            p->y = enemy.y + 20;
            p->muki = atan2(player.y - p->y, player.x - p->x) + (GetRand(40) - 20) / 180.0 * DX_PI;
            p->kind = phaseTimer / 45;
            p->pEnemyShotHead = new sEnemyShot;
            p->pEnemyShotHead->prev = p->pEnemyShotHead;
            p->pEnemyShotHead->next = p->pEnemyShotHead;

            p->prev = enemyShotSetHead.prev;
            p->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = p;
            enemyShotSetHead.prev = p;
        }
        if (phaseTimer > 180) {
            phase = 1;
            phaseTimer = 0;
        }
    }
    else if (phase == 1) { // 泡乱舞
        if (phaseTimer % 35 == 1) {
            sEnemyShotSet* p = new sEnemyShotSet;
            p->count = 0;
            p->patternFunc = ShotFoam;
            p->x = enemy.x;
            p->y = enemy.y - 10;
            p->kind = 0;
            p->pEnemyShotHead = new sEnemyShot;
            p->pEnemyShotHead->prev = p->pEnemyShotHead;
            p->pEnemyShotHead->next = p->pEnemyShotHead;

            p->prev = enemyShotSetHead.prev;
            p->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = p;
            enemyShotSetHead.prev = p;
        }
        if (phaseTimer > 160) {
            phase = 2;
            phaseTimer = 0;
        }
    }
    else if (phase == 2) { // スプラッシュ
        if (phaseTimer % 5 == 0) {
            sEnemyShotSet* p = new sEnemyShotSet;
            p->count = 0;
            p->patternFunc = ShotSplash;
            p->x = enemy.x;
            p->y = enemy.y + 30;
            p->kind = 0;
            p->pEnemyShotHead = new sEnemyShot;
            p->pEnemyShotHead->prev = p->pEnemyShotHead;
            p->pEnemyShotHead->next = p->pEnemyShotHead;

            p->prev = enemyShotSetHead.prev;
            p->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = p;
            enemyShotSetHead.prev = p;
        }
        if (phaseTimer > 140) {
            phase = 0; // ループ
            phaseTimer = 0;
        }
    }
}