// enemyPat_spirograph.cpp
// スピログラフモチーフ弾幕：機巧星華

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 外歯車（固定環）— 大きな歯車を弾の輪で表現
// 24発の弾を円周上に配置し、全体として回転させる
// ------------------------------------------------------------
static void ShotOuterGear(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int i;

    // 初回生成：24発で歯車を構成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int teeth = 24;
        for (i = 0; i < teeth; i++) {
            pEnemyShot = new sEnemyShot;

            // 歯の凹凸：偶数番目を大玉(赤)、奇数番目を中玉(橙)
            if (i % 2 == 0) {
                pEnemyShot->kind = img_enemyShotLargeBall[0];   // 赤
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[8];  // 橙
            }

            // 初期配置角度
            pEnemyShot->param_d[0] = DX_PI * 2.0 * i / teeth;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム：回転角を進めて位置を再計算
    double baseAngle = pEnemyShotSet->param_d[3] * pEnemyShotSet->count;
    double radius = pEnemyShotSet->param_d[2];

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double angle = pShot->param_d[0] + baseAngle;
        pShot->x = pEnemyShotSet->x + radius * cos(angle);
        pShot->y = pEnemyShotSet->y + radius * sin(angle);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 内歯車（回転環）— 小さな歯車を弾の輪で表現
// 外歯車の内側を公転＋自転させる
// ------------------------------------------------------------
static void ShotInnerGear(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int i;

    // 初回生成：8発で歯車を構成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int teeth = 8;
        for (i = 0; i < teeth; i++) {
            pEnemyShot = new sEnemyShot;

            // 歯の凹凸：偶数番目を中玉(青)、奇数番目を小玉(シアン)
            if (i % 2 == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[4];  // 青
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[3];   // シアン
            }

            // 自転基準角度
            pEnemyShot->param_d[0] = DX_PI * 2.0 * i / teeth;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 公転計算
    double orbitAngle = pEnemyShotSet->param_d[3] * pEnemyShotSet->count;
    double orbitR = pEnemyShotSet->param_d[2];
    double selfSpeed = pEnemyShotSet->param_d[4];
    double selfR = pEnemyShotSet->param_d[5];

    double centerX = pEnemyShotSet->x + orbitR * cos(orbitAngle);
    double centerY = pEnemyShotSet->y + orbitR * sin(orbitAngle);

    // 火花エフェクト（噛み合い演出）：30フレームごとに内歯車中心から散らす
    if (pEnemyShotSet->count % 30 == 0) {
        sEnemyShot* pSpark = new sEnemyShot;
        pSpark->x = centerX;
        pSpark->y = centerY;
        // GetRand(360) は 0〜360 の整数 → 0〜2π
        pSpark->muki = GetRand(360) / 180.0 * DX_PI;
        pSpark->speed = 0.5 + GetRand(100) / 100.0;
        pSpark->kind = img_enemyShotSmallBall[1];  // 黄
        pSpark->param_i[0] = 60;  // 寿命フレーム（火花専用フラグ兼用）

        pSpark->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pSpark->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pSpark;
        pEnemyShotSet->pEnemyShotHead->prev = pSpark;
    }

    // 弾更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        // param_i[0] > 0 のものは火花として扱う
        if (pShot->param_i[0] > 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            //pShot->param_i[0]--;

            // 寿命切れで自前削除（メインルーチンの画面外消去とは別）
            //if (pShot->param_i[0] <= 0) {
            //    pShot->prev->next = pShot->next;
            //    pShot->next->prev = pShot->prev;
            //    delete pShot;
            //}
        }
        else {
            // 歯車弾：自転
            double angle = pShot->param_d[0] + selfSpeed * pEnemyShotSet->count;
            pShot->x = centerX + selfR * cos(angle);
            pShot->y = centerY + selfR * sin(angle);
        }

        pShot = pNext;
    }
}

// ------------------------------------------------------------
// スピログラフ描画点 — 曲線を描く弾を生成・更新
// ------------------------------------------------------------
static void ShotSpirograph(sEnemyShotSet* pEnemyShotSet)
{
    int interval = pEnemyShotSet->param_i[1];
    if (interval <= 0) interval = 2;

    // 新規弾生成
    if (pEnemyShotSet->count % interval == 0) {
        if (pEnemyShotSet->count % 10 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        int numPoints = pEnemyShotSet->param_i[0];
        if (numPoints <= 0) numPoints = 3;

        double R = pEnemyShotSet->param_d[0];
        double r = pEnemyShotSet->param_d[1];
        double d = pEnemyShotSet->param_d[2];
        double thetaStep = pEnemyShotSet->param_d[3];
        double baseTheta = pEnemyShotSet->count * thetaStep;
        double centerX = pEnemyShotSet->param_d[4];
        double centerY = pEnemyShotSet->param_d[5];

        for (int i = 0; i < numPoints; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            double phase = DX_PI * 2.0 * i / numPoints;
            double t = baseTheta + phase;

            // スピログラフ曲線座標
            double x = (R - r) * cos(t) + d * cos((R - r) / r * t);
            double y = (R - r) * sin(t) - d * sin((R - r) / r * t);

            pShot->x = centerX + x;
            pShot->y = centerY + y;
            pShot->kind = img_enemyShotSmallBall[2];  // 緑

            // 曲線パラメータを保存
            pShot->param_d[0] = t;        // θ
            pShot->param_d[1] = R;
            pShot->param_d[2] = r;
            pShot->param_d[3] = d;
            pShot->param_d[4] = centerX;  // 中心X
            pShot->param_d[5] = centerY;  // 中心Y
            //pShot->speed = thetaStep;     // θ増加量
            pShot->speed = 1.0;
            pShot->muki = atan2(y, x);

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 既存弾更新
    //sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    //while (pShot != pEnemyShotSet->pEnemyShotHead) {
    //    sEnemyShot* pNext = pShot->next;

    //    // θを進めて座標を再計算
    //    //pShot->param_d[0] += pShot->speed;
    //    double t = pShot->param_d[0];
    //    double R = pShot->param_d[1];
    //    double r = pShot->param_d[2];
    //    double d = pShot->param_d[3];

    //    double x = (R - r) * cos(t) + d * cos((R - r) / r * t);
    //    double y = (R - r) * sin(t) - d * sin((R - r) / r * t);

    //    pShot->x = pShot->param_d[4] + x;
    //    pShot->y = pShot->param_d[5] + y;

    //    // 寿命管理：180フレーム（約3秒）で消去
    //    if (pShot->count > 180) {
    //        pShot->prev->next = pShot->next;
    //        pShot->next->prev = pShot->prev;
    //        delete pShot;
    //    }

    //    pShot = pNext;
    //}

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Spirograph_Kimi()
{
    static sEnemyShotSet* pOuter = nullptr;
    static sEnemyShotSet* pInner = nullptr;
    static sEnemyShotSet* pSpiro = nullptr;
    static int phase;
    static double muki;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        muki = 1.0;

        // --- 外歯車（固定環）生成 ---
        pOuter = new sEnemyShotSet;
        pOuter->count = 0;
        pOuter->patternFunc = ShotOuterGear;
        pOuter->x = enemy.x;
        pOuter->y = enemy.y;
        pOuter->param_d[2] = 100.0;  // 半径
        pOuter->param_d[3] = 0.013;  // 回転速度（ラジアン/フレーム、約1周/8秒）

        pOuter->pEnemyShotHead = new sEnemyShot;
        pOuter->pEnemyShotHead->prev = pOuter->pEnemyShotHead;
        pOuter->pEnemyShotHead->next = pOuter->pEnemyShotHead;

        pOuter->prev = enemyShotSetHead.prev;
        pOuter->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pOuter;
        enemyShotSetHead.prev = pOuter;

        // --- 内歯車（回転環）生成 ---
        pInner = new sEnemyShotSet;
        pInner->count = 0;
        pInner->patternFunc = ShotInnerGear;
        pInner->x = enemy.x;
        pInner->y = enemy.y;
        pInner->param_d[2] = 60.0;   // 公転半径
        pInner->param_d[3] = 0.052;  // 公転速度（約1周/2秒）
        pInner->param_d[4] = 0.105;  // 自転速度（約1周/1秒）
        pInner->param_d[5] = 20.0;   // 自転半径（歯車サイズ）

        pInner->pEnemyShotHead = new sEnemyShot;
        pInner->pEnemyShotHead->prev = pInner->pEnemyShotHead;
        pInner->pEnemyShotHead->next = pInner->pEnemyShotHead;

        pInner->prev = enemyShotSetHead.prev;
        pInner->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pInner;
        enemyShotSetHead.prev = pInner;

        // --- スピログラフ生成 ---
        pSpiro = new sEnemyShotSet;
        pSpiro->count = 0;
        pSpiro->patternFunc = ShotSpirograph;
        pSpiro->x = enemy.x;
        pSpiro->y = enemy.y;
        pSpiro->param_d[0] = 80.0;   // R（固定円半径）
        pSpiro->param_d[1] = 25.0;   // r（回転円半径）
        pSpiro->param_d[2] = 20.0;   // d（描画点距離）
        pSpiro->param_d[3] = 0.03;   // θ増加量
        pSpiro->param_d[4] = enemy.x;
        pSpiro->param_d[5] = enemy.y;
        pSpiro->param_i[0] = 3;      // 描画点数
        pSpiro->param_i[1] = 2;      // 生成間隔（フレーム）

        pSpiro->pEnemyShotHead = new sEnemyShot;
        pSpiro->pEnemyShotHead->prev = pSpiro->pEnemyShotHead;
        pSpiro->pEnemyShotHead->next = pSpiro->pEnemyShotHead;

        pSpiro->prev = enemyShotSetHead.prev;
        pSpiro->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSpiro;
        enemyShotSetHead.prev = pSpiro;
    }

    // 敵の動き：ゆっくり左右移動
    enemy.x += 0.6 * muki;
    if (count % 180 == 90) muki *= -1.0;

    // ShotSetの中心位置を敵に追従
    if (pOuter) {
        pOuter->x = enemy.x;
        pOuter->y = enemy.y;
    }
    if (pInner) {
        pInner->x = enemy.x;
        pInner->y = enemy.y;
    }
    if (pSpiro) {
        pSpiro->param_d[4] = enemy.x;
        pSpiro->param_d[5] = enemy.y;
    }

    // フェーズ遷移
    if (count == 600 && phase == 0) {       // 10秒：フェーズ2
        phase = 1;
        pInner->param_d[3] *= 1.5;          // 公転加速
        pInner->param_d[4] *= 1.5;          // 自転加速
        pSpiro->param_i[0] = 5;             // 描画点を5箇所に増加
        pSpiro->param_d[3] *= 1.3;          // 描画速度上昇
    }
    else if (count == 1200 && phase == 1) { // 20秒：フェーズ3
        phase = 2;
        pOuter->param_d[3] *= -1.0;         // 外歯車回転方向反転
    }
    else if (count == 1800 && phase == 2) { // 30秒：フェーズ4
        phase = 3;
    }

    // フェーズ4：外歯車の半径を徐々に縮小（高密度絶弾へ）
    if (phase >= 3 && pOuter && pOuter->param_d[2] > 50.0) {
        pOuter->param_d[2] -= 0.15;
    }
}