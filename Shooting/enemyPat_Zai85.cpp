// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕セット1：逆回転する超密リングA・B
// 2度間隔(180発×2=360発)で互いに1度ずらして配置することで、完全な弾の壁を形成します。
static void ShotRingA_B(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 180; i++) {
            // --- リングA (赤・菱形弾) ---
            pEnemyShot = new sEnemyShot;
            pEnemyShot->param_d[0] = pEnemyShotSet->x; // 中心X
            pEnemyShot->param_d[1] = pEnemyShotSet->y; // 中心Y
            pEnemyShot->param_d[2] = 320.0 - 10;           // 半径
            pEnemyShot->param_d[3] = (double)(i * 2);  // 初期角度(0, 2, 4...)
            pEnemyShot->param_d[4] = 0.05;             // 角速度(時計回り)
            pEnemyShot->kind = img_enemyShotDiamond[0]; // 赤
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // --- リングB (青・菱形弾) ---
            pEnemyShot = new sEnemyShot;
            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = 320.0 + 10;
            pEnemyShot->param_d[3] = (double)(i * 2) + 1.0; // 初期角度(1, 3, 5...) ※Aと1度ずらす
            pEnemyShot->param_d[4] = -0.05;                  // 角速度(反時計回り)
            pEnemyShot->kind = img_enemyShotDiamond[4];       // 青
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの回転処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->param_d[3] += pShot->param_d[4]; // 角度を更新
        double rad = pShot->param_d[3] * DX_PI / 180.0;
        pShot->x = pShot->param_d[0] + pShot->param_d[2] * cos(rad);
        pShot->y = pShot->param_d[1] + pShot->param_d[2] * sin(rad);
        pShot->muki = rad;

        pShot = pShot->next;
    }
}

// 弾幕セット2：高速回転する歯車リングC
// 2度間隔(180発)で、1箇所だけ2度分の欠損があります。
static void ShotRingC(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 180; i++) {
            // 初期角度0度〜1度の分だけ「欠損」させる（TAS用のすき間）
            if (i == 0) continue;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = 320.0;
            pEnemyShot->param_d[3] = (double)(i * 2); // 初期角度(4, 6, 8...)
            pEnemyShot->param_d[4] = 0.5;              // 角速度(超高速回転)
            pEnemyShot->kind = img_enemyShotDiamond[2]; // 緑
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->param_d[3] += pShot->param_d[4];
        double rad = pShot->param_d[3] * DX_PI / 180.0;
        pShot->x = pShot->param_d[0] + pShot->param_d[2] * cos(rad);
        pShot->y = pShot->param_d[1] + pShot->param_d[2] * sin(rad);
        pShot->muki = rad + DX_PI / 2;

        pShot = pShot->next;
    }
}

// 弾幕セット3：すき間を塞ぐ極太レーザー
static void ShotLaser(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki; // 敵本体で計算した角度
        pEnemyShot->speed = 15.0;               // 高速
        pEnemyShot->kind = img_enemyShotLaser[8]; // 橙色の短レーザー(64.0x4.0)

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}


// 敵本体のパターン
void EnemyPat_TAS_Zai()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // リングA・Bのセットを生成
        sEnemyShotSet* pSetAB = new sEnemyShotSet;
        pSetAB->count = 0;
        pSetAB->patternFunc = ShotRingA_B;
        pSetAB->x = enemy.x;
        pSetAB->y = enemy.y;
        pSetAB->pEnemyShotHead = new sEnemyShot;
        pSetAB->pEnemyShotHead->prev = pSetAB->pEnemyShotHead;
        pSetAB->pEnemyShotHead->next = pSetAB->pEnemyShotHead;

        pSetAB->prev = enemyShotSetHead.prev;
        pSetAB->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetAB;
        enemyShotSetHead.prev = pSetAB;

        // リングCのセットを生成
        sEnemyShotSet* pSetC = new sEnemyShotSet;
        pSetC->count = 0;
        pSetC->patternFunc = ShotRingC;
        pSetC->x = enemy.x;
        pSetC->y = enemy.y;
        pSetC->pEnemyShotHead = new sEnemyShot;
        pSetC->pEnemyShotHead->prev = pSetC->pEnemyShotHead;
        pSetC->pEnemyShotHead->next = pSetC->pEnemyShotHead;

        pSetC->prev = enemyShotSetHead.prev;
        pSetC->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetC;
        enemyShotSetHead.prev = pSetC;
    }
    else {
        // --- レーザーの発火タイミング計算 ---
        // リングCの欠損(0〜2度)が、真下(90度)に到達する瞬間を狙う。
        // 角度 = 0.5 * count。90度に到達するのは 0.5 * count = 90 → count = 180 (以降720フレーム周期)
        // レーザー(速度15.0)が敵(y=40)から円周(y=360)まで到達するのに約21フレームかかるため、その分だけ早く撃つ。
        int laserTiming = 180 - 21;

        if (count >= laserTiming && (count - laserTiming) % 720 == 0) {
            sEnemyShotSet* pSetL = new sEnemyShotSet;
            pSetL->count = 0;
            pSetL->patternFunc = ShotLaser;
            pSetL->x = enemy.x;
            pSetL->y = enemy.y;
            pSetL->muki = DX_PI / 2.0; // 真下
            pSetL->pEnemyShotHead = new sEnemyShot;
            pSetL->pEnemyShotHead->prev = pSetL->pEnemyShotHead;
            pSetL->pEnemyShotHead->next = pSetL->pEnemyShotHead;

            pSetL->prev = enemyShotSetHead.prev;
            pSetL->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSetL;
            enemyShotSetHead.prev = pSetL;
        }
    }
}