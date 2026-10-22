#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 噴水弾幕『アクア・ファウンテン』
// ============================================================
static void ShotFountain(sEnemyShotSet* pEnemyShotSet)
{
    // pEnemyShotSet->count はメインルーチンが毎フレーム +1 する
    // 周期 240 フレーム（約4秒）でタイムラインを回す
    int t = pEnemyShotSet->count % 240;

    // 弾をリストに追加するヘルパー
    auto addShot = [&](double x, double y, double angle, double speed,
        double gravity, int kind, int type) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = x;
        pEnemyShot->y = y;
        pEnemyShot->muki = angle;
        pEnemyShot->speed = speed;
        pEnemyShot->count = 0;
        pEnemyShot->kind = kind;
        pEnemyShot->param_i[0] = type; // 0:重力あり, 1:重力なし
        pEnemyShot->param_d[0] = speed * cos(angle); // vx
        pEnemyShot->param_d[1] = speed * sin(angle); // vy
        pEnemyShot->param_d[2] = gravity;            // 重力加速度
        pEnemyShot->margin = 480;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    };

    // --------------------------------------------------------
    // タイムライン
    // --------------------------------------------------------
    if (t == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (t == 12) {
        // 噴水の芯：針弾3本（上方向 ±5°）
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 3*5; i++) {
            double angle = -DX_PI / 2.0 + (GetRand(10) - 5) / 180.0 * DX_PI;
            addShot(pEnemyShotSet->x, pEnemyShotSet->y, angle, 12.0, 0.0,
                img_enemyShotBullet[6], 1); // 白い銃弾
        }
    }
    if (t == 18) {
        // 第1波：小円弾9発（±20°）
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 9*5; i++) {
            double angle = -DX_PI / 2.0 + (i - (4*2+1)*5/2) * (40.0 / 8/5) / 180.0 * DX_PI;
            addShot(pEnemyShotSet->x, pEnemyShotSet->y, angle, 6.0, 0.08,
                img_enemyShotSmallBall[3], 0); // シアンの小玉
        }
    }
    if (t == 30) {
        // 第2波：小円弾13発（±35°）
        for (int i = 0; i < 13*5; i++) {
            double angle = -DX_PI / 2.0 + (i - (6*2+1)*5/2) * (70.0 / 12/5) / 180.0 * DX_PI;
            addShot(pEnemyShotSet->x, pEnemyShotSet->y, angle, 6.5, 0.08,
                img_enemyShotSmallBall[3], 0);
        }
    }
    if (t == 42) {
        // 第3波：小円弾17発（±50°）
        for (int i = 0; i < 17*5; i++) {
            double angle = -DX_PI / 2.0 + (i - (8*2+1)*5/2) * (100.0 / 16/5) / 180.0 * DX_PI;
            addShot(pEnemyShotSet->x, pEnemyShotSet->y, angle, 7.0, 0.08,
                img_enemyShotSmallBall[3], 0);
        }
    }
    if (t == 60) {
        // 大粒の水滴：中円弾を左右に2発ずつ
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 2*5; i++) {
            double angleR = -DX_PI / 2.0 + (20.0 + i * 5.0) / 180.0 * DX_PI;
            double angleL = -DX_PI / 2.0 - (20.0 + i * 5.0) / 180.0 * DX_PI;
            addShot(pEnemyShotSet->x, pEnemyShotSet->y, angleR, 5.0, 0.12,
                img_enemyShotMediumBall[4], 0); // 青の中玉
            addShot(pEnemyShotSet->x, pEnemyShotSet->y, angleL, 5.0, 0.12,
                img_enemyShotMediumBall[4], 0);
        }
    }
    if (t == 90) {
        // 大きな雫：大円弾1発
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        addShot(pEnemyShotSet->x, pEnemyShotSet->y, -DX_PI / 2.0, 4.0, 0.15,
            img_enemyShotLargeBall[4], 0); // 青の大玉
    }
    if (t == 150) {
        // 水盤から溢れる：全方向12発
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 12*5; i++) {
            double angle = i * (2.0 * DX_PI / 12/5);
            double speed = 3.0;
            // 下向き（90°）に近い弾は速度を落とす
            double diff = fabs(angle - DX_PI / 2.0);
            if (diff > DX_PI) diff = 2.0 * DX_PI - diff;
            if (diff < DX_PI / 6.0) speed = 1.5;
            addShot(pEnemyShotSet->x, pEnemyShotSet->y, angle, speed, 0.05,
                img_enemyShotSmallBall[3], 0);
        }
    }

    // --------------------------------------------------------
    // 弾の移動
    // --------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 重力あり：放物線
            pShot->param_d[1] += pShot->param_d[2];
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
            pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
            pShot->speed = sqrt(pShot->param_d[0] * pShot->param_d[0] +
                pShot->param_d[1] * pShot->param_d[1]);
        }
        else {
            // 重力なし：直進
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Fountain_DeepSeek()
{
    static int muki;
    static sEnemyShotSet* pFountainSet = nullptr;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 220.0; // 噴水口の位置
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;

        // 噴水用のセットを1つ作成
        pFountainSet = new sEnemyShotSet;
        pFountainSet->count = 0;
        pFountainSet->patternFunc = ShotFountain;
        pFountainSet->x = enemy.x;
        pFountainSet->y = enemy.y;
        pFountainSet->muki = 0.0;
        pFountainSet->kind = 0;
        pFountainSet->pEnemyShotHead = new sEnemyShot;
        pFountainSet->pEnemyShotHead->prev = pFountainSet->pEnemyShotHead;
        pFountainSet->pEnemyShotHead->next = pFountainSet->pEnemyShotHead;
        pFountainSet->prev = enemyShotSetHead.prev;
        pFountainSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pFountainSet;
        enemyShotSetHead.prev = pFountainSet;
    }
    else {
        // 敵の左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;

        // 噴水口を敵に追従させる
        if (pFountainSet) {
            pFountainSet->x = enemy.x;
            pFountainSet->y = enemy.y;
        }
    }
}