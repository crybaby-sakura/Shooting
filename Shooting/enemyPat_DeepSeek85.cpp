// enemyPat_tmp.cpp
// 弾幕パターン：単一許容座標『Zero Coordinate Prison』
// 敵本体関数：void EnemyPat_TAS_DeepSeek()

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// ヘルパー：敵弾セットを生成してグローバルリストへ接続する
// ------------------------------------------------------------
static void AddEnemyShotSet(void (*func)(sEnemyShotSet*), double x, double y, double muki, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;

    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;

    // 弾リストのヘッダーを作成
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    // グローバル弾幕セットリストへ挿入
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ------------------------------------------------------------
// 系統A：全周針山（回転欠け弾幕）
// 60方向へ弾を発射し、1方向だけ欠けを作る。
// 欠けは毎フレーム0.75度回転する。
// ------------------------------------------------------------
static void PatternA(sEnemyShotSet* pSet)
{
    const int    N = 60;      // 全方向数
    const double gapSpeedDeg = 0.75;    // 欠けの回転速度（度/フレーム）
    const double bulletSpeed = 2.0;     // 弾速
    const int    spawnInterval = 6;       // 発射間隔（フレーム）
    const int    color = 6;       // 白

    // 発射位置をボス現在位置に追従
    pSet->x = enemy.x;
    pSet->y = enemy.y;

    // 一定間隔で欠け以外の全方向に弾を発射
    if (pSet->count % spawnInterval == 0) {
        int gapIndex = (int)(pSet->count * gapSpeedDeg * N / 360.0) % N;

        for (int i = 0; i < N; ++i) {
            if (i == gapIndex) continue; // 欠け方向は発射しない

            double angle = i * (2.0 * DX_PI / N);

            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->x;
            shot->y = pSet->y;
            shot->muki = angle;
            shot->speed = bulletSpeed;
            shot->kind = img_enemyShotSmallBall[color];

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }
    }

    // このセットが管理する全弾を移動
    sEnemyShot* shot = pSet->pEnemyShotHead->next;
    while (shot != pSet->pEnemyShotHead) {
        shot->x += shot->speed * cos(shot->muki);
        shot->y += shot->speed * sin(shot->muki);
        shot = shot->next;
    }
}

// ------------------------------------------------------------
// 系統B：完全照準即着弾
// 2フレームごとに自機座標を正確に狙う4発（角度オフセット付き）を発射。
// ------------------------------------------------------------
static void PatternB(sEnemyShotSet* pSet)
{
    const double bulletSpeed = 10.0;    // 高速弾
    const int    fireInterval = 2;       // 発射間隔
    const double offsetDeg[4] = { -0.4, -0.2, 0.2, 0.4 };
    const int    color = 0;       // 赤

    // 発射位置をボス現在位置に追従
    pSet->x = enemy.x;
    pSet->y = enemy.y;

    if (pSet->count % fireInterval == 0) {
        double baseAngle = atan2(player.y - pSet->y, player.x - pSet->x);

        for (int i = 0; i < 4; ++i) {
            double angle = baseAngle + offsetDeg[i] * DX_PI / 180.0;

            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->x;
            shot->y = pSet->y;
            shot->muki = angle;
            shot->speed = bulletSpeed;
            shot->kind = img_enemyShotBullet[color];

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }
    }

    // 移動
    sEnemyShot* shot = pSet->pEnemyShotHead->next;
    while (shot != pSet->pEnemyShotHead) {
        shot->x += shot->speed * cos(shot->muki);
        shot->y += shot->speed * sin(shot->muki);
        shot = shot->next;
    }
}

// ------------------------------------------------------------
// 系統C：回転レーザー波
// 毎フレーム1本の短レーザーを回転角に沿って発射し、連続したビームを形成。
// 回転するレーザーの壁が自機の可動域をさらに制限する。
// ------------------------------------------------------------
static void PatternC(sEnemyShotSet* pSet)
{
    const double bulletSpeed = 4.0;
    const double rotSpeedDeg = 0.5;   // 回転速度（度/フレーム）
    const int    color = 4;     // 青

    // 発射位置をボス現在位置に追従
    pSet->x = enemy.x;
    pSet->y = enemy.y;

    // 毎フレームレーザーを1本発射
    {
        double angle = fmod(pSet->count * rotSpeedDeg * DX_PI / 180.0, 2.0 * DX_PI);

        sEnemyShot* shot = new sEnemyShot;
        shot->x = pSet->x;
        shot->y = pSet->y;
        shot->muki = angle;
        shot->speed = bulletSpeed;
        shot->kind = img_enemyShotLaser[color];

        shot->prev = pSet->pEnemyShotHead->prev;
        shot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = shot;
        pSet->pEnemyShotHead->prev = shot;
    }

    // 移動
    sEnemyShot* shot = pSet->pEnemyShotHead->next;
    while (shot != pSet->pEnemyShotHead) {
        shot->x += shot->speed * cos(shot->muki);
        shot->y += shot->speed * sin(shot->muki);
        shot = shot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_TAS_DeepSeek()
{
    if (count == 1) {
        // ボスを画面中央に固定
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;

        // 3系統の弾幕セットを生成
        AddEnemyShotSet(PatternA, enemy.x, enemy.y, 0.0, 0);
        AddEnemyShotSet(PatternB, enemy.x, enemy.y, 0.0, 1);
        AddEnemyShotSet(PatternC, enemy.x, enemy.y, 0.0, 2);
    }
    else {
        // ボスは移動しない（TASによる精密回避を前提とした固定配置）
        // 必要ならここに微小な移動を追加してもよい
    }
}