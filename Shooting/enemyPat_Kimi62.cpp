// enemyPat_apocalypticNest.cpp
// 弾幕：侵食多重螺旋「アポカリプティック・ネスト」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 層3用：プレイヤー位置履歴（30フレーム前 = 0.5秒前）
// ------------------------------------------------------------
static double player_hist_x[60];
static double player_hist_y[60];
static int    player_hist_idx = 0;

// ============================================================
// 層1+2：可変中心黄金螺旋＋螺旋上誘導子弾
// ============================================================
static void ShotSpiralCore(sEnemyShotSet* pSet)
{
    sEnemyShot* p;
    int i;

    // ---- 初期化 ----
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pSet->param_d[0] = pSet->x;   // 中心x（player.xを追従）
        pSet->param_d[1] = 240.0;     // 中心y（画面中央固定）
        pSet->param_i[0] = 0;         // 最後に反転したフェーズ番号

        // --- 層1：黄金螺旋弾 36発 ---
        for (i = 0; i < 36; i++) {
            p = new sEnemyShot;
            p->x = pSet->x;
            p->y = 280.0;
            p->muki = 0.0;
            p->speed = 0.0;

            p->kind = img_enemyShotScale[2 + (i % 2)];
            p->margin = 240;

            p->param_i[0] = 0;                    // 層1識別子
            double initAngle = i * (DX_PI / 18.0);
            p->param_d[0] = initAngle;            // 初期角度（参照用）
            p->param_d[1] = 0.020 + GetRand(15) / 1000.0 / 8; // 角速度
            p->param_d[2] = 0.8 + GetRand(5) / 10.0;      // 拡張速度
            p->param_d[5] = initAngle;            // 累積角度（毎フレーム加算）
            p->param_d[6] = 0.0;                  // 累積半径（毎フレーム加算）

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }

        // --- 層2：螺旋上誘導子弾 6発 ---
        for (i = 0; i < 6; i++) {
            p = new sEnemyShot;
            p->x = pSet->x;
            p->y = 240.0;
            p->muki = 0.0;
            p->speed = 0.0;

            p->kind = img_enemyShotDiamond[0]; // 赤
            p->margin = 240;

            p->param_i[0] = 1;                    // 層2識別子
            double initAngle = i * (DX_PI / 3.0);
            p->param_d[0] = initAngle;            // 初期角度
            p->param_d[1] = 0.040 + GetRand(20) / 1000.0 / 8; // 角速度
            p->param_d[2] = 35.0;                 // 基準半径
            p->param_d[3] = 0.5;                  // 半径増加速度
            p->param_d[4] = 0.0;                  // 誘導射撃カウンタ
            p->param_d[5] = initAngle;            // 累積角度
            p->param_d[6] = 0.0;                  // 累積半径増分

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // ---- 位相反転（層4効果）：180フレーム（3秒）周期 ----
    int phase = pSet->count / 180;
    if (phase != pSet->param_i[0]) {
        pSet->param_i[0] = phase;

        sEnemyShot* cur = pSet->pEnemyShotHead->next;
        while (cur != pSet->pEnemyShotHead) {
            if (cur->param_i[0] == 0) {
                // 層1：角速度・拡張速度を反転・加速
                cur->param_d[1] *= -1.0;
                cur->param_d[2] *= 1.3;
            }
            else if (cur->param_i[0] == 1) {
                // 層2：角速度・半径増加速度を反転・加速
                cur->param_d[1] *= -1.0;
                cur->param_d[3] *= 1.3;
            }
            else if (cur->param_i[0] == 2) {
                // 層2子弾：方向逆転＋速度1.3倍
                //cur->muki += DX_PI;
                //cur->speed *= 1.3;
            }
            cur = cur->next;
        }
    }

    // ---- 中心追従：プレイヤーx座標（可変中心）----
    double cx = player.x;
    double cy = 240.0;
    pSet->x = cx;

    // ---- 弾更新 ----
    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 0) {
            // 層1：対数螺旋（中心可変）
            double prevX = p->x;
            double prevY = p->y;

            p->param_d[5] += p->param_d[1]; // 累積角度を更新
            p->param_d[6] += p->param_d[2]; // 累積半径を更新

            double angle = p->param_d[5];
            double radius = p->param_d[6];
            p->x = cx + radius * cos(angle);
            p->y = cy + radius * sin(angle);

            // 進行方向をmukiに設定
            double dx = p->x - prevX;
            double dy = p->y - prevY;
            if (dx != 0.0 || dy != 0.0) {
                p->muki = atan2(dy, dx);
            }
        }
        else if (p->param_i[0] == 1) {
            // 層2：螺旋上を疾走（半径が時間とともに増加）
            double prevX = p->x;
            double prevY = p->y;

            p->param_d[5] += p->param_d[1]; // 累積角度を更新
            p->param_d[6] += p->param_d[3]; // 累積半径増分を更新

            double angle = p->param_d[5];
            double radius = p->param_d[2] + p->param_d[6];
            p->x = cx + radius * cos(angle);
            p->y = cy + radius * sin(angle);

            // 進行方向をmukiに設定
            double dx = p->x - prevX;
            double dy = p->y - prevY;
            if (dx != 0.0 || dy != 0.0) {
                p->muki = atan2(dy, dx);
            }

            // 誘導射撃：35フレームごとにプレイヤー方向へ小弾発射
            p->param_d[4] += 1.0;
            if ((int)p->param_d[4] % 70 == 0) {
                sEnemyShot* child = new sEnemyShot;
                child->x = p->x;
                child->y = p->y;
                child->muki = atan2(player.y - p->y, player.x - p->x);
                child->speed = 2.5;
                child->kind = img_enemyShotSmallBall[0];
                child->param_i[0] = 2; // 子弾識別子

                child->prev = pSet->pEnemyShotHead->prev;
                child->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = child;
                pSet->pEnemyShotHead->prev = child;
            }
        }
        else if (p->param_i[0] == 2) {
            // 層2子弾：直線（誘導弾）
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }

        p = p->next;
    }
}

// ============================================================
// 層3：境界からの侵食ライン
// ============================================================
static void ShotBorderErosion(sEnemyShotSet* pSet)
{
    sEnemyShot* p;
    int i;

    // ---- 初期化 ----
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double tx = pSet->param_d[0];
        double ty = pSet->param_d[1];

        pSet->param_i[0] = 0; // 最後に反転したフェーズ番号

        // 四辺から目標点へ向かう直線弾 8発
        for (i = 0; i < 8; i++) {
            int side = i / 2;
            double offset = (i % 2 == 0) ? -25.0 : 25.0;

            p = new sEnemyShot;

            if (side == 0) {        // 上辺
                p->x = tx + offset; p->y = -30.0;
            }
            else if (side == 1) {   // 右辺
                p->x = 510.0;       p->y = ty + offset;
            }
            else if (side == 2) {   // 下辺
                p->x = tx + offset; p->y = 510.0;
            }
            else {                  // 左辺
                p->x = -30.0;       p->y = ty + offset;
            }

            p->muki = atan2(ty - p->y, tx - p->x);
            p->speed = 3.0 + GetRand(3) / 10.0;
            p->kind = img_enemyShotLaser[6]; // 白レーザー
            p->margin = 40.0;                // レーザー用margin

            p->param_i[0] = 3;    // 層3識別子
            p->param_d[0] = tx;   // 目標x
            p->param_d[1] = ty;   // 目標y
            p->param_d[2] = 0.0;  // 爆発済みフラグ（0=未爆発）

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // ---- 位相反転（層4効果）：180フレーム周期 ----
    int phase = pSet->count / 180;
    if (phase != pSet->param_i[0]) {
        pSet->param_i[0] = phase;

        sEnemyShot* cur = pSet->pEnemyShotHead->next;
        while (cur != pSet->pEnemyShotHead) {
            if (cur->param_i[0] == 3) {
                // 侵食ライン：方向逆転＋速度1.3倍
                //cur->muki += DX_PI;
                //cur->speed *= 1.3;
            }
            else if (cur->param_i[0] == 4) {
                // 爆発弾も同様に反転・加速
                //cur->muki += DX_PI;
                //cur->speed *= 1.3;
            }
            cur = cur->next;
        }
    }

    // ---- 弾更新 ----
    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 3) {
            // 層3：直線侵食
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);

            // 目標点付近で爆発（交差地点で二次生成を模擬）
            double dx = p->x - p->param_d[0];
            double dy = p->y - p->param_d[1];
            double dist = sqrt(dx * dx + dy * dy);

            if (dist < 25.0 && p->param_d[2] == 0.0) {
                p->param_d[2] = 1.0; // 爆発済み

                // 爆発弾：8方向へ小弾を拡散
                for (i = 0; i < 8; i++) {
                    sEnemyShot* ex = new sEnemyShot;
                    ex->x = p->x;
                    ex->y = p->y;
                    ex->muki = i * (DX_PI / 4.0);
                    ex->speed = 1.5 + GetRand(5) / 10.0;
                    ex->kind = img_enemyShotSmallBall[6]; // 白
                    ex->param_i[0] = 4; // 爆発弾識別子

                    ex->prev = pSet->pEnemyShotHead->prev;
                    ex->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = ex;
                    pSet->pEnemyShotHead->prev = ex;
                }
            }
        }
        else if (p->param_i[0] == 4) {
            // 爆発弾：直線拡散
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }

        p = p->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_TooChaotic_Kimi()
{
    static int muki;
    static int shot_count;

    // ---- 初期化 ----
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;

        // 履歴バッファ初期化
        for (int i = 0; i < 60; i++) {
            player_hist_x[i] = player.x;
            player_hist_y[i] = player.y;
        }
        player_hist_idx = 0;
    }
    else {
        // 敵の左右移動（ゆるやかに）
        enemy.x += 1.0 * muki;
        if (enemy.x < 80.0 || enemy.x > 400.0) muki *= -1;
    }

    // ---- プレイヤー位置履歴更新（層3用：30フレーム前 = 0.5秒前）----
    player_hist_x[player_hist_idx] = player.x;
    player_hist_y[player_hist_idx] = player.y;
    int idx_30 = (player_hist_idx - 30 + 60) % 60;
    double target_x = player_hist_x[idx_30];
    double target_y = player_hist_y[idx_30];
    player_hist_idx = (player_hist_idx + 1) % 60;

    // ---- 層1+2：螺旋コア生成（90フレームごと）----
    if (count % 360 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSpiralCore;
        pSet->x = player.x; // 初期中心x（後追従）
        pSet->y = 240.0;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ---- 層3：境界侵食生成（60フレームごと = 1秒間隔）----
    if (count % 360 == 181) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBorderErosion;
        pSet->x = target_x;
        pSet->y = target_y;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;
        pSet->param_d[0] = target_x; // 30フレーム前のx
        pSet->param_d[1] = target_y; // 30フレーム前のy

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}