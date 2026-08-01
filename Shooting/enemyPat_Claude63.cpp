// enemyPat_sanshokuKaitentou.cpp
//
// 三色回転塔（サインポール／理容店の看板ポールがモチーフ）
//
// フェーズ構成:
//   フェーズ1 建立        : 中心の柱の輪郭（左右2本の縁）と上下の飾り玉が形成される
//   フェーズ2 螺旋点灯    : 赤・白・青の帯が下端で生まれて上昇し、隙間の位置が層ごとに
//                           cos波でスライドすることで「回転しながら昇っていく」錯視を作る
//   フェーズ3 回転加速    : 発生間隔短縮・振れ幅拡大・上昇速度上昇で難度を引き上げる
//   フェーズ4 頂天解放    : 上端の飾り玉が全方位へリングバースト、
//                           下端の飾り玉からプレイヤー狙いの3way弾が放たれる
//
// 色の役割:
//   赤 = 標準の帯（動脈）
//   白 = 隙間が広めの安全帯（包帯）
//   青 = 上昇が速い帯（静脈）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ===== 画面・塔の基本パラメータ =====
static const double CENTER_X = 240.0;
static const double TOP_Y = 10.0;
static const double BOTTOM_Y = 470.0;
static const double POLE_R = 100.0; // 塔の見かけの半径

// ===== フェーズ境界（フレーム数） =====
static const int PHASE1_END = 140; // 建立
static const int PHASE2_END = 500; // 螺旋点灯
static const int PHASE3_END = 680; // 回転加速
// フェーズ4はPHASE3_END以降。新規発生は行わず、既存弾が自然に画面外へ抜けて終了する。

// ===== 色 =====
// 色一覧: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
static const int COL_RED = 0;
static const int COL_WHITE = 6;
static const int COL_BLUE = 4;
static const int SPIRAL_COLOR_CYCLE[3] = { COL_RED, COL_WHITE, COL_BLUE };

static const int T = 750;

// ------------------------------------------------------------
// 塔の縁（左右2本の柱）
// フェーズ1で下から上へ積み上がり、フェーズ3終了まで静止して塔の幅を示す。
// フェーズ4開始で画面外へ退避しメインルーチンに削除させる。
// ------------------------------------------------------------
static void ShotFrame(sEnemyShotSet* pEnemyShotSet)
{
    if (count % T <= PHASE1_END && (count % T - 1) % 3 == 0) {
        int seg = (count % T - 1) / 3;
        double y = BOTTOM_Y - seg * 10.0;
        if (y >= TOP_Y) {
            for (int side = 0; side < 2; side++) {
                sEnemyShot* pShot = new sEnemyShot;

                // 位置は固定（静止した縁）。formula-driven方針のためparam_dに焼き込む。
                pShot->param_d[0] = CENTER_X + (side == 0 ? -POLE_R : POLE_R);
                pShot->param_d[1] = y;
                pShot->muki = 0.0;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotSmallBall[COL_WHITE];
               
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (count % T < PHASE3_END) {
            pShot->x = pShot->param_d[0];
            pShot->y = pShot->param_d[1];
        }
        else if (count % T == PHASE3_END) {
            pShot->muki = atan2(pShot->y - 240.0, pShot->x - 240.0);
            pShot->speed = 3.0 + GetRand(300) / 100.0;
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 上部飾り玉（塔の頂上のボール部分）
// フェーズ1で半径0から広がってリング状に形成され、フェーズ3終了まで維持。
// フェーズ4で全方位リングバーストへ移行する。
// ------------------------------------------------------------
static void ShotFinialTop(sEnemyShotSet* pEnemyShotSet)
{
    const int N = 12;
    const double HOLD_R = 18.0;

    if (count % T == 1) {
        for (int i = 0; i < N; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = DX_PI * 2.0 * i / N;

            pShot->param_d[0] = angle;
            pShot->muki = angle;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotMediumBall[COL_WHITE];
            
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double angle = pShot->param_d[0];

        if (count % T <= PHASE3_END) {
            // 建立フェーズで半径が0からHOLD_Rまで伸び、以後は維持される
            double r = HOLD_R * (double)pShot->count / (double)PHASE1_END;
            if (r > HOLD_R) r = HOLD_R;
            pShot->x = enemy.x + r * cos(angle);
            pShot->y = enemy.y + r * sin(angle);
        }
        else {
            // フェーズ4：全方位へリングバースト
            double t = (double)(count % T - PHASE3_END);
            double r = HOLD_R + 6.0 * t;
            pShot->x = enemy.x + r * cos(angle);
            pShot->y = enemy.y + r * sin(angle);
            pShot->kind = img_enemyShotLargeBall[6];
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 下部飾り玉（塔の土台のボール部分）
// フェーズ1〜3は上部と同様に静止したリングを維持するだけの土台。
// フェーズ4突入の瞬間、プレイヤー狙いの3way弾を新規発射し、
// 元のリングは静かに退避させる。
// ------------------------------------------------------------
static void ShotFinialBottom(sEnemyShotSet* pEnemyShotSet)
{
    const int N = 12;
    const double HOLD_R = 18.0;

    if (count % T == 1) {
        for (int i = 0; i < N; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = DX_PI * 2.0 * i / N;

            pShot->param_i[0] = 0; // 0:土台のリング弾
            pShot->param_d[0] = angle;
            pShot->muki = angle;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotMediumBall[COL_WHITE];
           
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // フェーズ4突入の瞬間、プレイヤー位置を狙った3way弾を追加発射
    if (count % T == PHASE3_END + 1) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - BOTTOM_Y, player.x - CENTER_X);
        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->param_i[0] = 1; // 1:狙い撃ちの直進弾
            pShot->param_d[0] = CENTER_X;
            pShot->param_d[1] = BOTTOM_Y;
            pShot->param_d[2] = 3.2; // 速さ
            pShot->muki = baseAngle + i * (15.0 * DX_PI / 180.0);
            pShot->speed = 3.2;
            pShot->kind = img_enemyShotBullet[COL_RED];
          
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 狙い撃ちの直進弾（countに基づく式で位置計算、速度積分は行わない）
            pShot->x = pShot->param_d[0] + pShot->param_d[2] * cos(pShot->muki) * pShot->count;
            pShot->y = pShot->param_d[1] + pShot->param_d[2] * sin(pShot->muki) * pShot->count;
        }
        else {
            double angle = pShot->param_d[0];
            if (count % T <= PHASE3_END) {
                double r = HOLD_R * (double)pShot->count / (double)PHASE1_END;
                if (r > HOLD_R) r = HOLD_R;
                pShot->x = CENTER_X + r * cos(angle);
                pShot->y = BOTTOM_Y + r * sin(angle);
            }
            else {
                pShot->y = 9999.0; // フェーズ4で退避、画面外判定により自動削除される
            }
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 螺旋の帯（メインの三色縞）
// 下端で横一列の帯が生まれ、隙間を残したまま上昇していく。
// 隙間の中心位置は層ごとにcos波でスライドし、
// 「回転しながら昇っていく」錯視を作る。
// フェーズ3では発生間隔短縮・振れ幅拡大・上昇速度上昇で難度を上げる。
// ------------------------------------------------------------
static void ShotSpiral(sEnemyShotSet* pEnemyShotSet)
{
    if (count % T > PHASE1_END && count % T <= PHASE3_END) {
        bool isAccel = (count % T > PHASE2_END);
        int interval = isAccel ? 4 : 8;
        interval *= 5;

        if ((count % T - PHASE1_END - 1) % interval == 0) {
            int layerIndex = pEnemyShotSet->param_i[0]++;

            double angleStep = (isAccel ? 40.0 : 25.0) * DX_PI / 180.0;
            double gapAngle = layerIndex * angleStep;
            double gapCenterX = CENTER_X + POLE_R * cos(gapAngle) * 0.9;

            int color = SPIRAL_COLOR_CYCLE[layerIndex % 3];
            double gapHalfWidth = (color == COL_WHITE) ? 22.0 : 13.0;

            double riseSpeed = 1.6 * (isAccel ? 1.4 : 1.0) * (color == COL_BLUE ? 1.3 : 1.0);

            // 弾の種類一覧より、帯の質感を出すため鱗弾(4.0x3.0)を使用
            const int N_SLOTS = 20;
            for (int s = 0; s < N_SLOTS; s++) {
                double slotX = CENTER_X - POLE_R + s * (2.0 * POLE_R / (N_SLOTS - 1));
                if (fabs(slotX - gapCenterX) < gapHalfWidth) continue; // ここが隙間

                sEnemyShot* pShot = new sEnemyShot;
                pShot->param_d[0] = slotX;
                pShot->param_d[1] = BOTTOM_Y;
                pShot->param_d[2] = riseSpeed;
                pShot->muki = -DX_PI / 2.0; // 見た目上の向きは「上」
                pShot->speed = riseSpeed;
                pShot->kind = img_enemyShotScale[color];
               
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0];
        pShot->y = pShot->param_d[1] - pShot->param_d[2] * pShot->count;
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 共通のセット生成処理（4本の弾セットで使い回す）
// ------------------------------------------------------------
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->alive = 600;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    return pEnemyShotSet;
}

// ------------------------------------------------------------
// 敵本体のパターン：三色回転塔
// ------------------------------------------------------------
void EnemyPat_SignPole_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = CENTER_X;
        enemy.y = TOP_Y;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }

    if (count % T == 1) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        CreateShotSet(ShotFrame, CENTER_X, TOP_Y);
        CreateShotSet(ShotFinialTop, CENTER_X, TOP_Y);
        CreateShotSet(ShotFinialBottom, CENTER_X, BOTTOM_Y);
        CreateShotSet(ShotSpiral, CENTER_X, BOTTOM_Y);
    }
}