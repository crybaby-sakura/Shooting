// enemyPat_Tmp.cpp
// 蚊遣り火渦（かやりびうず）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 定数
// ------------------------------------------------------------
static const double PI = 3.14159265358979323846;

// 渦の弾数・巻き数・半径
static const int    SPIRAL_BULLET_COUNT = 36 * 5;
static const double SPIRAL_TURNS = 3.0;
static const double SPIRAL_TOTAL_ANGLE = SPIRAL_TURNS * 2.0 * PI;
static const double SPIRAL_MIN_RADIUS = 480.0 * 0.06;   // 28.8
static const double SPIRAL_MAX_RADIUS = 480.0 * 0.52;   // 105.6

// 渦の回転速度（20度/秒 → 1フレームあたりのラジアン）
static const double SPIRAL_ROT_PER_FRAME = (20.0 * PI / 180.0) / 60.0;

// 煙の発生間隔（フレーム）
static const int    SMOKE_INTERVAL = 36 / 12;   // 0.6秒
// 火の粉（自機狙い）の発生間隔（フレーム）
static const int    SPARK_INTERVAL = 108;  // 1.8秒

// ------------------------------------------------------------
// 弾リストへ追加するヘルパー
// ------------------------------------------------------------
static void AddShotToList(sEnemyShotSet* pSet, sEnemyShot* pShot)
{
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// ------------------------------------------------------------
// 弾幕パターン更新関数
// ------------------------------------------------------------
static void UpdateKayaribi(sEnemyShotSet* pSet)
{
    // 現在の敵位置を基準にする（敵は動くため毎フレーム更新）
    pSet->x = enemy.x;
    pSet->y = enemy.y;

    // --- 初期化（count == 0 のとき） ---
    if (pSet->count == 0) {
        // 渦本体：オレンジの小玉を螺旋状に配置
        for (int i = 0; i < SPIRAL_BULLET_COUNT; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->kind = img_enemyShotSmallBall[8];   // 橙
            pShot->param_i[0] = 0;                     // 種類：渦
            pShot->param_d[0] = i * (SPIRAL_TOTAL_ANGLE / SPIRAL_BULLET_COUNT); // 位相オフセット
            pShot->x = enemy.x;
            pShot->y = enemy.y;
            pShot->muki = 0.0;
            pShot->speed = 0.0;
            pShot->margin = 240;

            AddShotToList(pSet, pShot);
        }

        // 火種：中心の赤い中玉
        sEnemyShot* pFire = new sEnemyShot;
        pFire->kind = img_enemyShotMediumBall[0];      // 赤
        pFire->param_i[0] = 1;                         // 種類：火種
        pFire->x = enemy.x;
        pFire->y = enemy.y;
        pFire->muki = 0.0;
        pFire->speed = 0.0;
        AddShotToList(pSet, pFire);

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // --- 各弾の更新 ---
    double globalTheta = fmod(pSet->count * SPIRAL_ROT_PER_FRAME, SPIRAL_TOTAL_ANGLE);

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case 0: // 渦
        {
            double theta = fmod(globalTheta + pShot->param_d[0], SPIRAL_TOTAL_ANGLE);
            double radius = SPIRAL_MIN_RADIUS + (theta / SPIRAL_TOTAL_ANGLE) * (SPIRAL_MAX_RADIUS - SPIRAL_MIN_RADIUS);
            pShot->x = enemy.x + radius * cos(theta);
            pShot->y = enemy.y + radius * sin(theta);
            break;
        }
        case 1: // 火種
            pShot->x = enemy.x;
            pShot->y = enemy.y;
            break;

        case 2: // 煙
        {
            // パラメータ：param_d[0]=初期X, [1]=初期Y, [2]=振幅, [3]=角周波数, [4]=位相, [5]=上昇速度
            double t = (double)pShot->count;
            pShot->x = pShot->param_d[0] + pShot->param_d[2] * sin(pShot->param_d[3] * t + pShot->param_d[4]);
            pShot->y = pShot->param_d[1] - pShot->param_d[5] * t;   // 上へ（Y座標が減る）
            break;
        }
        case 3: // 火の粉（自機狙い）
        {
            if (pShot->count >= pShot->param_i[1]) {
                // 発射タイミングになったら速度を設定
                if (pShot->speed == 0.0) {
                    pShot->muki = pShot->param_d[0];
                    pShot->speed = 260.0 / 60.0;  // 260ピクセル/秒
                }
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            // 発射前はその場に留まる
            break;
        }
        default:
            break;
        }

        pShot = pShot->next;
    }

    // --- 煙の発生 ---
    if (pSet->count > 0 && pSet->count % SMOKE_INTERVAL == 0) {
        double angle = GetRand(359) * PI / 180.0;
        double sx = enemy.x + SPIRAL_MAX_RADIUS * cos(angle);
        double sy = enemy.y + SPIRAL_MAX_RADIUS * sin(angle);

        sEnemyShot* pSmoke = new sEnemyShot;
        pSmoke->kind = img_enemyShotSmallBall[6];      // 白
        pSmoke->param_i[0] = 2;                        // 種類：煙
        pSmoke->param_d[0] = sx;                       // 初期X
        pSmoke->param_d[1] = sy;                       // 初期Y
        pSmoke->param_d[2] = 8.0 + GetRand(4);         // 振幅（ピクセル）
        pSmoke->param_d[3] = 0.15 + GetRand(10) * 0.005; // 角周波数
        pSmoke->param_d[4] = GetRand(360) * PI / 180.0;  // 位相
        pSmoke->param_d[5] = 0.6 + GetRand(3) * 0.1;     // 上昇速度（ピクセル/フレーム）
        pSmoke->x = sx;
        pSmoke->y = sy;
        pSmoke->muki = 0.0;
        pSmoke->speed = 0.0;
        pSmoke->margin = 240;

        AddShotToList(pSet, pSmoke);

        //if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        //PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // --- 火の粉（自機狙い3連射）の発生 ---
    if (pSet->count > 0 && pSet->count % SPARK_INTERVAL == 0) {
        double aim = atan2(player.y - enemy.y, player.x - enemy.x);
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pSpark = new sEnemyShot;
            pSpark->kind = img_enemyShotBullet[0];     // 赤い針弾
            pSpark->param_i[0] = 3;                    // 種類：火の粉
            pSpark->param_i[1] = i * 6;                // 発射遅延（フレーム）
            pSpark->param_d[0] = aim;                  // 狙い角
            pSpark->x = enemy.x;
            pSpark->y = enemy.y;
            pSpark->muki = aim;
            pSpark->speed = 0.0;

            AddShotToList(pSet, pSpark);
        }

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_MosquitoCoil_DeepSeek()
{
    static int muki;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // 弾幕セットを作成（1回だけ）
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = UpdateKayaribi;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0; // 未使用

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵本体の移動（左右にゆっくり）
        enemy.x += 0.38 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }
}