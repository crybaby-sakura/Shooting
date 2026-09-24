// enemyPat_ringToss.cpp
// 弾幕：輪投げ「スピニング・リング・トス」
//
// 画面内に杭（大玉）を3本配置し、敵から輪状に並んだ小弾群を投げる。
// 輪は回転しながら杭へ飛び、杭に重なると半径を縮めてハマり、
// 最後に外側へ弾けて周囲に弾の輪を作る。
//
// 使用素材:
//   弾  : img_enemyShotLargeBall （杭 / 色は param_i[0]）
//         img_enemyShotSmallBall （輪 / 色は param_i[2]）
//   音  : sound_enemyShot_heavy （杭の設置）
//         sound_enemyShot_light （輪を投げる）
//         sound_enemyShot_medium（輪が弾ける）
//
// 注意:
//   - count, pEnemyShotSet->count, pEnemyShot->count のインクリメント、
//     画面外弾の削除はメインルーチン側で行われる前提。
//   - GetRand(x) は 0..x の x+1 種類を返す仕様だが、本パターンでは
//     再現性重視のため乱数は使わず、杭の位置・輪の色を固定サイクルにしている。

#include "gv.h"
#include <DxLib.h>
#include <cmath>

// ------------------------------------------------------------
// 杭（大玉を静止配置）
// ------------------------------------------------------------
static void ShotPeg(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        // 杭：大玉（色は param_i[0] で指定）
        pEnemyShot->kind = img_enemyShotLargeBall[pEnemyShotSet->param_i[0] % 9];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }
    // 杭は静止しているので毎フレームの移動処理は不要
}

// ------------------------------------------------------------
// 輪（小弾を円形に並べ、回転しながら杭へ飛ばす）
// ------------------------------------------------------------
static void ShotRing(sEnemyShotSet* pEnemyShotSet)
{
    const int RING_N = 12*4; // 輪を構成する弾数

    // param_d の用途:
    //   [0] 中心x, [1] 中心y
    //   [2] 現在の半径
    //   [3] 回転角
    //   [4] 初期半径, [5] 最終半径（杭にハマったときの半径）
    //   [6] ターゲットx, [7] ターゲットy   ※呼び出し側で設定
    //   [8] 縮小進捗(0..1)
    // param_i の用途:
    //   [0] フェーズ (0=飛翔, 1=縮小, 2=弾け)
    //   [1] タイマー
    //   [2] 輪の色  ※呼び出し側で設定

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
        pEnemyShotSet->param_d[2] = 50.0; // 初期半径
        pEnemyShotSet->param_d[3] = 0.0;  // 回転角
        pEnemyShotSet->param_d[4] = 50.0;
        pEnemyShotSet->param_d[5] = 12.0; // 杭にハマったときの半径
        // param_d[6], [7] は呼び出し側でターゲット設定済み
        pEnemyShotSet->param_d[8] = 0.0;
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;

        const int color = pEnemyShotSet->param_i[2] % 9;

        for (int i = 0; i < RING_N; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->param_d[0];
            pEnemyShot->y = pEnemyShotSet->param_d[1];
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotSmallBall[color];
            // 輪の中での基準角
            pEnemyShot->param_d[0] = (2.0 * DX_PI * i) / RING_N;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    const double targetX = pEnemyShotSet->param_d[6];
    const double targetY = pEnemyShotSet->param_d[7];

    if (pEnemyShotSet->param_i[0] == 0) {
        // --- 飛翔フェーズ: 中心を杭へ近づける ---
        const double dx = targetX - pEnemyShotSet->param_d[0];
        const double dy = targetY - pEnemyShotSet->param_d[1];
        const double dist = sqrt(dx * dx + dy * dy);
        const double moveSpeed = 3.5;

        if (dist > moveSpeed) {
            pEnemyShotSet->param_d[0] += dx / dist * moveSpeed;
            pEnemyShotSet->param_d[1] += dy / dist * moveSpeed;
        }
        else {
            pEnemyShotSet->param_d[0] = targetX;
            pEnemyShotSet->param_d[1] = targetY;
            pEnemyShotSet->param_i[0] = 1; // 縮小フェーズへ
            pEnemyShotSet->param_i[1] = 0;
        }
        pEnemyShotSet->param_d[3] += 0.25; // 飛翔中も回転

    }
    else if (pEnemyShotSet->param_i[0] == 1) {
        // --- 縮小フェーズ: 杭にハマるように半径を縮める ---
        pEnemyShotSet->param_i[1]++;
        double t = pEnemyShotSet->param_i[1] / 30.0;
        if (t >= 1.0) {
            t = 1.0;
            pEnemyShotSet->param_i[0] = 2; // 弾けフェーズへ

            // 各弾に外向きの速度を与える
            sEnemyShot* pS = pEnemyShotSet->pEnemyShotHead->next;
            while (pS != pEnemyShotSet->pEnemyShotHead) {
                const double ang = pEnemyShotSet->param_d[3] + pS->param_d[0];
                pS->muki = ang;
                pS->speed = 2.8;
                pS = pS->next;
            }
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        pEnemyShotSet->param_d[2] = pEnemyShotSet->param_d[4]
            + (pEnemyShotSet->param_d[5] - pEnemyShotSet->param_d[4]) * t;
        pEnemyShotSet->param_d[3] += 0.4;
    }
    // フェーズ2（弾け）は速度に任せて移動

    // --- 弾の位置更新 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->param_i[0] == 0 || pEnemyShotSet->param_i[0] == 1) {
            // 中心＋半径＋角度で配置（輪を描く）
            const double ang = pEnemyShotSet->param_d[3] + pShot->param_d[0];
            pShot->x = pEnemyShotSet->param_d[0] + pEnemyShotSet->param_d[2] * cos(ang);
            pShot->y = pEnemyShotSet->param_d[1] + pEnemyShotSet->param_d[2] * sin(ang);
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_RingToss_DeepSeek()
{
    static int    muki;
    static int    shot_count;
    static double pegX[3];
    static double pegY[3];
    static int    nextPeg;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        muki = 1;
        shot_count = 0;
        nextPeg = 0;

        // 杭の位置を3か所決める
        pegX[0] = 100.0; pegY[0] = 200.0;
        pegX[1] = 240.0; pegY[1] = 290.0+100;
        pegX[2] = 380.0; pegY[2] = 200.0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // --- 杭を3本配置（ゲーム開始直後に1回だけ） ---
    if (count == 30) {
        for (int i = 0; i < 3; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotPeg;
            pEnemyShotSet->x = pegX[i];
            pEnemyShotSet->y = pegY[i];
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = 0;
            pEnemyShotSet->param_i[0] = 3; // 大玉の色: シアン

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }

    // --- 輪を投げる ---
    if (count > 60 && count % 70 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRing;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count;

        // 輪のターゲット（杭）の座標
        pEnemyShotSet->param_d[6] = pegX[nextPeg];
        pEnemyShotSet->param_d[7] = pegY[nextPeg];
        // 輪の色（0..8 をサイクル）
        pEnemyShotSet->param_i[2] = (shot_count + 1) % 9;

        nextPeg = (nextPeg + 1) % 3;
        shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}