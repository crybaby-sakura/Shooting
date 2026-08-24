// enemyPat_tmp.cpp
// 北風と太陽の賭け - 3フェーズ弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  Phase 1: 北風
// ============================================================

// 風の弾：画面左端から右へ流れる青白い弾
static void ShotWind(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(60) - 30;
            pEnemyShot->muki = 0.0; // 右方向
            pEnemyShot->speed = 3.0 + GetRand(3) / 2.0; // 3.0〜4.0

            // 小玉：青(4) or シアン(3)
            pEnemyShot->kind = img_enemyShotSmallBall[GetRand(1) == 0 ? 4 : 3];

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

// 冷気の針：敵からプレイヤー方向を中心に扇状に広がる
static void ShotColdNeedle(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int num = 11;
        double baseAngle = pEnemyShotSet->muki;

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // baseAngle ± 50度の扇状
            pEnemyShot->muki = baseAngle + (-50.0 + i * 100.0 / (num - 1)) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.5 + GetRand(2) / 2.0; // 2.5〜3.5

            // 銃弾：白(6) or 青(4)
            pEnemyShot->kind = img_enemyShotBullet[GetRand(1) == 0 ? 6 : 4];

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

// 凍結の息：扇状に広がる大きく遅い弾
static void ShotFreezeBreath(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int num = 7;
        double baseAngle = pEnemyShotSet->muki;

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + (-40.0 + i * 80.0 / (num - 1)) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.0 + GetRand(2) / 2.0; // 1.0〜2.0（遅め）

            // 中楕円弾：シアン(3) or 白(6)
            pEnemyShot->kind = img_enemyShotMediumOval[GetRand(1) == 0 ? 3 : 6];

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
//  Phase 2: 太陽
// ============================================================

// 陽光の波：同心円状に3重に広がる全方位弾
static void ShotSunWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int num = 12;
        for (int wave = 0; wave < 3; wave++) {
            for (int i = 0; i < num; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = (360.0 / num * i) / 180.0 * DX_PI;
                pEnemyShot->speed = 1.2 + wave * 0.7; // 1.2, 1.9, 2.6

                // 小玉：橙(8)
                pEnemyShot->kind = img_enemyShotSmallBall[8];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 熱波の誘導弾：プレイヤーにゆっくり追従する大きな弾
static void ShotSunHoming(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int num = 2 + GetRand(1); // 2〜3発

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->speed = 1.0 + GetRand(1); // 1.0〜2.0（遅め）

            // 大玉：橙(8) or 赤(0)
            pEnemyShot->kind = img_enemyShotLargeBall[GetRand(1) == 0 ? 8 : 0];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 30フレームごとにプレイヤー方向へ微調整（180フレームまで）
        if (pShot->count % 30 == 0 && pShot->count < 180) {
            double targetAngle = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = targetAngle - pShot->muki;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            pShot->muki += diff * 0.12;
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  Phase 3: 決戦
// ============================================================

// 渦巻き弾：回転しながら拡散する弾（時計回りと反時計回りを混在）
static void ShotVortex(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int num = 20;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (360.0 / num * i) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.6 + GetRand(2) / 2.0; // 1.6〜2.6

            // 回転方向と速度を param_d[0] に保存（隣り合う弾は逆回転）
            pEnemyShot->param_d[0] = ((i % 2 == 0) ? 1.0 : -1.0) * (1.5 + GetRand(1)) / 180.0 * DX_PI;

            // 中玉：黄(1) or 橙(8)
            pEnemyShot->kind = img_enemyShotMediumBall[GetRand(1) == 0 ? 1 : 8];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->muki += pShot->param_d[0];
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================

void EnemyPat_NorthWindAndSun_Kimi()
{
    const int PHASE1_END = 450;   // 北風
    const int PHASE2_END = 900;   // 太陽
    const int PHASE3_END = 1500;  // 決戦

    // ---- 初期化 ----
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.x2 = 240.0;
        enemy.y2 = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // ---- フェーズ切替予告音 ----
    if (count == PHASE1_END || count == PHASE2_END) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ---- 敵の位置更新 ----
    if (count < PHASE1_END) {
        // Phase 1: 北風 - 画面上部で左右に揺れる
        enemy.x = 240.0 + 100.0 * sin(count * 0.02);
        enemy.y = 60.0 + 20.0 * sin(count * 0.03);
        enemy.x2 = -999.0;
        enemy.y2 = -999.0;
    }
    else if (count < PHASE2_END) {
        // Phase 2: 太陽 - 中央付近で大きく揺れる
        enemy.x = -999.0;
        enemy.y = -999.0;
        enemy.x2 = 240.0 + 80.0 * sin((count - PHASE1_END) * 0.015);
        enemy.y2 = 120.0 + 30.0 * cos((count - PHASE1_END) * 0.02);
    }
    else if (count < PHASE3_END) {
        // Phase 3: 決戦 - 中央で小さく動く
        enemy.x = 240.0 + 40.0 * sin((count - PHASE2_END) * 0.03);
        enemy.y = 100.0 + 20.0 * cos((count - PHASE2_END) * 0.04);
        enemy.x2 = 240.0 - 40.0 * sin((count - PHASE2_END) * 0.03);
        enemy.y2 = 100.0 - 20.0 * cos((count - PHASE2_END) * 0.04);
    }

    // ========== Phase 1: 北風 ==========
    if (count < PHASE1_END) {
        // 風の弾：画面左端から右へ流れる
        if (count % 12 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotWind;
            pSet->x = -20.0;
            pSet->y = GetRand(480);
            pSet->muki = 0.0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 冷気の針：敵から扇状
        if (count % 35 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotColdNeedle;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 15.0;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 凍結の息：敵から扇状、大きな弾
        if (count % 80 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotFreezeBreath;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 15.0;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    // ========== Phase 2: 太陽 ==========
    else if (count < PHASE2_END) {
        // 陽光の波：全方位3重拡散
        if (count % 55 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSunWave;
            pSet->x = enemy.x2;
            pSet->y = enemy.y2 + 15.0;
            pSet->muki = 0.0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 熱波の誘導弾
        if (count % 45 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSunHoming;
            pSet->x = enemy.x2;
            pSet->y = enemy.y2 + 15.0;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    // ========== Phase 3: 決戦 ==========
    else if (count < PHASE3_END) {
        // 北風の残り風
        if (count % 18 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotWind;
            pSet->x = -20.0;
            pSet->y = GetRand(480);
            pSet->muki = 0.0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 陽光の波
        if (count % 65 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSunWave;
            pSet->x = enemy.x2;
            pSet->y = enemy.y2 + 15.0;
            pSet->muki = 0.0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 渦巻き弾：北風と太陽が混ざる渦
        if (count % 40 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotVortex;
            pSet->x = (enemy.x + enemy.x2) / 2;
            pSet->y = (enemy.y + enemy.y2) / 2 + 15.0;
            pSet->muki = 0.0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 熱波の誘導弾
        if (count % 55 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSunHoming;
            pSet->x = enemy.x2;
            pSet->y = enemy.y2 + 15.0;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}