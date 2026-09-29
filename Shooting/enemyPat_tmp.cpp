// enemyPat_Tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 10分 = 60fps * 600秒 = 36000フレーム
static const int TOTAL_FRAMES = 36000/20;
// 最後の1秒 = 60フレーム
static const int FINAL_FRAMES = 60;

// 敵出現時の count を記録（パターン関数からも参照する）
static int g_startCount = -1;

// 前方宣言
static void PatternGridReflect(sEnemyShotSet* pEnemyShotSet);
static void PatternWave(sEnemyShotSet* pEnemyShotSet);

// 弾幕セットを生成するヘルパー
static void CreateSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ------------------------------------------------------------
// 格子弾パターン：反射・追尾・巨大化・最終収束
// ------------------------------------------------------------
static void PatternGridReflect(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 効果音（予告・生成）
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // プレイヤー速度記録用の初期値
        pEnemyShotSet->param_d[0] = player.x;
        pEnemyShotSet->param_d[1] = player.y;

        // 16x16 = 256 発の低速格子弾
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 16; j++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = 15.0 + i * 30.0;
                pEnemyShot->y = 15.0 + j * 30.0;
                pEnemyShot->muki = GetRand(359) / 180.0 * DX_PI;
                pEnemyShot->speed = 0.5 + GetRand(100) / 100.0;
                pEnemyShot->kind = img_enemyShotSmallBall[GetRand(8)];
                pEnemyShot->param_i[0] = 0; // 反射回数
                pEnemyShot->param_i[1] = 0; // 巨大化段階

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // プレイヤー速度を算出し、その逆方向へ弾を曲げる
    double pvx = player.x - pEnemyShotSet->param_d[0];
    double pvy = player.y - pEnemyShotSet->param_d[1];
    pEnemyShotSet->param_d[0] = player.x;
    pEnemyShotSet->param_d[1] = player.y;

    double targetAngle = atan2(pvy, pvx) + DX_PI; // プレイヤーの移動と逆方向
    double pvLen = sqrt(pvx * pvx + pvy * pvy);

    bool finalPhase = (count - g_startCount) > (TOTAL_FRAMES - FINAL_FRAMES);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (finalPhase) {
            // 最終1秒：全弾がプレイヤーへ収束
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 8.0 + (pShot->count % 60) * 0.1;
        }
        else {
            // 徐々に加速
            if (pShot->speed < 6.0) pShot->speed += 0.001;

            // プレイヤーの動きと逆方向へゆっくり旋回
            if (pvLen > 0.1) {
                double diff = targetAngle - pShot->muki;
                while (diff > DX_PI) diff -= 2 * DX_PI;
                while (diff < -DX_PI) diff += 2 * DX_PI;
                pShot->muki += diff * 0.01;
            }
        }

        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 壁反射（最終1秒は反射しない）
        if (!finalPhase) {
            if (pShot->x < 0.0) {
                pShot->x = 0.0;
                pShot->muki = DX_PI - pShot->muki;
                pShot->speed *= 1.03;
                pShot->param_i[0]++;
            }
            if (pShot->x > 480.0) {
                pShot->x = 480.0;
                pShot->muki = DX_PI - pShot->muki;
                pShot->speed *= 1.03;
                pShot->param_i[0]++;
            }
            if (pShot->y < 0.0) {
                pShot->y = 0.0;
                pShot->muki = -pShot->muki;
                pShot->speed *= 1.03;
                pShot->param_i[0]++;
            }
            if (pShot->y > 480.0) {
                pShot->y = 480.0;
                pShot->muki = -pShot->muki;
                pShot->speed *= 1.03;
                pShot->param_i[0]++;
            }

            // 反射回数に応じて弾を巨大化
            if (pShot->param_i[0] > 5 && pShot->param_i[1] == 0) {
                pShot->param_i[1] = 1;
                pShot->kind = img_enemyShotMediumBall[GetRand(8)];
            }
            else if (pShot->param_i[0] > 10 && pShot->param_i[1] == 1) {
                pShot->param_i[1] = 2;
                pShot->kind = img_enemyShotLargeBall[GetRand(8)];
            }
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 波パターン：30秒ごとにプレイヤー位置から放射状に射出
// ------------------------------------------------------------
static void PatternWave(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int num = 64;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            double angle = (double)i / num * 2.0 * DX_PI;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.0 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumBall[GetRand(8)];
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    bool finalPhase = (count - g_startCount) > (TOTAL_FRAMES - FINAL_FRAMES);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (finalPhase) {
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 8.0;
        }
        else {
            if (pShot->speed < 5.0) pShot->speed += 0.002;
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Tmp()
{
    static bool initialized = false;
    static int muki = 1;
    static bool finalStarted = false;

    if (!initialized) {
        initialized = true;
        g_startCount = count;

        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = 200;
        enemy.hp = 200;
        muki = 1;
        finalStarted = false;

        // 最初の格子弾セット
        CreateSet(PatternGridReflect, enemy.x, enemy.y + 10.0, 0.0, 0);
    }

    // 敵の左右移動
    enemy.x += 0.98 * muki;
    if (count % 120 == 60) muki *= -1;
    if (enemy.x < 20.0) { enemy.x = 20.0; muki = 1; }
    if (enemy.x > 460.0) { enemy.x = 460.0; muki = -1; }

    int localFrame = count - g_startCount;

    // 最終1秒まで無敵。最終1秒に入った瞬間だけ HP を 1 にする。
    if (localFrame < TOTAL_FRAMES - FINAL_FRAMES) {
        enemy.hp = enemy.maxHp;
    }
    else {
        if (!finalStarted) {
            finalStarted = true;
            enemy.hp = 1;
        }
    }

    // 30秒ごとに波パターン
    if (localFrame > 0 && localFrame % 180 == 0) {
        CreateSet(PatternWave, enemy.x, enemy.y + 10.0, 0.0, 1);
    }

    // 120秒ごとに追加の格子弾セット
    if (localFrame > 0 && localFrame % 720 == 0) {
        CreateSet(PatternGridReflect, 240.0, 240.0, 0.0, 2);
    }
}