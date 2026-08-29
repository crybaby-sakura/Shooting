// enemyPat_futon.cpp
// 弾幕パターン：「朝の覚醒—布団吹飛び」
// 布団被覆 → 一斉吹っ飛び → 羽毛漂い の3段階演出弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 布団弾幕パターン
// ------------------------------------------------------------
static void ShotFuton(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // param_d[0], param_d[1] : 布団中心（生成時の敵位置）
    double centerX = pEnemyShotSet->param_d[0];
    double centerY = pEnemyShotSet->param_d[1];
    double baseAngle = pEnemyShotSet->muki; // 生成時の自機方向

    // ========================================================
    // 初期生成（count == 0）：布団被覆フェーズ
    // ========================================================
    if (pEnemyShotSet->count == 0) {
        // 予告音：布団が被さってくる予兆
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 布団中心を記録
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;

        double a = 100.0; // 短軸半径
        double b = 170.0; // 長軸半径（自機方向）

        // --- 外周：高密度（縁が厚い布団の縁部分）---
        int outerCount = 40 + 20;
        for (int i = 0; i < outerCount; i++) {
            pEnemyShot = new sEnemyShot;

            double theta = 2.0 * DX_PI * i / outerCount;
            double rx = a * cos(theta); // 短軸方向成分
            double ry = b * sin(theta); // 長軸方向成分

            // 長軸をbaseAngle方向に回転
            double offsetX = -rx * sin(baseAngle) + ry * cos(baseAngle);
            double offsetY = rx * cos(baseAngle) + ry * sin(baseAngle);

            // わずかなランダムで布団の厚み演出
            offsetX += GetRand(20) - 10;
            offsetY += GetRand(20) - 10;

            pEnemyShot->x = pEnemyShotSet->x + offsetX;
            pEnemyShot->y = pEnemyShotSet->y + offsetY;

            // 自機方向へゆっくり移動（布団が被さってくる）
            pEnemyShot->muki = baseAngle + (GetRand(16) - 8) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.0 + GetRand(10) / 100.0;

            // 中楕円弾(10.5x7.0)、白色（布団っぽい色）
            pEnemyShot->kind = img_enemyShotMediumOval[6];
            
            // 弾種別フラグ：0=布団弾
            pEnemyShot->param_i[0] = 0;
            pEnemyShot->margin = 240;

            // リンク追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // --- 内側：低密度（中心が薄い）---
        int innerCount = 10;
        for (int i = 0; i < innerCount; i++) {
            pEnemyShot = new sEnemyShot;

            double r = GetRand(75) / 100.0 * a;
            double theta = GetRand(359) / 180.0 * DX_PI;
            double rx = r * cos(theta);
            double ry = r * sin(theta) * (b / a);

            double offsetX = -rx * sin(baseAngle) + ry * cos(baseAngle);
            double offsetY = rx * cos(baseAngle) + ry * sin(baseAngle);

            pEnemyShot->x = pEnemyShotSet->x + offsetX;
            pEnemyShot->y = pEnemyShotSet->y + offsetY;

            pEnemyShot->muki = baseAngle + (GetRand(24) - 12) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.9 + GetRand(10) / 100.0;

            pEnemyShot->kind = img_enemyShotMediumOval[6];
            pEnemyShot->param_i[0] = 0;
            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ========================================================
    // フェーズ1：布団移動（count 1 ～ 119）
    // ========================================================
    if (pEnemyShotSet->count >= 1 && pEnemyShotSet->count < 120) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) { // 布団弾のみ
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }

    // ========================================================
    // フェーズ2：吹っ飛び（count == 120）
    // ========================================================
    if (pEnemyShotSet->count == 120) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) { // 布団弾
                // 布団中心から弾への方向（放射状外側へ吹っ飛ぶ）
                pShot->muki = atan2(pShot->y - centerY, pShot->x - centerX);
                // 急加速
                pShot->speed = 4.5 + GetRand(35) / 10.0;
                // わずかなばらつき
                pShot->muki += (GetRand(20) - 10) / 180.0 * DX_PI;
            }
            pShot = pShot->next;
        }
    }

    // ========================================================
    // フェーズ2継続：吹っ飛んだ弾の移動（count 121 ～ 124）
    // ========================================================
    if (pEnemyShotSet->count > 120 && pEnemyShotSet->count < 125) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }

    // ========================================================
    // フェーズ3：羽毛生成（count == 125）
    // ========================================================
    if (pEnemyShotSet->count == 125) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // --- 有効羽毛弾 ---
        int featherCount = 50;
        for (int i = 0; i < featherCount; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = 2.0 * DX_PI * i / featherCount + (GetRand(10) - 5) / 180.0 * DX_PI;
            double dist = 5.0 + GetRand(25);

            pEnemyShot->x = centerX + dist * cos(angle);
            pEnemyShot->y = centerY + dist * sin(angle);

            pEnemyShot->muki = angle;
            double initSpeed = 2.2 + GetRand(28) / 10.0;
            pEnemyShot->speed = initSpeed;

            // 菱形弾(4.5x2.5)、白色（羽毛）
            pEnemyShot->kind = img_enemyShotDiamond[6];

            // 弾種別：1=羽毛弾
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->param_d[0] = initSpeed; // 初期速度（減速用）
            pEnemyShot->param_d[1] = GetRand(359) / 180.0 * DX_PI; // 漂い位相

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // --- 無害羽毛（すり抜ける演出用、約20%）---
        // ※当たり判定の有無はメインルーチン側で param_i[0]==2 を参照して判定してください
        int harmlessCount = 12;
        for (int i = 0; i < harmlessCount; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = GetRand(359) / 180.0 * DX_PI;
            double dist = 5.0 + GetRand(35);

            pEnemyShot->x = centerX + dist * cos(angle);
            pEnemyShot->y = centerY + dist * sin(angle);

            pEnemyShot->muki = angle;
            double initSpeed = 1.8 + GetRand(20) / 10.0;
            pEnemyShot->speed = initSpeed;

            // 小玉(2.5x2.5)、白色（軽い羽毛）
            pEnemyShot->kind = img_enemyShotSmallBall[6];

            // 弾種別：2=無害羽毛
            pEnemyShot->param_i[0] = 2;
            pEnemyShot->param_d[0] = initSpeed;
            pEnemyShot->param_d[1] = GetRand(359) / 180.0 * DX_PI;
            pEnemyShot->margin = 10.0; // 画面外判定を小さくして早く消す

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ========================================================
    // フェーズ3継続：羽毛の漂い（count >= 126）
    // ========================================================
    if (pEnemyShotSet->count >= 126) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {         // 有効羽毛弾
                // 減速
                pShot->speed *= 0.985;
                if (pShot->speed < 0.25) pShot->speed = 0.25;

                // ゆらゆら漂う
                pShot->param_d[1] += 0.025;
                pShot->muki += sin(pShot->param_d[1]) * 0.012;

                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (pShot->param_i[0] == 2) {    // 無害羽毛
                pShot->speed *= 0.98;
                if (pShot->speed < 0.15) pShot->speed = 0.15;

                pShot->param_d[1] += 0.035;
                pShot->muki += sin(pShot->param_d[1]) * 0.018;

                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (pShot->param_i[0] == 0) {    // 吹っ飛んだ布団弾（等速のまま画面外へ）
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_FutonFlewAway_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆっくり左右移動
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;

        // 画面端クランプ（安全策）
        if (enemy.x < 40.0) { enemy.x = 40.0; muki = 1; }
        if (enemy.x > 440.0) { enemy.x = 440.0; muki = -1; }
    }

    // 約4秒ごとに布団弾幕を発動
    if (count % 240 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFuton;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        // パラメータ初期化
        for (int i = 0; i < 16; i++) {
            pEnemyShotSet->param_i[i] = 0;
            pEnemyShotSet->param_d[i] = 0.0;
        }

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}