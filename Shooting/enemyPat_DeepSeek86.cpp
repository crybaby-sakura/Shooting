// enemyPat_tmp.cpp
// 近接乱舞「斬鉄残光」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾追加ヘルパー
// ------------------------------------------------------------
static void AddEnemyShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->count = 0;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// ------------------------------------------------------------
// 各攻撃パターン
// ------------------------------------------------------------

// 突進残置弾：7個の赤い弾を残し、0.8秒後に8方向へ炸裂
static void PatternResidual(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        double sx = pSet->param_d[0];
        double sy = pSet->param_d[1];
        double tx = pSet->param_d[2];
        double ty = pSet->param_d[3];

        for (int i = 0; i < 7; i++) {
            double t = (double)i / 6.0;
            double x = sx + (tx - sx) * t;
            double y = sy + (ty - sy) * t;
            // 赤い小玉（色0）
            AddEnemyShot(pSet, x, y, 0.0, 0.0, img_enemyShotSmallBall[0]);
            pSet->pEnemyShotHead->prev->param_i[0] = 1;
        }
    }

    if (pSet->count == 48) { // 0.8秒後に炸裂
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 現在リストにある残置弾を収集
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            if (pShot->param_i[0] == 1) {
                // 8方向に低速で炸裂（橙の小玉）
                for (int j = 0; j < 8; j++) {
                    double angle = j * (2.0 * DX_PI / 8.0);
                    AddEnemyShot(pSet, pShot->x, pShot->y, angle, 80.0 / 40, img_enemyShotSmallBall[8]);
                }
                // 元の残置弾を削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
            pShot = pNext;
        }
    }

    // 全弾を移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 居合い斬り：前方180度に12発の刃弾
static void PatternSlash(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        double baseAngle = pSet->muki; // 自機方向
        double spread = DX_PI;         // 180度
        int num = 12;
        double step = spread / (num - 1);

        for (int i = 0; i < num; i++) {
            double angle = baseAngle - spread * 0.5 + step * i;
            // 白銀の菱形弾（白色6）
            AddEnemyShot(pSet, pSet->x, pSet->y, angle, 160.0 / 40, img_enemyShotDiamond[6]);
        }
    }

    // 全弾を移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 背後牽制：90度間隔に4つの隙間を持つ24発の全方位弾
static void PatternSupport(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        double gapSize = DX_PI / 9.0; // 20度の隙間
        int groups = 4;
        int bulletsPerGroup = 6;

        for (int g = 0; g < groups; g++) {
            double base = g * (DX_PI / 2.0) + gapSize * 0.5;
            double range = DX_PI / 2.0 - gapSize;
            for (int j = 0; j < bulletsPerGroup; j++) {
                double angle = base + range * (double)j / (bulletsPerGroup - 1);
                // 淡い紫＝マゼンタ小玉（色5）
                AddEnemyShot(pSet, pSet->x, pSet->y, angle, 120.0 / 40, img_enemyShotSmallBall[5]);
            }
        }
    }

    // 全弾を移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 離脱時の自機狙い3連射
static void PatternNeedles(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        double baseAngle = pSet->muki;
        double offsets[3] = { -10.0 * DX_PI / 180.0, 0.0, 10.0 * DX_PI / 180.0 };
        for (int i = 0; i < 3; i++) {
            // 黄色の銃弾（色1）
            AddEnemyShot(pSet, pSet->x, pSet->y, baseAngle + offsets[i], 240.0 / 40, img_enemyShotBullet[1]);
        }
    }

    // 全弾を移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// ボス本体パターン
// ------------------------------------------------------------
void EnemyPat_CloseCombat_DeepSeek()
{
    static int phase = 0;
    static int phaseCount = 0;
    static double dashStartX, dashStartY, dashTargetX, dashTargetY;
    static double retreatStartX, retreatStartY, retreatTargetX, retreatTargetY;

    // 初期化（count == 1 で行う）
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        phaseCount = 0;
    }

    phaseCount++;

    switch (phase) {
    case 0: // 予兆：0.5秒停止
        if (phaseCount == 1) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 移動しない
        if (phaseCount >= 30) {
            dashStartX = enemy.x;
            dashStartY = enemy.y;
            dashTargetX = player.x;
            dashTargetY = player.y;
            phase = 1;
            phaseCount = 0;
        }
        break;

    case 1: // 突進斬り
    {
        int dashDuration = 20;
        if (phaseCount <= dashDuration) {
            double t = (double)phaseCount / dashDuration;
            enemy.x = dashStartX + (dashTargetX - dashStartX) * t;
            enemy.y = dashStartY + (dashTargetY - dashStartY) * t;

            // 突進開始時に残置弾パターンを生成
            if (phaseCount == 1) {
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = PatternResidual;
                pSet->x = dashStartX;
                pSet->y = dashStartY;
                pSet->muki = 0.0;
                pSet->param_d[0] = dashStartX;
                pSet->param_d[1] = dashStartY;
                pSet->param_d[2] = dashTargetX;
                pSet->param_d[3] = dashTargetY;

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }

            if (phaseCount == dashDuration) {
                phase = 2;
                phaseCount = 0;
            }
        }
        break;
    }

    case 2: // 居合い斬り：0.3秒溜め
        if (phaseCount >= 18) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            // 居合い斬りパターン生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = PatternSlash;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;

            phase = 3;
            phaseCount = 0;
        }
        break;

    case 3: // 背後牽制：0.4秒後
        if (phaseCount >= 24) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            // 全方位弾パターン生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = PatternSupport;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;

            // 離脱目標を設定
            retreatStartX = enemy.x;
            retreatStartY = enemy.y;
            retreatTargetX = 240.0; // 画面上部中央へ退く
            retreatTargetY = 60.0;

            phase = 4;
            phaseCount = 0;
        }
        break;

    case 4: // 離脱
    {
        int retreatDuration = 30;
        if (phaseCount <= retreatDuration) {
            double t = (double)phaseCount / retreatDuration;
            enemy.x = retreatStartX + (retreatTargetX - retreatStartX) * t;
            enemy.y = retreatStartY + (retreatTargetY - retreatStartY) * t;

            // 離脱開始時に自機狙い弾を生成
            if (phaseCount == 1) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = PatternNeedles;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }

            if (phaseCount == retreatDuration) {
                // ループするため初期位置へ戻す
                enemy.x = 240.0;
                enemy.y = 80.0;
                phase = 0;
                phaseCount = 0;
            }
        }
        break;
    }
    }
}