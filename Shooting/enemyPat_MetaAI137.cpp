#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ラプラスの檻 - Laplace's Cage -
// Phase1: 未確定弾の網を設置 / 近づくと確定弾に変化
// Phase2: 未来予測の青弾 + 網の収縮加速
// Phase3: 自分の過去の軌跡が弾源になる
// ============================================================

// 履歴と共有状態
static double g_histX[18000];
static double g_histY[18000];
static double g_playerVelX = 0.0;
static double g_playerVelY = 0.0;
static double g_netGlobalRot = 0.0;

// ------------------------------------------------------------
// 未確定弾 -> 確定弾 ネット
// pSet->param_d[0]=centerX, [1]=centerY, [2]=globalRot, [3]=shrinkSpeed
// pShot->param_i[0]=0:未確定/1:確定
// pShot->param_d[0]=base角度, [1]=半径, [2]=自転速度
// ------------------------------------------------------------
static void ShotLaplaceNet(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int num = 18; // 1セット18発
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->param_i[0] = 0; // 未確定
            pEnemyShot->param_d[0] = (2.0 * DX_PI / num) * i + GetRand(100) / 100.0 * 0.2;
            // GetRand注意: 0〜xの x+1種類
            pEnemyShot->param_d[1] = 70.0 + GetRand(180); // 70〜250
            pEnemyShot->param_d[2] = (GetRand(20) - 10) / 1000.0 + 0.005; // 回転速度

            double cx = pEnemyShotSet->param_d[0];
            double cy = pEnemyShotSet->param_d[1];
            double gr = pEnemyShotSet->param_d[2];
            pEnemyShot->x = cx + pEnemyShot->param_d[1] * cos(pEnemyShot->param_d[0] + gr);
            pEnemyShot->y = cy + pEnemyShot->param_d[1] * sin(pEnemyShot->param_d[0] + gr);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;

            // 素材選定: 未確定は半透明に見える白中玉、確定後は赤中玉
            pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全体回転を進める
    pEnemyShotSet->param_d[2] += 0.012;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) { // 未確定
            double dx = player.x - pShot->x;
            double dy = player.y - pShot->y;
            if (dx * dx + dy * dy < 38.0 * 38.0) {
                // 確定に変化
                pShot->param_i[0] = 1;
                pShot->muki = atan2(dy, dx);
                pShot->speed = 3.0 + GetRand(150) / 100.0; // 3.0〜4.5
                pShot->kind = img_enemyShotMediumBall[0]; // 赤
            }
            else {
                pShot->param_d[0] += pShot->param_d[2];
                pShot->param_d[1] -= pShot->param_d[3]; // 徐々に収縮
                if (pShot->param_d[1] < 20.0) pShot->param_d[1] = 20.0;
                pShot->x = pEnemyShotSet->param_d[0] + pShot->param_d[1] * cos(pShot->param_d[0] + pEnemyShotSet->param_d[2]);
                pShot->y = pEnemyShotSet->param_d[1] + pShot->param_d[1] * sin(pShot->param_d[0] + pEnemyShotSet->param_d[2]);
            }
        }
        else { // 確定後は反射して消えない
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->x < 6.0) { pShot->x = 6.0; pShot->muki = DX_PI - pShot->muki; }
            if (pShot->x > 474.0) { pShot->x = 474.0; pShot->muki = DX_PI - pShot->muki; }
            if (pShot->y < 6.0) { pShot->y = 6.0; pShot->muki = -pShot->muki; }
            if (pShot->y > 474.0) { pShot->y = 474.0; pShot->muki = -pShot->muki; }
            if (pShot->count < 30) pShot->speed += 0.02; // 確定直後の加速
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 未来予測弾: 0.8秒(48F)後の自機を狙う
// pSet->param_d[0]=velX, [1]=velY
// ------------------------------------------------------------
static void ShotPredictive(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        double predX = player.x + pEnemyShotSet->param_d[0] * 48.0;
        double predY = player.y + pEnemyShotSet->param_d[1] * 48.0;
        if (predX < 20.0) predX = 20.0;
        if (predX > 460.0) predX = 460.0;
        if (predY < 20.0) predY = 20.0;
        if (predY > 460.0) predY = 460.0;

        if (GetRand(3) == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        int n = 1 + GetRand(1); // 1〜2発
        for (int i = 0; i < n; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            double add = (GetRand(20) - 10) / 180.0 * DX_PI * 0.15;
            pEnemyShot->muki = atan2(predY - pEnemyShot->y, predX - pEnemyShot->x) + add;
            pEnemyShot->speed = 4.0 + GetRand(80) / 100.0;
            // 素材選定: 予測弾は視認しやすい青小玉
            pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 軌跡爆弾: 過去の自機位置から弾が発生
// pSet->param_i[0]=開始frame, [1]=終了frame
// ------------------------------------------------------------
static void ShotHistBomb(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int start = pEnemyShotSet->param_i[0];
        int end = pEnemyShotSet->param_i[1];
        if (start < 1) start = 1;
        if (end >= 18000) end = 17999;

        for (int idx = start; idx < end; idx += 12) { // 12F間隔でサンプリング
            double hx = g_histX[idx];
            double hy = g_histY[idx];
            if (hx == 0.0 && hy == 0.0) continue;
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = hx;
            pEnemyShot->y = hy;
            pEnemyShot->param_i[0] = 0; // 待機
            pEnemyShot->param_d[0] = 0.0; // タイマー
            pEnemyShot->muki = atan2(player.y - hy, player.x - hx) + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.0;
            // 素材選定: 軌跡上は橙小玉、発射後は赤小玉
            pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            pShot->param_d[0] += 1.0;
            if (pShot->param_d[0] > 20.0) {
                pShot->param_i[0] = 1;
                pShot->speed = 2.5 + GetRand(200) / 100.0;
                pShot->kind = img_enemyShotSmallBall[0]; // 赤に変化
            }
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            // 反射させて檻を維持
            if (pShot->x < 6.0 || pShot->x > 474.0) pShot->muki = DX_PI - pShot->muki;
            if (pShot->y < 6.0 || pShot->y > 474.0) pShot->muki = -pShot->muki;
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_TenYears_MetaAI()
{
    static int muki;
    static double playerPrevX, playerPrevY;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        playerPrevX = player.x;
        playerPrevY = player.y;
        g_netGlobalRot = 0.0;
        g_playerVelX = g_playerVelY = 0.0;
        for (int i = 0; i < 18000; i++) { g_histX[i] = 0.0; g_histY[i] = 0.0; }
    }
    else {
        // 履歴保存
        if (count < 18000) {
            g_histX[count] = player.x;
            g_histY[count] = player.y;
        }
        // 速度計算 (未来予測用)
        g_playerVelX = player.x - playerPrevX;
        g_playerVelY = player.y - playerPrevY;
        playerPrevX = player.x;
        playerPrevY = player.y;

        // 本体は微移動
        enemy.x += 0.35 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // Phase1: 網の設置 0〜7200F 90Fごとに18発
    if (count % 90 == 1 && count < 400) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotLaplaceNet;
        pSet->x = 0; pSet->y = 0;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->param_d[0] = 240.0; // centerX
        pSet->param_d[1] = 240.0; // centerY
        pSet->param_d[2] = g_netGlobalRot + GetRand(314) / 100.0;
        pSet->param_d[3] = 0.06 + (count / 400.0) * 0.14; // 収縮加速
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    g_netGlobalRot += 0.008;

    // Phase2: 予測弾 7200〜14400F
    if (count >= 400 && count < 800 && count % 14 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPredictive;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0.0;
        pSet->param_d[0] = g_playerVelX;
        pSet->param_d[1] = g_playerVelY;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // Phase3: 証明完了
    if (count == 800) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (count >= 800 && count % 90 == 0) {
        int start = count - 400;
        int end = count;
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHistBomb;
        pSet->param_i[0] = start;
        pSet->param_i[1] = end;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}