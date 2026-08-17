// enemyPat_processingLag.cpp
// 弾幕：「処理落ち」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 処理落ち係数。
// 1.0 = 通常、0.0 に近いほど停止。
static double LagFactor(int frame)
{
    const int t = frame % 300;

    if (t < 90)  return 1.0;
    if (t < 150) return 1.0 - (t - 90) / 80.0;
    if (t < 205) return 0.08;
    if (t < 220) return 0.08 + 0.92 * (t - 205) / 15.0;
    return 1.0;
}

// 実時間ではなく、「処理落ち後の仮想経過フレーム数」を返す。
// これを使うことで、処理落ち係数が変化しても弾がワープしない。
static double LagClock(int frame)
{
    if (frame <= 0) return 0.0;

    const int cycle = frame / 300;
    const int t = frame % 300;
    const double cycleArea =
        90.0 +
        (1.0 + 0.25) * 60.0 / 2.0 +
        0.08 * 55.0 +
        (0.08 + 1.0) * 15.0 / 2.0 +
        80.0;

    double area = cycle * cycleArea;

    if (t < 90) {
        area += t;
    }
    else if (t < 150) {
        const double u = t - 90.0;
        // LagFactor = 1 - u / 80
        area += u - u * u / 160.0;
    }
    else if (t < 205) {
        area += 60.0 - 60.0 * 60.0 / 160.0;
        area += 0.08 * (t - 150.0);
    }
    else if (t < 220) {
        const double before = 60.0 - 60.0 * 60.0 / 160.0;
        area += before + 0.08 * 55.0;
        const double u = t - 205.0;
        // LagFactor = 0.08 + 0.92*u/15
        area += 0.08 * u + 0.92 * u * u / 30.0;
    }
    else {
        area +=
            (60.0 - 60.0 * 60.0 / 160.0) +
            0.08 * 55.0 +
            (0.08 + 1.0) * 15.0 / 2.0;
        area += t - 220.0;
    }

    return area;
}

// 弾の処理
static void ShotProcessingLag(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 弾ごとに保存した初期位置・速度を使い、処理落ち係数を毎フレーム適用する。
        // param_d[0] = 初期角度
        // param_d[1] = 速度
        // param_d[2] = 追加回転量
        // param_d[3] = 放射状の基準距離
        // param_i[0] = 発生時のゲームフレーム
        const double a0 = pShot->param_d[0];
        const double speed = pShot->param_d[1];
        const double spin = pShot->param_d[2];
        const double startR = pShot->param_d[3];
        const double phase = LagClock(count) - LagClock(pShot->param_i[0]);
        const double ang = a0 + spin * phase;
        const double r = startR + speed * phase;

        pShot->x = pEnemyShotSet->x + cos(ang) * r;
        pShot->y = pEnemyShotSet->y + sin(ang) * r;

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Lag_ChatGPT()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 処理落ちに合わせて敵の動きも重くなる。
    const double lag = LagFactor(count);
    const double move = 0.90 * lag;
    const int phase = (count / 180) % 2;
    enemy.x += move * (phase == 0 ? 1.0 : -1.0);

    if (enemy.x < 90.0) enemy.x = 90.0;
    if (enemy.x > 390.0) enemy.x = 390.0;

    // 大量生成前の予兆音と、復帰瞬間の強い音。
    if (count % 300 == 150) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (count % 300 == 205) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 12発の円形弾を周期的に生成。
    // 通常時は軽快に流れ、処理落ち区間ではほぼ停止し、復帰時に一気に進む。
    if (count % 12 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotProcessingLag;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = count / 12;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        for (int i = 0; i < 12; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            const double a = DX_PI * 2.0 * i / 12.0
                + (pEnemyShotSet->kind % 2) * DX_PI / 12.0;

            // 前半は外側へ広がり、後半はわずかに回転しながら進む。
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = a;
            pEnemyShot->speed = 2.2;

            // 種類と色を波ごとに切り替える。
            // 中玉を主体にし、復帰時に見た目が一気に広がるようにする。
            if ((pEnemyShotSet->kind + i) % 5 == 0) {
                pEnemyShot->kind = img_enemyShotLargeBall[6]; // 白・大玉
                pEnemyShot->param_d[1] = 2.6;
            }
            else if ((pEnemyShotSet->kind + i) % 3 == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン・中玉
                pEnemyShot->param_d[1] = 2.2;
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青・小玉
                pEnemyShot->param_d[1] = 2.0;
            }

            pEnemyShot->param_d[0] = a;
            pEnemyShot->param_i[0] = count;
            pEnemyShot->param_d[2] = (i % 2 == 0 ? 0.004 : -0.004);
            pEnemyShot->param_d[3] = 8.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 処理落ち区間だけ、画面中央へ収束する別系統の弾を追加。
    // 「止まっているように見えるが、実際には位置関係が変化している」演出。
    if (count % 20 == 5 && count % 300 >= 110 && count % 300 < 220) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotProcessingLag;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 240.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 1000 + count / 20;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        for (int i = 0; i < 8; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            const double a = DX_PI * 2.0 * i / 8.0 + count * 0.01;

            pEnemyShot->x = 240.0 + cos(a) * 130.0;
            pEnemyShot->y = 240.0 + sin(a) * 130.0;
            pEnemyShot->muki = a;
            pEnemyShot->speed = -1.1;
            pEnemyShot->kind = img_enemyShotMediumOval[5]; // マゼンタ・中楕円
            pEnemyShot->param_d[0] = a;
            pEnemyShot->param_i[0] = count;
            pEnemyShot->param_d[1] = -1.1;
            pEnemyShot->param_d[2] = -0.006;
            pEnemyShot->param_d[3] = 130.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}