// enemyPat_Tmp.cpp
// 弾幕：北風と太陽

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 北風：青い小玉を横方向へ激しく吹かせる
// ============================================================
static void ShotNorthWind(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 17; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            double t = (double)i / 16.0;
            pShot->x = pEnemyShotSet->x + (t - 0.5) * 70.0;
            pShot->y = pEnemyShotSet->y + 5.0;

            // プレイヤーへ向かう角度を基準に、風の横成分を大きく与える。
            double aim = atan2(player.y - pShot->y, player.x - pShot->x);
            double side = (t - 0.5) * 1.8;
            pShot->muki = aim + side;
            pShot->speed = 1.8 + 0.12 * (double)(i % 5);

            // 青い小玉：北風らしい細かい風の流れを表現
            pShot->kind = img_enemyShotSmallBall[4];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 風に押し流されるように、時間とともに左右の振れ幅を増やす。
        double wind = sin((pShot->count + pShot->x * 0.03) * 0.055) * 0.025;
        pShot->muki += wind;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 太陽：黄色～橙色の中玉がゆっくり中心へ集まる
// ============================================================
static void ShotSun(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const int N = 14;

        for (int i = 0; i < N; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            double a = DX_PI * 2.0 * (double)i / (double)N;
            double r = 125.0 + (double)(i % 3) * 12.0;

            pShot->x = pEnemyShotSet->x + cos(a) * r;
            pShot->y = pEnemyShotSet->y + sin(a) * r;
            pShot->muki = a;
            pShot->speed = 0.55 + (double)(i % 4) * 0.05;

            // 奇数だけ橙、偶数を黄にして太陽らしい色の輪にする。
            pShot->kind = (i & 1) ? img_enemyShotMediumOval[8]
                : img_enemyShotMediumOval[1];

            pShot->param_d[0] = a;
            pShot->param_d[1] = r;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 中心を囲む小さな光の粒
        for (int i = 0; i < 10; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double a = DX_PI * 2.0 * (double)i / 10.0 + DX_PI / 20.0;
            double r = 58.0;

            pShot->x = pEnemyShotSet->x + cos(a) * r;
            pShot->y = pEnemyShotSet->y + sin(a) * r;
            pShot->muki = a;
            pShot->speed = 0.35;
            pShot->kind = img_enemyShotSmallBall[(i & 1) ? 8 : 1];
            pShot->param_d[0] = a;
            pShot->param_d[1] = r;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // ほぼ静止した太陽が、時間をかけてプレイヤーへ収束する。
        double a = pShot->param_d[0];
        double r0 = pShot->param_d[1];
        double r = r0 - pShot->count * 0.40;
        if (r < 3.0) r = 3.0;

        // 少しだけ回転させ、太陽光線のような動きを出す。
        a += pShot->count * 0.0025;
        pShot->x = pEnemyShotSet->x + cos(a) * r;
        pShot->y = pEnemyShotSet->y + sin(a) * r;

        pShot = pShot->next;
    }
}

// ============================================================
// 最終段階：北風が外へ、太陽が中央へ引っ張る巨大な渦
// ============================================================
static void ShotFinalVortex(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const int N = 28;

        for (int i = 0; i < N; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            double a = DX_PI * 2.0 * (double)i / (double)N;
            double r = 38.0 + (double)(i % 7) * 12.0;

            pShot->x = pEnemyShotSet->x + cos(a) * r;
            pShot->y = pEnemyShotSet->y + sin(a) * r;
            pShot->muki = a;
            pShot->speed = 0.0;

            // 北風と太陽を混ぜた配色
            if (i & 1)
                pShot->kind = img_enemyShotSmallBall[4];
            else
                pShot->kind = img_enemyShotMediumOval[(i & 2) ? 1 : 8];

            pShot->param_d[0] = a;
            pShot->param_d[1] = r;
            pShot->param_d[2] = (i & 1) ? 0.020 : -0.020;
            pShot->param_d[3] = (i & 1) ? 1.00 : -0.55;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double a = pShot->param_d[0] + pShot->count * pShot->param_d[2];
        double r0 = pShot->param_d[1];
        double radial = pShot->param_d[3];

        // 青弾は外へ吹き飛び、黄/橙弾は中心へ吸い込まれる。
        double r = r0 + radial * pShot->count;

        if (r < 8.0) r = 8.0;
        if (r > 235.0) r = 235.0;

        pShot->x = pEnemyShotSet->x + cos(a) * r;
        pShot->y = pEnemyShotSet->y + sin(a) * r;

        // 徐々に渦を強くする
        pShot->param_d[2] += (pShot->param_d[3] > 0.0 ? 0.00008 : -0.00008);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_NorthWindAndSun_ChatGPT()
{
    static int muki;

    if (count == 1) {
        // 画面サイズ 480x480
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.x2 = 240.0;
        enemy.y2 = 55.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 前半は左右へゆったり移動。太陽では中央へ戻る。
        if (count < 300) {
            enemy.x += 0.75 * (double)muki;
            enemy.x2 += 0.75 * (double)muki;
            if (count % 120 == 60) muki *= -1;
        }
        else if (count < 600) {
            enemy.x += (240.0 - enemy.x) * 0.02;
            enemy.x2 += (240.0 - enemy.x2) * 0.02;
        }
        else {
            enemy.x += sin(count * 0.01) * 0.25;
            enemy.x2 = enemy.x;
        }
    }

    // --------------------------------------------------------
    // 1段階目：北風（0～300F）
    // 15Fごとに左右交互の風を放つ
    // --------------------------------------------------------
    if (count < 300 && count % 15 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotNorthWind;

        bool left = ((count / 15) & 1) == 0;
        pSet->x = left ? enemy.x - 28.0 : enemy.x2 + 28.0;
        pSet->y = enemy.y + 12.0;
        pSet->muki = left ? 0.0 : DX_PI;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --------------------------------------------------------
    // 2段階目：太陽（300～600F）
    // 70Fごとに別の太陽輪を追加する
    // --------------------------------------------------------
    if (count >= 300 && count < 600 && count % 70 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSun;
        pSet->x = 240.0;
        pSet->y = 175.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --------------------------------------------------------
    // 3段階目：北風と太陽の融合（600F～）
    // 80Fごとに巨大な渦を追加
    // --------------------------------------------------------
    if (count >= 600 && count < 1080 && count % 80 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFinalVortex;
        pSet->x = 240.0;
        pSet->y = 220.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}