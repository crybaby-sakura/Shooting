// enemyPat_Tmp.cpp
// 弾幕:バックドラフト
//
//  [Phase 0 燻り]   : 画面全域に停滞弾(暗い赤)をばら撒く
//  [Phase 1 酸素流入]: 画面外周から加速弾(シアン)が風となって吹き込み、
//                      停滞弾がボス方向へ引き寄せられ、橙に点滅して着火を予告する
//  [Phase 2 引火]   : ボスからレーザーを一斉照射。停滞弾は時間差で
//                      炎色(橙/黄)の高速弾となり、自機方向へ飛来する
//
//  1サイクル終了後、少し間を空けて次のサイクルへ。
//  サイクルを重ねるごとに停滞弾が増加し、炎弾も高速化していく。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const int SMOLDER_END = 240;   // 燻り終了フレーム
static const int INFLOW_END = 360;   // 酸素流入終了=引火開始フレーム
static const int IGNITE_DELAY = 18;   // 引火の時間差(最大フレーム数)
static const int CYCLE_FRAMES = 510-40;  // 1サイクルの長さ(引火後の余裕を含む)

// 弾の役割(param_i[0] に格納)
static const int ROLE_SMOLDER = 0;  // 停滞弾(燻り)
static const int ROLE_WIND = 1;  // 風(酸素流入)弾
static const int ROLE_LASER = 2;  // 引火用レーザー
static const int ROLE_FLAME = 3;  // 引火後の炎弾

// 色インデックス(画像配列の添字 = 色)
static const int COLOR_RED = 0;
static const int COLOR_YELLOW = 1;
static const int COLOR_CYAN = 3;
static const int COLOR_ORANGE = 8;

// ============================================================
//  弾の生成ヘルパー
//  ballType: 0=小玉 1=中玉 2=大玉 3=レーザー
// ============================================================
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int color, int ballType)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->margin = 120;
    switch (ballType) {
    case 0:  pShot->kind = img_enemyShotSmallBall[color];  break;
    case 1:  pShot->kind = img_enemyShotMediumBall[color]; break;
    case 2:  pShot->kind = img_enemyShotLargeBall[color];  break;
    default: pShot->kind = img_enemyShotLaser[color];      break;
    }
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ============================================================
//  バックドラフト弾幕本体
//  pSet->kind = サイクル数(難易度)
// ============================================================
static void ShotBackdraft(sEnemyShotSet* pSet)
{
    int loop = pSet->kind; // 何周目か
    int c = pSet->count;

    // 効果音(燻り開始)
    if (c == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // ---- Phase 0:燻り(停滞弾を画面全域にばら撒く) ----
    if (c < SMOLDER_END && c % 6 == 0) {
        int tick = c / 6; // 0..39
        // 基本の停滞弾
        double sx = 0.0, sy = 0.0;
        while (true) {
            sx = 20.0-40 + GetRand(440+80);
            sy = 60.0-80 + GetRand(380+160);
            if (hypot(sx - player.x, sy - player.y) > 30) break;
        }
        double m = GetRand(360) / 180.0 * DX_PI;
        sEnemyShot* pShot = AddShot(pSet, sx, sy, m, 0.25, COLOR_RED, 0);
        pShot->param_i[0] = ROLE_SMOLDER;
        pShot->param_i[2] = GetRand(9); // 点滅の位相ずらし用
        // ループが進むごとに停滞弾が増える(+5発/周、最大+25発)
        int extra = loop;
        if (extra > 5) extra = 5;
        for (int i = 0; i < extra; i++) {
            if (tick % 8 == 7 - i) {
                double sx2 = 0.0, sy2 = 0.0;
                while (true) {
                    sx2 = 20.0 - 40 + GetRand(440 + 80);
                    sy2 = 60.0 - 80 + GetRand(380 + 160);
                    if (hypot(sx2 - player.x, sy2 - player.y) > 30) break;
                }
                double m2 = GetRand(360) / 180.0 * DX_PI;
                sEnemyShot* pShot2 = AddShot(pSet, sx2, sy2, m2, 0.25, COLOR_RED, 0);
                pShot2->param_i[0] = ROLE_SMOLDER;
                pShot2->param_i[2] = GetRand(9);
            }
        }
    }

    // ---- Phase 1:酸素流入(画面外周から風が吹き込む) ----
    if (c >= SMOLDER_END && c < INFLOW_END && c % 15 == 0) {
        if (c == SMOLDER_END) PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 予告音
        for (int i = 0; i < 6; i++) {
            double sx, sy;
            switch (GetRand(2)) { // 0..2 の3択
            case 0:  sx = GetRand(480) * 1.0; sy = -10.0;  break; // 上から
            case 1:  sx = -10.0;  sy = GetRand(480) * 1.0; break; // 左から
            default: sx = 490.0;  sy = GetRand(480) * 1.0; break; // 右から
            }
            double m = atan2(enemy.y - sy, enemy.x - sx);
            sEnemyShot* pShot = AddShot(pSet, sx, sy, m, 1.0, COLOR_CYAN, 0);
            pShot->param_i[0] = ROLE_WIND;
        }
    }

    // ---- Phase 2:引火(レーザー照射) ----
    if (c == INFLOW_END) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 3; i++) {
            double m = atan2(player.y - enemy.y, player.x - enemy.x);
            sEnemyShot* pShot = AddShot(pSet, enemy.x, enemy.y, m, 14.0, COLOR_ORANGE, 3);
            pShot->param_i[0] = ROLE_LASER;
        }
    }

    // ---- 各弾の更新 ----
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case ROLE_SMOLDER:
            if (c < SMOLDER_END) {
                // 燻り:ほぼ停止して漂う
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (c < INFLOW_END) {
                // 酸素流入:ボス方向へゆっくり引き寄せられ、橙に点滅(着火予告)
                double m = atan2(enemy.y - pShot->y, enemy.x - pShot->x);
                pShot->x += 0.5 * cos(m);
                pShot->y += 0.5 * sin(m);
                int blink = ((c / 8) + pShot->param_i[2]) % 2;
                pShot->kind = img_enemyShotSmallBall[blink ? COLOR_ORANGE : COLOR_RED];
            }
            else {
                // 引火待ち:時間差で発火(炎が燃え広がるように見せる)
                if (pShot->param_i[1] == 0) {
                    pShot->param_i[1] = c + GetRand(IGNITE_DELAY);
                    pShot->kind = img_enemyShotSmallBall[COLOR_ORANGE];
                }
                if (c >= pShot->param_i[1]) {
                    // 炎弾化:自機方向へ急加速
                    pShot->param_i[0] = ROLE_FLAME;
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->speed = 4.0 + GetRand(100) / 100.0 + loop * 0.15;
                    if (pShot->speed > 6.0) pShot->speed = 6.0;
                    int col = (pShot->param_i[2] % 2) ? COLOR_ORANGE : COLOR_YELLOW;
                    pShot->kind = img_enemyShotMediumBall[col]; // 中玉に変えて視認性アップ
                }
            }
            break;

        case ROLE_WIND:
            // 風:ボス目掛けて加速していく
            pShot->speed += 0.06;
            if (pShot->speed > 6.0) pShot->speed = 6.0;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;

        case ROLE_LASER:
        case ROLE_FLAME:
            // 直進するだけ(画面外に出たらメインルーチンが消去)
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Backdraft_Zai()
{
    static int muki;
    static int cycle;   // サイクル内の経過フレーム
    static int loopNum; // サイクル数(難易度)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        cycle = 0;
        loopNum = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // サイクル管理:周期ごとに新しいバックドラフトを開始する
    if (cycle == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBackdraft;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = loopNum++; // サイクル数=難易度を記録
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    cycle++;
    if (cycle >= CYCLE_FRAMES) cycle = 0;
}