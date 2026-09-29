// ============================================================
//  ニュートンのゆりかご弾幕
//  ファイル名: enemyPat_Tmp.cpp
//  - 5つの大玉おもりを糸（小玉の点線）で支点から吊るす
//  - 両端のおもりが交互に振れ、中央で衝突して反対側が飛ぶ
//  - 衝突時に火花（高速小弾）を放射
//  - 素材: 大玉(img_enemyShotLargeBall) / 中玉(img_enemyShotMediumBall)
//          / 小玉(img_enemyShotSmallBall) のみで構成
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕：ニュートンのゆりかご
//   ・初回フレームで「支点」「糸」「おもり」を全生成
//   ・毎フレーム param_i を頼りに位置を更新
//   ・param_i[0] : 列番号(0-4) / それ以外は -1
//   ・param_i[1] : 役割 0=支点, 1=糸, 2=おもり, 9=火花
//   ・param_i[2] : 糸の点のインデックス
// ------------------------------------------------------------
static void ShotNewtonCradle(sEnemyShotSet* pEnemyShotSet)
{
    // ---- 定数 ----
    const int    N_COL = 5;                  // おもりの数
    const int    N_STR = 10;                 // 1本の糸に並べる小玉の数
    const double FULCRUM_Y = 80.0;               // 支点のY座標
    const double STRING_LEN = 150.0;              // 糸の長さ(px)
    const double AMP = DX_PI / 3.0;        // 最大振れ角 60度
    const int    PERIOD = 240;                // 往復1周期(フレーム)
    const double OMEGA = 2.0 * DX_PI / (double)PERIOD;
    const double BALL_R = 20.0;               // 大玉半径(密着配置用)
    const double SPACING = BALL_R * 2.0;       // 支点間隔 = 40
    const double CX = 240.0;              // 画面中央X

    const double fulcrum_x[N_COL] = {
        CX - SPACING * 2.0,   // 160
        CX - SPACING,         // 200
        CX,                   // 240
        CX + SPACING,         // 280
        CX + SPACING * 2.0    // 320
    };

    // ============================================================
    // 初回：支点・糸・おもりをすべて生成
    // ============================================================
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int c = 0; c < N_COL; c++) {
            // --- 支点（中玉・白）---
            {
                sEnemyShot* p = new sEnemyShot;
                p->x = fulcrum_x[c];
                p->y = FULCRUM_Y;
                p->muki = 0.0;
                p->speed = 0.0;
                p->kind = img_enemyShotMediumBall[6];  // 白
                p->param_i[0] = c;
                p->param_i[1] = 0;   // fulcrum
                p->param_i[2] = 0;
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
            // --- 糸（小玉・白の点線）---
            for (int s = 0; s < N_STR; s++) {
                sEnemyShot* p = new sEnemyShot;
                p->x = fulcrum_x[c];
                p->y = FULCRUM_Y;
                p->muki = 0.0;
                p->speed = 0.0;
                p->kind = img_enemyShotSmallBall[6];   // 白
                p->param_i[0] = c;
                p->param_i[1] = 1;   // string
                p->param_i[2] = s;
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
            // --- おもり（大玉・列ごとに色替え）---
            {
                sEnemyShot* p = new sEnemyShot;
                p->x = fulcrum_x[c];
                p->y = FULCRUM_Y + STRING_LEN;
                p->muki = 0.0;
                p->speed = 0.0;
                p->kind = img_enemyShotLargeBall[c];   // 0:赤 1:黄 2:緑 3:シアン 4:青
                p->param_i[0] = c;
                p->param_i[1] = 2;   // weight
                p->param_i[2] = 0;
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // ============================================================
    // 振れ角の計算（左右の端のみ振れ、中央3個は静止）
    //   区間 [0, T/4)   : 左端 -A → 0
    //   区間 [T/4, T/2) : 右端  0 → +A
    //   区間 [T/2, 3T/4): 右端 +A → 0
    //   区間 [3T/4, T)  : 左端  0 → -A
    // ============================================================
    double theta[N_COL] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
    {
        double t = (double)(pEnemyShotSet->count % PERIOD);
        double q = (double)PERIOD * 0.25;
        if (t < q) {
            theta[0] = -AMP * cos(OMEGA * t);
        }
        else if (t < q * 2.0) {
            theta[4] = AMP * sin(OMEGA * (t - q));
        }
        else if (t < q * 3.0) {
            theta[4] = AMP * cos(OMEGA * (t - q * 2.0));
        }
        else {
            theta[0] = -AMP * sin(OMEGA * (t - q * 3.0));
        }
    }

    // ============================================================
    // 各弾の位置更新
    // ============================================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int c = pShot->param_i[0];
        int role = pShot->param_i[1];
        int s = pShot->param_i[2];

        if (role == 0) {
            // 支点：固定
            pShot->x = fulcrum_x[c];
            pShot->y = FULCRUM_Y;
        }
        else if (role == 1) {
            // 糸：支点とおもりを結ぶ線分を等分した位置
            double th = theta[c];
            double wx = fulcrum_x[c] + STRING_LEN * sin(th);
            double wy = FULCRUM_Y + STRING_LEN * cos(th);
            double f = (double)(s + 1) / (double)(N_STR + 1);
            pShot->x = fulcrum_x[c] + (wx - fulcrum_x[c]) * f;
            pShot->y = FULCRUM_Y + (wy - FULCRUM_Y) * f;
        }
        else if (role == 2) {
            // おもり：支点を中心とした振り子位置
            double th = theta[c];
            pShot->x = fulcrum_x[c] + STRING_LEN * sin(th);
            pShot->y = FULCRUM_Y + STRING_LEN * cos(th);
        }
        else if (role == 9) {
            // 火花：等速直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // ============================================================
    // 衝突の瞬間（半周期ごと）に火花を放射
    //   半周期 = PERIOD/2 = 120、そのうち T/4 = 60 が衝突タイミング
    // ============================================================
    {
        int tc = pEnemyShotSet->count % (PERIOD / 2);
        if (tc == PERIOD / 4) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 12*5; i++) {
                sEnemyShot* p = new sEnemyShot;
                p->x = CX;
                p->y = FULCRUM_Y + STRING_LEN;
                // GetRand(360) は 0～360 を返す → 0～2π に正規化
                p->muki = (double)GetRand(360) / 180.0 * DX_PI;
                p->speed = 2.5 + (double)GetRand(150 + 500) / 100.0;
                p->kind = img_enemyShotSmallBall[1];   // 黄
                p->param_i[0] = -1;
                p->param_i[1] = 9;   // spark
                p->param_i[2] = 0;
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
//   ニュートンのゆりかご弾幕を1セットだけ生成して維持する
// ------------------------------------------------------------
void EnemyPat_NewtonsCradle_DeepSeek()
{
    static bool initialized = false;

    // ゲーム開始時の初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;  // 200から変更しないこと
        initialized = false;
    }

    // 弾幕セットを1度だけ生成（以降は永続）
    if (!initialized) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNewtonCradle;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        initialized = true;
    }
}