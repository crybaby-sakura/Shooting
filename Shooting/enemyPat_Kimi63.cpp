// enemyPat_signpost.cpp
// 弾幕：交差点標識（サインポール）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// サインポール弾幕
// ------------------------------------------------------------
static void ShotSignpost(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ===== 初期化 =====
    if (pEnemyShotSet->count == 0) {
        // 重い展開音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // パラメータ設定
        // directions: 4 or 8（GetRand(1) は 0 or 1 の2種類）
        int directions = 4 + GetRand(1) * 4;
        int signLength = 4 + GetRand(2);           // 標識板の長さ（弾数 4〜6）
        pEnemyShotSet->param_i[0] = directions;    // 方向数
        pEnemyShotSet->param_i[1] = signLength;    // 標識の長さ
        pEnemyShotSet->param_d[0] = pEnemyShotSet->muki; // 基準角度（ラジアン）
        // 回転速度：毎フレーム 0.4〜0.8度、方向はランダム
        pEnemyShotSet->param_d[1] = (GetRand(1) == 0 ? 1.0 : -1.0) * (0.4 + GetRand(40) / 100.0);

        // --- 支柱（中心の中玉） ---
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白
        pEnemyShot->param_i[0] = -1; // 支柱識別子
        pEnemyShot->param_i[1] = 0;
        pEnemyShot->param_i[2] = 2;  // 種別：支柱

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // --- 各方向に標識板を展開 ---
        for (int d = 0; d < directions; d++) {
            double baseAngle = (DX_PI * 2.0 / directions) * d;
            int colorIdx = d % 8; // 方向ごとに色を変える

            for (int i = 0; i < signLength; i++) {
                pEnemyShot = new sEnemyShot;

                double dist = 20.0 + i * 14.0; // 中心からの距離
                pEnemyShot->x = pEnemyShotSet->x + dist * cos(baseAngle);
                pEnemyShot->y = pEnemyShotSet->y + dist * sin(baseAngle);
                pEnemyShot->muki = baseAngle;
                pEnemyShot->speed = 0.0; // 回転中心と一体で動く

                if (i == signLength - 1) {
                    // 先端：中玉（矢印頭）
                    pEnemyShot->kind = img_enemyShotMediumBall[colorIdx];
                    pEnemyShot->param_i[2] = 1; // 種別：先端矢印
                }
                else {
                    // 途中：小玉
                    pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];
                    pEnemyShot->param_i[2] = 0; // 種別：通常
                }

                pEnemyShot->param_i[0] = d; // 方向インデックス
                pEnemyShot->param_i[1] = i; // 標識板内位置

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ===== 回転角度更新 =====
    // param_d[1] は度/フレーム、ラジアンに変換して加算
    pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[1] * DX_PI / 180.0;

    // ===== 分岐弾生成（先端から案内矢印） =====
    // 45フレーム毎、発生後300フレームまで
    int directions = pEnemyShotSet->param_i[0];
    int signLength = pEnemyShotSet->param_i[1];
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 45 == 0 && pEnemyShotSet->count < 600) {
        for (int d = 0; d < directions; d++) {
            double angle = pEnemyShotSet->param_d[0] + (DX_PI * 2.0 / directions) * d;
            double dist = 20.0 + (signLength - 1) * 14.0;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + dist * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + dist * sin(angle);

            // プレイヤー方向を向く（やや誘導）
            double toPlayer = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            // GetRand(20) は 0〜20、-10 で -10〜10、/100.0 で ±0.1ラジアン（約±5.7度）のばらつき
            pEnemyShot->muki = toPlayer + (GetRand(20) - 10) / 100.0;
            pEnemyShot->speed = 1.5 + GetRand(15) / 10.0;

            int colorIdx = d % 8;
            pEnemyShot->kind = img_enemyShotScale[colorIdx]; // 鱗弾で矢印感を演出
            pEnemyShot->param_i[0] = -2; // 分岐弾識別子

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 軽い発射音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // ===== 弾の位置更新 =====
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] >= 0) {
            // --- 標識板の弾：中心＋回転角度から再計算 ---
            int d = pShot->param_i[0];
            int i = pShot->param_i[1];
            double angle = pEnemyShotSet->param_d[0] + (DX_PI * 2.0 / pEnemyShotSet->param_i[0]) * d;
            double dist = 20.0 + i * 14.0;
            pShot->x = pEnemyShotSet->x + dist * cos(angle);
            pShot->y = pEnemyShotSet->y + dist * sin(angle);
            if (pShot->count == 600) {
                pShot->margin = -9999;
            }
        }
        else if (pShot->param_i[0] == -1) {
            // --- 支柱：中心に固定 ---
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            if (pShot->count == 600) {
                pShot->margin = -9999;
            }
        }
        else {
            // --- 分岐弾：直進 ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_SignPole_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆっくり左右移動（画面端で反転）
        enemy.x = 240 + 180 * sin(count * 0.01);
        enemy.y = 100 + 50 * sin(count * 0.035);
    }

    // 150フレーム毎にサインポールを1本生成
    if (count % 150 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSignpost;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}