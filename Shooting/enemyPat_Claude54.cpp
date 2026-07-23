// enemyPat_beerKake.cpp
// パターン名：祝勝ビールかけ
//
// 「①ビールを注ぐ → ②泡が弾ける → ③祝杯（瓶）が宙を舞う」の
// 3フェーズで構成する弾幕。
//
// 使用素材:
//   img_enemyShotMediumBall[8] (橙) … 注ぎストリーム（ビール本体）
//   img_enemyShotSmallBall[6]  (白) … 泡リング／噴射しぶき
//   img_enemyShotLargeBall[8]  (橙) … 祝杯の瓶本体（大玉で存在感を出す）
//   sound_enemyShot_light … 注ぎ開始音
//   sound_enemyShot_heavy … 瓶を投げる音
//   ※ img_enemyShotLaser は当たり判定が大きすぎるため今回は不使用。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {
    // ---- 「注ぎ」関連パラメータ ----
    const int    POUR_COLUMN_NUM = 6;     // 同時に発生させる注ぎストリームの本数
    const double POUR_SPREAD = 200.0; // ストリーム群の左右への広がり幅
    const double POUR_SWAY_AMP = 18.0;  // 注ぎの左右への揺れ幅
    const double POUR_SWAY_OMEGA = 0.05;  // 揺れの角速度
    const double POUR_FALL_SPEED = 2.6;   // 落下速度
    const int    POUR_BURST_COUNT = 70;    // 何フレーム落下したら泡化するか

    // ---- 「泡」関連パラメータ ----
    const int    FOAM_RING_NUM = 12;    // 1回の泡化で発生させる泡弾の数
    const double FOAM_R0 = 4.0;   // 泡リングの初期半径
    const double FOAM_R_SPEED = 1.35;  // 泡リングが広がる速さ
    const double FOAM_RISE_SPEED = 0.35;  // 泡がゆっくり上に浮く速さ

    // ---- 「祝杯（瓶）」関連パラメータ ----
    const double BOTTLE_G = 0.045;        // 瓶にかかる重力加速度
    const double BOTTLE_VY = 5.6;          // 瓶の初速（上向き成分）
    const int    BOTTLE_SPRAY_NUM = 10;            // 頂点で撒き散らす泡しぶきの数
    const double BOTTLE_SPRAY_SPEED = 3.2;           // 泡しぶきの初速
    const double BOTTLE_SPRAY_SPREAD = DX_PI * 0.6;   // 泡しぶきの扇の開き角
}

// ============================================================
//  弾幕：注ぎ→泡化（ShotPour）
//  1つの sEnemyShotSet の中で「注ぎストリーム」と「泡リング」の
//  両方を param_i[0] のフェーズ値で切り替えて扱う。
//  位置はどちらのフェーズも pShot->count のみから決まる純関数。
// ============================================================
static void ShotPour(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < POUR_COLUMN_NUM; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            double baseX = pEnemyShotSet->x - POUR_SPREAD / 2.0
                + POUR_SPREAD * i / (double)(POUR_COLUMN_NUM - 1);
            // GetRand(628) は 0〜628 (629通り) を返すので /100.0 でおよそ 0〜2π の位相にする
            double phi = GetRand(628) / 100.0;

            pShot->x = baseX;
            pShot->y = pEnemyShotSet->y;
            pShot->kind = img_enemyShotMediumBall[8]; // 橙（ビール色）
            pShot->param_i[0] = 0; // 0:注ぎ中　1:泡化後
            pShot->param_d[0] = baseX;
            pShot->param_d[1] = phi;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 消去に備えて先に確保
        double t = (double)pShot->count;

        if (pShot->param_i[0] == 0) {
            // フェーズ0：注いでいる最中
            pShot->x = pShot->param_d[0] + POUR_SWAY_AMP * sin(POUR_SWAY_OMEGA * t + pShot->param_d[1]);
            pShot->y = pEnemyShotSet->y + POUR_FALL_SPEED * t;

            if (pShot->count >= POUR_BURST_COUNT) {
                // 泡化：現在位置を中心にリング状の泡弾を発生させる
                double burstX = pShot->x;
                double burstY = pShot->y;

                for (int k = 0; k < FOAM_RING_NUM; k++) {
                    sEnemyShot* pFoam = new sEnemyShot;
                    double angle = 2.0 * DX_PI * k / FOAM_RING_NUM;

                    pFoam->x = burstX;
                    pFoam->y = burstY;
                    pFoam->kind = img_enemyShotSmallBall[6]; // 白（泡）
                    pFoam->param_i[0] = 1; // 泡リング
                    pFoam->param_d[0] = angle;
                    pFoam->param_d[1] = burstX;
                    pFoam->param_d[2] = burstY;

                    pFoam->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pFoam->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pFoam;
                    pEnemyShotSet->pEnemyShotHead->prev = pFoam;
                }

                // 雫弾は泡に置き換わったので消去
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        else {
            // フェーズ1：泡リングが広がりながらゆっくり浮く
            double radius = FOAM_R0 + FOAM_R_SPEED * t;
            pShot->x = pShot->param_d[1] + radius * cos(pShot->param_d[0]);
            pShot->y = pShot->param_d[2] + radius * sin(pShot->param_d[0]) - FOAM_RISE_SPEED * t;
        }

        pShot = pNext;
    }
}

// ============================================================
//  弾幕：祝杯（瓶）の放物線＋頂点での噴射（ShotBottle）
//  瓶本体(param_i[0]==0)と噴射しぶき(param_i[0]==1)を区別する。
//  どちらも pShot->count のみから決まる閉形式の放物線（速度積分なし）。
// ============================================================
static void ShotBottle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = new sEnemyShot;

        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->kind = img_enemyShotLargeBall[8]; // 橙（瓶）
        pShot->param_i[0] = 0; // 0:瓶本体　1:噴射しぶき
        pShot->param_i[1] = 0; // 頂点での噴射をまだ行っていない
        pShot->param_d[0] = pEnemyShotSet->x;          // x0
        pShot->param_d[1] = pEnemyShotSet->y;          // y0
        pShot->param_d[2] = pEnemyShotSet->param_d[0]; // vx
        pShot->param_d[3] = pEnemyShotSet->param_d[1]; // vy
        pShot->margin = 240;

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        double t = (double)pShot->count;
        double x0 = pShot->param_d[0];
        double y0 = pShot->param_d[1];
        double vx = pShot->param_d[2];
        double vy = pShot->param_d[3];

        if (pShot->param_i[0] == 0) {
            // 瓶本体：放物線軌道
            pShot->x = x0 + vx * t;
            pShot->y = y0 - vy * t + 0.5 * BOTTLE_G * t * t;

            double tApex = vy / BOTTLE_G;
            if (pShot->param_i[1] == 0 && t >= tApex) {
                pShot->param_i[1] = 1; // 噴射は一度だけ

                for (int k = 0; k < BOTTLE_SPRAY_NUM; k++) {
                    sEnemyShot* pSpray = new sEnemyShot;
                    double ratio = (BOTTLE_SPRAY_NUM == 1) ? 0.5 : (double)k / (BOTTLE_SPRAY_NUM - 1);
                    double angle = DX_PI / 2.0 + (ratio - 0.5) * BOTTLE_SPRAY_SPREAD; // 下向き扇状

                    pSpray->x = pShot->x;
                    pSpray->y = pShot->y;
                    pSpray->kind = img_enemyShotDiamond[6]; // 白（泡しぶき）
                    pSpray->param_i[0] = 1; // 噴射しぶき
                    pSpray->param_d[0] = pShot->x;
                    pSpray->param_d[1] = pShot->y;
                    pSpray->param_d[2] = BOTTLE_SPRAY_SPEED * cos(angle);
                    pSpray->param_d[3] = BOTTLE_SPRAY_SPEED * sin(angle);
                    pSpray->muki = angle;
                    pSpray->margin = 240;

                    pSpray->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pSpray->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pSpray;
                    pEnemyShotSet->pEnemyShotHead->prev = pSpray;
                }
            }
        }
        else {
            // 噴射しぶき：緩やかな放物線
            pShot->x = x0 + vx * t;
            pShot->y = y0 + vy * t + 0.5 * BOTTLE_G * t * t;
        }

        pShot = pNext;
    }
}

// ============================================================
//  敵本体パターン：祝勝ビールかけ
// ============================================================
void EnemyPat_BeerSpray_Claude()
{
    static int bottleSide;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        bottleSide = 0;
    }
    else {
        // countのみから決まる純関数でゆったり左右にスイング
        enemy.x = 240.0 + 60.0 * sin(count / 110.0);
    }

    // 「注ぎ」：45フレームごとに新しいストリーム群を発生
    if (count % 45 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPour;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 「祝杯」：150フレームごとに瓶を画面の左右交互から放物線で投げる
    if (count % 90 == 45) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBottle;

        bottleSide = 1 - bottleSide; // 左右交互に切り替え
        double startX = (bottleSide == 0) ? 20.0 : 460.0;
        double targetX = (bottleSide == 0) ? 460.0 : 20.0;

        pEnemyShotSet->x = startX;
        pEnemyShotSet->y = 400;
        pEnemyShotSet->param_d[0] = (targetX - startX) / 180.0; // vx（約130フレームで横断）
        pEnemyShotSet->param_d[1] = BOTTLE_VY;                   // vy（初速上向き）

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}