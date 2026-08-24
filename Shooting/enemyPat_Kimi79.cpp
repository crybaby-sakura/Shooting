// enemyPat_lissajous.cpp
// リサジュー曲線弾幕「追跡する位相花」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 自機狙い子弾
// ------------------------------------------------------------
static void ShotAimPlayer(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;

    if (pSet->count == 0) {
        pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = pSet->muki;
        pShot->speed = pSet->param_d[0];

        int color = pSet->kind;
        pShot->kind = img_enemyShotSmallBall[color];

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
// リサジュー親弾（軌道上を周回し、自機方向へ子弾を発射）
// ------------------------------------------------------------
static void ShotLissajousParent(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;

    // ---- 親弾生成（count == 0 のみ、8個） ----
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int shotNum = 8;
        int parentColor;
        switch (pSet->kind % 3) {
        case 0: parentColor = 4; break; // 青
        case 1: parentColor = 5; break; // マゼンタ
        case 2: parentColor = 0; break; // 赤
        }

        for (int i = 0; i < shotNum; i++) {
            pShot = new sEnemyShot;

            pShot->x = pSet->x;
            pShot->y = pSet->y;

            // リサジュー参数
            pShot->param_i[0] = pSet->param_i[0]; // a
            pShot->param_i[1] = pSet->param_i[1]; // b
            pShot->param_i[2] = pSet->param_i[2]; // 発射間隔
            pShot->param_i[3] = pSet->param_i[3]; // 子弾色

            pShot->param_d[2] = pSet->param_d[2]; // A
            pShot->param_d[3] = pSet->param_d[3]; // B
            pShot->param_d[4] = pSet->param_d[4]; // omega
            pShot->param_d[5] = 2.0 * DX_PI * i / shotNum; // delta（初期位相）
            pShot->param_d[6] = pSet->param_d[6]; // alpha
            pShot->param_d[7] = pSet->param_d[7]; // phi0

            pShot->kind = img_enemyShotLargeBall[parentColor];

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // ---- 親弾移動＆子弾発射 ----
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        // 中心を敵本体に追従
        double cx = enemy.x;
        double cy = enemy.y + 40.0;
        double t = p->count;
        double omega = p->param_d[4];
        int a = p->param_i[0];
        int b = p->param_i[1];
        double delta = p->param_d[5];
        double phi = p->param_d[7] + p->param_d[6] * pSet->count;

        p->x = cx + p->param_d[2] * sin(a * omega * t + delta + phi);
        p->y = cy + p->param_d[3] * sin(b * omega * t + delta);

        // 子弾発射（自機狙い）
        if (p->count > 0 && p->count % p->param_i[2] == 0) {
            sEnemyShotSet* pChild = new sEnemyShotSet;
            pChild->count = 0;
            pChild->patternFunc = ShotAimPlayer;
            pChild->x = p->x;
            pChild->y = p->y;
            pChild->muki = atan2(player.y - p->y, player.x - p->x);
            pChild->kind = p->param_i[3];
            pChild->param_d[0] = pSet->param_i[4] / 10.0; // 子弾速度

            pChild->pEnemyShotHead = new sEnemyShot;
            pChild->pEnemyShotHead->prev = pChild->pEnemyShotHead;
            pChild->pEnemyShotHead->next = pChild->pEnemyShotHead;

            pChild->prev = enemyShotSetHead.prev;
            pChild->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pChild;
            enemyShotSetHead.prev = pChild;
        }

        p = p->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Lissajous_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // ========================================================
    // Phase 1：楕円追跡（1:1、位相固定、子弾速度2.5）
    // ========================================================
    if (count >= 1 && count <= 600 && count % 120 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotLissajousParent;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 40.0;
        pSet->kind = 0;

        pSet->param_i[0] = 1;   // a
        pSet->param_i[1] = 1;   // b
        pSet->param_i[2] = 30;  // 発射間隔
        pSet->param_i[3] = 4;   // 子弾色：青
        pSet->param_i[4] = 25;  // 子弾速度×10

        pSet->param_d[2] = 90.0;  // A
        pSet->param_d[3] = 60.0;  // B
        pSet->param_d[4] = 0.025; // omega
        pSet->param_d[6] = 0.0;   // alpha
        pSet->param_d[7] = 0.0;   // phi0

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ========================================================
    // Phase 2：8の字追跡（2:1、位相回転、子弾速度3.0）
    // ========================================================
    if (count >= 601 && count <= 1200 && count % 120 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotLissajousParent;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 40.0;
        pSet->kind = 1;

        pSet->param_i[0] = 2;   // a
        pSet->param_i[1] = 1;   // b
        pSet->param_i[2] = 25;  // 発射間隔
        pSet->param_i[3] = 5;   // 子弾色：マゼンタ
        pSet->param_i[4] = 30;  // 子弾速度×10

        pSet->param_d[2] = 100.0; // A
        pSet->param_d[3] = 70.0;  // B
        pSet->param_d[4] = 0.022; // omega
        pSet->param_d[6] = 0.01;  // alpha（回転開始）
        pSet->param_d[7] = 0.0;   // phi0

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ========================================================
    // Phase 3：花弁追跡（3:2、高速回転、子弾速度3.5）
    // ========================================================
    if (count >= 1201 && count % 120 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotLissajousParent;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 40.0;
        pSet->kind = 2;

        pSet->param_i[0] = 3;   // a
        pSet->param_i[1] = 2;   // b
        pSet->param_i[2] = 20;  // 発射間隔
        pSet->param_i[3] = 0;   // 子弾色：赤
        pSet->param_i[4] = 35;  // 子弾速度×10

        pSet->param_d[2] = 110.0; // A
        pSet->param_d[3] = 80.0;  // B
        pSet->param_d[4] = 0.020; // omega
        pSet->param_d[6] = 0.025; // alpha（高速回転）
        pSet->param_d[7] = 0.0;   // phi0

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}