#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数・ニューロン定義
// ============================================================
static const int NEURON_NUM = 6;          // ニューロン数（中央1 + 周囲5）
static const double NEURON_RADIUS = 110.0; // 周囲ニューロンの配置半径

// 各ニューロンの固定位置（画面中央寄り上部）
static double g_neuronX[NEURON_NUM];
static double g_neuronY[NEURON_NUM];

// 発火管理用
static int g_fireTimer[NEURON_NUM];       // 各ニューロンの次発火までの残りフレーム
static int g_lastFired = -1;              // 最後に発火したニューロン
static int g_chainStep = 0;               // 連鎖段階

// ============================================================
//  弾幕パターン1：ニューロン発火（放射弾）
// ============================================================
static void ShotNeuronFire(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 発火音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 12方向放射（中玉＋小玉を交互）
        for (int i = 0; i < 12; i++) {
            double ang = pEnemyShotSet->muki + (i * 30.0) / 180.0 * DX_PI;

            // 中玉（活動電位のコア）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 3.2 + (i % 3) * 0.15;
            pEnemyShot->kind = img_enemyShotMediumBall[(i % 2 == 0) ? 1 : 3]; // 黄 or シアン
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 小玉（外側のスパーク）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang + 0.08; // 少しずらす
            pEnemyShot->speed = 2.6 + (i % 2) * 0.2;
            pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕パターン2：シナプス信号（点と点を繋ぐ線）
// ============================================================
static void ShotSynapse(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // param_i[0] = 目標ニューロンindex
    // param_d[0] = 目標X, param_d[1] = 目標Y
    // param_i[1] = 残りの生成弾数

    if (pEnemyShotSet->count == 0) {
        // 軽い信号音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[1] = 14; // 生成する弾の数
    }

    // 数フレームかけて密集した弾列を生成（移動するシナプス線）
    if (pEnemyShotSet->param_i[1] > 0 && pEnemyShotSet->count % 2 == 0) {
        double dx = pEnemyShotSet->param_d[0] - pEnemyShotSet->x;
        double dy = pEnemyShotSet->param_d[1] - pEnemyShotSet->y;
        double dist = sqrt(dx * dx + dy * dy);
        double ang = atan2(dy, dx);

        // 小玉を密集配置
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = ang;
        pEnemyShot->speed = 4.8; // 信号は速め
        pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // たまにレーザーを混ぜて「太い接続」感を出す
        if (pEnemyShotSet->param_i[1] % 4 == 0) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 5.5;
            pEnemyShot->kind = img_enemyShotLaser[3]; // シアンレーザー
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        pEnemyShotSet->param_i[1]--;
    }

    // 弾移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  ユーティリティ：指定ニューロンを発火させる
// ============================================================
static void FireNeuron(int idx, int chainLevel)
{
    if (idx < 0 || idx >= NEURON_NUM) return;

    // --- 放射弾セット ---
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotNeuronFire;
    pSet->x = g_neuronX[idx];
    pSet->y = g_neuronY[idx];
    pSet->muki = 0.0; // 放射は内部で全方位
    pSet->kind = chainLevel;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    // --- シナプスを2〜3本伸ばす ---
    int targets = 2 + (chainLevel > 2 ? 1 : 0);
    for (int t = 0; t < targets; t++) {
        int targetIdx;
        do {
            targetIdx = GetRand(NEURON_NUM - 1);
        } while (targetIdx == idx);

        sEnemyShotSet* pSyn = new sEnemyShotSet;
        pSyn->count = 0;
        pSyn->patternFunc = ShotSynapse;
        pSyn->x = g_neuronX[idx];
        pSyn->y = g_neuronY[idx];
        pSyn->param_d[0] = g_neuronX[targetIdx];
        pSyn->param_d[1] = g_neuronY[targetIdx];
        pSyn->param_i[0] = targetIdx;
        pSyn->param_i[1] = 14;
        pSyn->pEnemyShotHead = new sEnemyShot;
        pSyn->pEnemyShotHead->prev = pSyn->pEnemyShotHead;
        pSyn->pEnemyShotHead->next = pSyn->pEnemyShotHead;
        pSyn->prev = enemyShotSetHead.prev;
        pSyn->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSyn;
        enemyShotSetHead.prev = pSyn;

        // 受け取った側を少し遅れて発火予約
        if (g_fireTimer[targetIdx] <= 0) {
            g_fireTimer[targetIdx] = 18 + GetRand(8); // 18〜26フレーム後
        }
    }

    g_lastFired = idx;
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_NeuralNetwork_Grok()
{
    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;

        // ニューロン配置（中央 + 五角形）
        g_neuronX[0] = 240.0;
        g_neuronY[0] = 160.0;
        for (int i = 1; i < NEURON_NUM; i++) {
            double ang = (i - 1) * 72.0 / 180.0 * DX_PI - DX_PI / 2.0;
            g_neuronX[i] = 240.0 + cos(ang) * NEURON_RADIUS;
            g_neuronY[i] = 160.0 + sin(ang) * NEURON_RADIUS;
        }

        // タイマー初期化
        for (int i = 0; i < NEURON_NUM; i++) {
            g_fireTimer[i] = 0;
        }
        g_lastFired = -1;
        g_chainStep = 0;

        // 最初の発火を予約
        g_fireTimer[0] = 30;
    }

    // 敵本体はゆっくり左右に揺れる程度
    enemy.x = 240.0 + sin(count * 0.02) * 40.0;

    // --- 発火タイマー処理 ---
    for (int i = 0; i < NEURON_NUM; i++) {
        if (g_fireTimer[i] > 0) {
            g_fireTimer[i]--;
            if (g_fireTimer[i] == 0) {
                FireNeuron(i, g_chainStep);
                g_chainStep++;
            }
        }
    }

    // 全体が静かになったら新しい連鎖を開始
    bool anyActive = false;
    for (int i = 0; i < NEURON_NUM; i++) {
        if (g_fireTimer[i] > 0) {
            anyActive = true;
            break;
        }
    }
    if (!anyActive && count > 60 && count % 90 == 0) {
        // ランダムなニューロンから新しい連鎖開始
        int start = GetRand(NEURON_NUM - 1);
        g_fireTimer[start] = 1;
        g_chainStep = 0;
    }

    // クライマックス：一定間隔で全発火
    if (count > 180 && count % 240 == 0) {
        for (int i = 0; i < NEURON_NUM; i++) {
            g_fireTimer[i] = 5 + i * 3; // 少しずらして全発火
        }
        g_chainStep = 5;
    }
}