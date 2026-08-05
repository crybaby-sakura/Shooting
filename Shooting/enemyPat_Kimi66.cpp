// enemyPat_GravityEye.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 超巨大弾弾幕：重力の瞳（グラヴィティ・アイズ）
// ------------------------------------------------------------
//  画面中央に超巨大な眼球を出現させ、3フェーズの攻撃を繰り返す。
//  ・フェーズ1「凝視」      ：瞳がプレイヤーを追い、3方向レーザー
//  ・フェーズ2「涙の雨」    ：8方向に青い大玉（雫）→ 進むと分裂
//  ・フェーズ3「虚空のまばたき」：白目が縮小し、十字／斜め十字の衝撃波
//  常時：中心への弱引力フィールド＋白目の回転＋瞳の追従
// ------------------------------------------------------------

static void GravityEye(sEnemyShotSet* pSet)
{
    // param_i[0] : フェーズ (0=出現, 1=凝視, 2=涙, 3=まばたき)
    // param_i[1] : フェーズ内タイマー
    // param_d[0] : 中心 X
    // param_d[1] : 中心 Y

    if (pSet->count == 0) {
        pSet->param_d[0] = 240.0;   // 画面中央固定
        pSet->param_d[1] = 240.0;
        pSet->param_i[0] = 0;       // フェーズ0：出現
        pSet->param_i[1] = 0;

        // --- 中心核（大玉・赤） ---
        sEnemyShot* pCore = new sEnemyShot;
        pCore->x = pSet->param_d[0];
        pCore->y = pSet->param_d[1];
        pCore->muki = 0.0;
        pCore->speed = 0.0;
        pCore->kind = img_enemyShotLargeBall[0];   // 0:赤
        pCore->margin = 100.0;
        pCore->param_i[0] = 0;                     // 0:核
        pCore->prev = pSet->pEnemyShotHead->prev;
        pCore->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pCore;
        pSet->pEnemyShotHead->prev = pCore;

        // --- 白目（中玉・白）16個で同心円 ---
        for (int i = 0; i < 16; i++) {
            double angle = DX_PI * 2.0 * i / 16.0;
            sEnemyShot* pWhite = new sEnemyShot;
            pWhite->x = pSet->param_d[0] + cos(angle) * 20.0;
            pWhite->y = pSet->param_d[1] + sin(angle) * 20.0;
            pWhite->muki = angle;
            pWhite->speed = 0.0;
            pWhite->kind = img_enemyShotMediumBall[6]; // 6:白
            pWhite->margin = 100.0;
            pWhite->param_i[0] = 1;                    // 1:白目
            pWhite->param_d[0] = angle;                // 初期角度記録
            pWhite->prev = pSet->pEnemyShotHead->prev;
            pWhite->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pWhite;
            pSet->pEnemyShotHead->prev = pWhite;
        }

        // --- 瞳（小玉・黒） ---
        sEnemyShot* pPupil = new sEnemyShot;
        pPupil->x = pSet->param_d[0];
        pPupil->y = pSet->param_d[1];
        pPupil->muki = 0.0;
        pPupil->speed = 0.0;
        pPupil->kind = img_enemyShotSmallBall[7];  // 7:黒
        pPupil->margin = 100.0;
        pPupil->param_i[0] = 2;                    // 2:瞳
        pPupil->prev = pSet->pEnemyShotHead->prev;
        pPupil->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pPupil;
        pSet->pEnemyShotHead->prev = pPupil;

        // 出現予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    int& phase = pSet->param_i[0];
    int& phaseTimer = pSet->param_i[1];

    // --- フェーズ遷移 ---
    if (phase == 0 && pSet->count >= 60) {      // 出現 60F
        phase = 1; phaseTimer = 0;
    }
    else if (phase == 1 && phaseTimer >= 180) { // 凝視 180F
        phase = 2; phaseTimer = 0;
    }
    else if (phase == 2 && phaseTimer >= 240) { // 涙 240F
        phase = 3; phaseTimer = 0;
    }
    else if (phase == 3 && phaseTimer >= 180) { // まばたき 180F → ループ
        phase = 1; phaseTimer = 0;
    }

    // ============================================================
    // 全フェーズ共通：目玉パーツの更新
    // ============================================================
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int part = pShot->param_i[0];

        if (part == 0) { // 核（大玉・赤）：中心固定
            pShot->x = pSet->param_d[0];
            pShot->y = pSet->param_d[1];
        }
        else if (part == 1) { // 白目（中玉・白）：回転＋まばたき縮小
            double baseAngle = pShot->param_d[0];
            double rot = pSet->count * 0.015;
            double radius = 20.0;

            // フェーズ3で「まばたき」：25~35F と 85~95F で半径縮小
            if (phase == 3) {
                if ((phaseTimer >= 25 && phaseTimer <= 35) ||
                    (phaseTimer >= 85 && phaseTimer <= 95)) {
                    radius = 10.0;
                }
            }
            pShot->x = pSet->param_d[0] + cos(baseAngle + rot) * radius;
            pShot->y = pSet->param_d[1] + sin(baseAngle + rot) * radius;
            pShot->muki = baseAngle + rot;
        }
        else if (part == 2) { // 瞳（小玉・黒）：プレイヤー方向を追従
            double dx = player.x - pSet->param_d[0];
            double dy = player.y - pSet->param_d[1];
            double dist = sqrt(dx * dx + dy * dy);
            double r = 6.0; // 瞳の移動半径
            if (dist > 0.1) {
                pShot->x = pSet->param_d[0] + (dx / dist) * r;
                pShot->y = pSet->param_d[1] + (dy / dist) * r;
            }
            pShot->muki = atan2(dy, dx);
        }
        pShot = pShot->next;
    }

    // ============================================================
    // 引力フィールド（常時）
    // ============================================================
    double pdx = player.x - pSet->param_d[0];
    double pdy = player.y - pSet->param_d[1];
    double pdist = sqrt(pdx * pdx + pdy * pdy);
    if (pdist < 90.0 && pdist > 1.0) {
        double pull = 0.25 * (90.0 - pdist) / 90.0;
        player.x -= (pdx / pdist) * pull;
        player.y -= (pdy / pdist) * pull;
    }

    // ============================================================
    // フェーズ1：凝視（レーザー）
    // ============================================================
    if (phase == 1) {
        // 60Fごとに3方向レーザー（予告→発射）
        if (phaseTimer % 60 == 30) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            double baseAngle = atan2(player.y - pSet->param_d[1],
                player.x - pSet->param_d[0]);
            for (int i = -1; i <= 1; i++) {
                double angle = baseAngle + i * DX_PI / 6.0; // ±30°
                sEnemyShot* pLaser = new sEnemyShot;
                pLaser->x = pSet->param_d[0] + cos(angle) * 24.0;
                pLaser->y = pSet->param_d[1] + sin(angle) * 24.0;
                pLaser->muki = angle;
                pLaser->speed = 0.0;                       // 予告：静止
                pLaser->kind = img_enemyShotLaser[1];      // 1:黄（予告線）
                pLaser->param_i[0] = 10;                   // 10:レーザー予告
                pLaser->param_i[1] = 30;                   // 30F後に発射
                pLaser->margin = 100.0;
                pLaser->prev = pSet->pEnemyShotHead->prev;
                pLaser->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pLaser;
                pSet->pEnemyShotHead->prev = pLaser;
            }
        }
    }

    // ============================================================
    // フェーズ2：涙の雨
    // ============================================================
    if (phase == 2) {
        if (phaseTimer % 40 == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 8; i++) {
                double angle = DX_PI * 2.0 * i / 8.0 + pSet->count * 0.03;
                sEnemyShot* pDrop = new sEnemyShot;
                pDrop->x = pSet->param_d[0] + cos(angle) * 24.0;
                pDrop->y = pSet->param_d[1] + sin(angle) * 24.0;
                pDrop->muki = angle;
                pDrop->speed = 1.2;
                pDrop->kind = img_enemyShotLargeBall[4]; // 4:青（大玉＝雫）
                pDrop->param_i[0] = 20;                  // 20:雫
                pDrop->param_i[1] = 0;                   // 生存カウンタ
                pDrop->param_d[0] = pDrop->x;            // 射出時X
                pDrop->param_d[1] = pDrop->y;            // 射出時Y
                pDrop->margin = 50.0;
                pDrop->prev = pSet->pEnemyShotHead->prev;
                pDrop->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pDrop;
                pSet->pEnemyShotHead->prev = pDrop;
            }
        }
    }

    // ============================================================
    // フェーズ3：虚空のまばたき
    // ============================================================
    if (phase == 3) {
        // 30F と 90F で衝撃波（十字 → 斜め十字）
        if (phaseTimer == 30 || phaseTimer == 90) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            bool diagonal = (phaseTimer == 90);
            double offset = diagonal ? DX_PI / 4.0 : 0.0;
            for (int i = 0; i < 4; i++) {
                double angle = DX_PI * 2.0 * i / 4.0 + offset;
                for (int j = 0; j < 6; j++) {
                    sEnemyShot* pWave = new sEnemyShot;
                    pWave->x = pSet->param_d[0] + cos(angle) * (8.0 + j * 10.0);
                    pWave->y = pSet->param_d[1] + sin(angle) * (8.0 + j * 10.0);
                    pWave->muki = angle;
                    pWave->speed = 3.5 + j * 0.8;
                    pWave->kind = img_enemyShotLaser[6]; // 6:白（衝撃波）
                    pWave->param_i[0] = 30;              // 30:衝撃波
                    pWave->margin = 50.0;
                    pWave->prev = pSet->pEnemyShotHead->prev;
                    pWave->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pWave;
                    pSet->pEnemyShotHead->prev = pWave;
                }
            }
        }
    }

    // ============================================================
    // 弾移動・状態遷移（全フェーズ共通後処理）
    // ============================================================
    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int type = pShot->param_i[0];

        // --- レーザー予告 → 実体 ---
        if (type == 10) {
            pShot->param_i[1]--;
            if (pShot->param_i[1] <= 0) {
                pShot->speed = 7.0;
                pShot->kind = img_enemyShotLaser[0]; // 0:赤（実体）
                pShot->param_i[0] = 11;              // 11:レーザー実体
                if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
            }
        }
        else if (type == 11) { // レーザー実体移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        // --- 雫 → 分裂 ---
        else if (type == 20) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->param_i[1]++;

            double dx = pShot->x - pShot->param_d[0];
            double dy = pShot->y - pShot->param_d[1];
            double moved = sqrt(dx * dx + dy * dy);

            // 50px 進むか 50F 経過で分裂
            if (moved > 50.0 || pShot->param_i[1] > 50) {
                double baseAngle = pShot->muki;

                // 元の雫を中心散弾に変化
                pShot->param_i[0] = 21;
                pShot->speed = 2.0;
                pShot->kind = img_enemyShotSmallBall[4]; // 4:青（小玉）
                pShot->margin = 20.0;

                // 左右に追加散弾
                for (int j = -5; j <= 5; j++) {
                    if (j == 0) continue;
                    sEnemyShot* pFrag = new sEnemyShot;
                    pFrag->x = pShot->x;
                    pFrag->y = pShot->y;
                    pFrag->muki = baseAngle + j * DX_PI / 10.0;
                    pFrag->speed = 2.0;
                    pFrag->kind = img_enemyShotSmallBall[4]; // 4:青
                    pFrag->param_i[0] = 21;                  // 21:散弾
                    pFrag->margin = 20.0;
                    pFrag->prev = pSet->pEnemyShotHead->prev;
                    pFrag->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pFrag;
                    pSet->pEnemyShotHead->prev = pFrag;
                }

                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
        }
        else if (type == 21) { // 散弾移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        // --- 衝撃波 ---
        else if (type == 30) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }

    phaseTimer++;
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_HugeBullet_Kimi()
{
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        shot_count = 0;
    }
    else {
        // 敵本体は中央上部で小刻みに左右に揺れる
        enemy.x = 240.0 + sin(count * 0.02) * 20.0;
        enemy.y = 40.0 + cos(count * 0.03) * 5.0;
    }

    // 1回だけ巨大弾セットを生成（count == 60 で出現）
    if (count == 60 && shot_count == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = GravityEye;
        pSet->x = 240.0;
        pSet->y = 240.0;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        shot_count = 1;
    }
}