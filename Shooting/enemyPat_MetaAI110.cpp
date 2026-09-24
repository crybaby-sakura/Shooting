// enemyPat_Tmp.cpp : 輪投げモチーフ弾幕「スリーピン・トス」
// ゲーム画面 480x480 前提

#include "DxLib.h"
#include "gv.h"
#include <cmath>
#ifndef DX_PI
#define DX_PI 3.14159265358979323846
#endif

extern int img_enemyShotSmallBall[9];
extern int img_enemyShotMediumBall[9];
extern int img_enemyShotLargeBall[9];
extern int img_enemyShotBullet[9];
extern int img_enemyShotScale[9];
extern int img_enemyShotDiamond[9];
extern int img_enemyShotMediumOval[9];
extern int img_enemyShotLaser[9];

extern int sound_enemyShot_light;
extern int sound_enemyShot_medium;
extern int sound_enemyShot_heavy;

//----------------------------------------
// 杭 3本を設置するセット
// pEnemyShotSet->param_d[0..2] = targetX[3]
// pEnemyShotSet->param_d[3..5] = targetY[3]
//----------------------------------------
static void ShotStake(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->param_d[i];
            pShot->y = pEnemyShotSet->param_d[i + 3];
            pShot->muki = DX_PI / 2.0; // 縦向きに見えるように
            pShot->speed = 0.0;
            pShot->count = 0;
            pShot->kind = img_enemyShotMediumOval[2]; // 緑の中楕円 = 杭
            pShot->margin = 60.0;
            pShot->param_i[0] = 0; // 0:停止中 1:ホーミング開始済み
            pShot->param_d[0] = 0;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 180F 停止後、プレイヤーに向かってゆっくりホーミング(投げ失敗の腹いせ)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->count > 280 && pShot->param_i[0] == 0) {
            pShot->param_i[0] = 1;
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 1.2 + (double)GetRand(80) / 100.0; // 1.2～2.0
        }
        if (pShot->param_i[0] == 1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

//----------------------------------------
// 輪 1つ = 小玉16発の円
// pEnemyShotSet->param_d[0] = centerX
// pEnemyShotSet->param_d[1] = centerY
// pEnemyShotSet->param_d[2] = targetX (杭の位置)
// pEnemyShotSet->param_d[3] = targetY
// pEnemyShotSet->param_d[4] = rotation
// pEnemyShotSet->param_d[5] = radius
// pEnemyShotSet->param_d[7] = vx
// pEnemyShotSet->param_d[8] = vy
// param_i[0] = 0:移動中 1:入った 2:落下中
//----------------------------------------
static void ShotRing(sEnemyShotSet* pEnemyShotSet)
{
    const int RING_NUM = 16;
    const double RADIUS = 38.0*1.5;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 中心と速度を初期化
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
        pEnemyShotSet->param_d[4] = 0.0;
        pEnemyShotSet->param_d[5] = RADIUS;
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0; // 入ってからの経過

        double dx = pEnemyShotSet->param_d[2] - pEnemyShotSet->param_d[0];
        double dy = pEnemyShotSet->param_d[3] - pEnemyShotSet->param_d[1];
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < 1.0) dist = 1.0;
        double sp = 2.4;
        pEnemyShotSet->param_d[7] = dx / dist * sp;
        pEnemyShotSet->param_d[8] = dy / dist * sp;

        int col = pEnemyShotSet->param_i[5]; // 0..8
        if (col < 0 || col > 8) col = 0;

        for (int i = 0; i < RING_NUM; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double ang = 2.0 * DX_PI * i / RING_NUM;
            pShot->x = pEnemyShotSet->param_d[0] + RADIUS * cos(ang);
            pShot->y = pEnemyShotSet->param_d[1] + RADIUS * sin(ang);
            pShot->muki = ang;
            pShot->speed = 0.0;
            pShot->count = 0;
            pShot->kind = img_enemyShotSmallBall[col]; // 小玉で輪を構成
            pShot->margin = 125.0;
            pShot->param_i[0] = i; // 円周上のインデックス

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 中心移動
    if (pEnemyShotSet->param_i[0] == 0) {
        pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[7];
        pEnemyShotSet->param_d[1] += pEnemyShotSet->param_d[8];
        pEnemyShotSet->param_d[4] += 0.08; // 回転

        double dx = pEnemyShotSet->param_d[2] - pEnemyShotSet->param_d[0];
        double dy = pEnemyShotSet->param_d[3] - pEnemyShotSet->param_d[1];
        if (dx * dx + dy * dy < 25.0) { // 杭に到達
            pEnemyShotSet->param_i[0] = 1;
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }
    else if (pEnemyShotSet->param_i[0] == 1) {
        // 入った状態で少し留まる
        pEnemyShotSet->param_d[4] += 0.02;
        pEnemyShotSet->param_i[1]++;
        if (pEnemyShotSet->param_i[1] > 55) {
            pEnemyShotSet->param_i[0] = 2; // 落下フェーズへ
            pEnemyShotSet->param_d[7] = 0.0;
            pEnemyShotSet->param_d[8] = 0.9;
        }
    }
    else {
        // 失敗/成功後に落下
        pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[7];
        pEnemyShotSet->param_d[1] += pEnemyShotSet->param_d[8];
        pEnemyShotSet->param_d[4] += 0.06;
    }

    // 各小玉を中心+回転で再配置
    double cx = pEnemyShotSet->param_d[0];
    double cy = pEnemyShotSet->param_d[1];
    double rot = pEnemyShotSet->param_d[4];
    double rad = pEnemyShotSet->param_d[5];

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int idx = pShot->param_i[0];
        double baseAng = 2.0 * DX_PI * idx / RING_NUM;
        double ang = baseAng + rot;
        pShot->x = cx + rad * cos(ang);
        pShot->y = cy + rad * sin(ang);
        pShot = pShot->next;
    }
}

//----------------------------------------
// 敵本体
//----------------------------------------
void EnemyPat_RingToss_MetaAI()
{
    static int muki;
    static double lastStakeX[3];
    static double lastStakeY[3];

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        for (int i = 0; i < 3; i++) {
            lastStakeX[i] = 120.0 + i * 120.0;
            lastStakeY[i] = 340.0;
        }
    }
    else {
        enemy.x += 0.5 * (double)muki;
        if (enemy.x < 80) muki = 1;
        if (enemy.x > 400) muki = -1;
    }

    // 360Fサイクル
    // 1F:杭設置 60F,120F,180F:輪投げ
    int cycle = count % 180;

    if (cycle == 1) {
        // 杭の位置を決定 GetRand(x)は0～xを返すので注意
        // 120,240,360付近にランダムでばらけさせる
        for (int i = 0; i < 3; i++) {
            lastStakeX[i] = (120.0 + i * 120.0) + (double)(GetRand(60) - 30);
            lastStakeY[i] = 340.0 + (double)(GetRand(60) - 30); // 310～370
            if (lastStakeX[i] < 40) lastStakeX[i] = 40;
            if (lastStakeX[i] > 440) lastStakeX[i] = 440;
            if (lastStakeY[i] < 280) lastStakeY[i] = 280;
            if (lastStakeY[i] > 400) lastStakeY[i] = 400;
        }

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotStake;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0;
        pSet->kind = 0;
        for (int i = 0; i < 3; i++) {
            pSet->param_d[i] = lastStakeX[i];
            pSet->param_d[i + 3] = lastStakeY[i];
        }
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else if (cycle == 60 || cycle == 100 || cycle == 140) {
        int idx = (cycle - 60) / 40; // 0,1,2
        int colors[3] = { 0, 1, 4 }; // 赤、黄、青

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotRing;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0;
        pSet->kind = idx;
        pSet->param_d[2] = lastStakeX[idx];
        pSet->param_d[3] = lastStakeY[idx];
        pSet->param_i[5] = colors[idx];

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}