#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <cmath>

// 参考 URL: https://www.desmos.com/calculator/i05puaquwh?lang=ja
static double calculateX(double y, double t) {
    const double sin_t = std::sin(t);

    // 第1項
    double term1 = (1.5 * std::exp((0.12 * sin_t - 0.5) * std::pow(y - 0.16 * sin_t, 2)))
        / (1.0 + std::exp(20.0 * (5.0 * y - sin_t)));

    // 第2項
    double term2 = ((1.5 - 0.8 * std::pow(y - 0.2 * sin_t, 3)) / (1.0 + std::exp(20.0 * (sin_t - 5.0 * y))))
        / (1.0 + std::exp(100.0 * (y - 1.0) - 16.0 * sin_t));

    // 第3項
    double term3 = (0.2 * (std::exp(-std::pow(y - 1.0, 2)) + 1.0))
        / (1.0 + std::exp(100.0 * (1.0 - y) + 16.0 * sin_t));

    // 第4項
    double term4 = 0.1 / std::exp(2.0 * std::pow(10.0 * y - 1.2 * (2.0 + sin_t) * sin_t, 4));

    return term1 + term2 + term3 + term4;
}

static void ShotOppai(sEnemyShotSet* pEnemyShotSet) {
    double t = pEnemyShotSet->count / 30.0;

    // 【1】弾の初期化（リストに登録するだけ）
    if (pEnemyShotSet->count == 0) {
        // カーブが伸びても足りるように、弾の数を多めに確保します（例: 300個）
        for (int i = 0; i < 300; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->kind = img_enemyShotSmallBall[8]; // 弾の種類
            pEnemyShot->margin = 100;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    // 【2】毎フレーム、曲線の長さを測りながら等間隔に配置する

    // 数式の入力となる変数の開始位置（元の 0 ピクセル位置に相当）
    double math_val = -2.4;

    // 最初の弾の座標
    double current_y = math_val * 100.0 + 240.0;
    double current_x = calculateX(math_val, t) * 100.0;

    // --- 調整パラメータ ---
    const double TARGET_DISTANCE = 6.0; // 弾と弾の間隔（ピクセル）。好みに合わせて調整してください。
    const double STEP = 0.0005;          // 座標を探索する際の刻み幅。小さいほど等間隔の精度が上がります。
    // ----------------------

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 現在の座標を弾に適用
        pShot->x = current_x;
        pShot->y = current_y;

        // 次の弾の座標を探索するための変数
        double next_x = current_x;
        double next_y = current_y;
        double dist = 0.0;

        // 次の弾が TARGET_DISTANCE ピクセル以上離れるまで、数式の入力値を少しずつ進める
        // （math_val <= 3.0 は描画終了位置の目安です。元の 480 ピクセル位置なら 2.4 になります）
        while (dist < TARGET_DISTANCE && math_val <= 3.0) {
            math_val += STEP;
            next_y = math_val * 100.0 + 240.0;
            next_x = calculateX(math_val, t) * 100.0;

            // 直前の弾からの直線距離を計算
            double dx = next_x - pShot->x;
            double dy = next_y - pShot->y;
            dist = std::sqrt(dx * dx + dy * dy);
        }

        // 見つかった座標を次の開始点にする
        current_x = next_x;
        current_y = next_y;

        // 描画範囲（画面外）まで引き終わったら、残ってしまった余剰な弾は画面外へ隠す
        if (math_val > 3.0) {
            pShot->x = -50;
            pShot->y = -50;
        }

        if (pEnemyShotSet->kind == 1) {
            pShot->x = 480 - pShot->x;
        }

        pShot = pShot->next;
    }
}

static void ShotMilk(sEnemyShotSet* pEnemyShotSet) {
    double t = (pEnemyShotSet->count + 300) / 30.0;

    if (pEnemyShotSet->count % 3 == 0) {
        double y = 0.12 * (2.0 + std::sin(t)) * std::sin(t);
        double x = calculateX(y, t);

        // --- 弾の生成 ---
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->kind = img_enemyShotDiamond[6]; // 弾の種類
        pEnemyShot->x = x * 100;
        pEnemyShot->y = y * 100 + 240;
        pEnemyShot->muki = (GetRand(200) - 100) / 100.0 * (DX_PI / 3.0);
        pEnemyShot->speed = 0.0 + GetRand(500) / 100.0;

        if (pEnemyShotSet->kind == 1) {
            pEnemyShot->x = 480 - pEnemyShot->x;
            pEnemyShot->muki = DX_PI - pEnemyShot->muki;
        }

        // リストへの追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // --- 弾の更新と重力処理 ---
    const double GRAVITY = 0.03; // 重力加速度（弱い重力。好みに合わせて数値を調整してください）

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 1. 現在のX, Y方向の速度成分を分解
        double vx = pShot->speed * std::cos(pShot->muki);
        double vy = pShot->speed * std::sin(pShot->muki);

        // 2. Y方向の速度に重力を加算（Y軸正の方向 ＝ 画面下へ加速）
        vy += GRAVITY;

        // 3. 新しい速度成分から、speed と muki を再計算して上書き
        pShot->speed = std::sqrt(vx * vx + vy * vy);
        pShot->muki = std::atan2(vy, vx);

        // 4. 座標の更新
        pShot->x += vx; // すでにvx, vyを計算済みなのでそのまま足せばOKです
        pShot->y += vy;

        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_Oppai()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    if (count == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotOppai;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    if (count == 301) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMilk;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    if (count == 601) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotOppai;
        pEnemyShotSet->kind = 1;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    if (count == 901) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMilk;
        pEnemyShotSet->kind = 1;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}