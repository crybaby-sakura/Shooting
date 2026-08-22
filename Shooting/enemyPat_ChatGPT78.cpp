// enemyPat_kaleidoscope.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static double Clamp01(double v)
{
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

static double SmoothStep(double v)
{
    v = Clamp01(v);
    return v * v * (3.0 - 2.0 * v);
}

// ============================================================
// 弾幕：万華鏡の中で遊ぶ
// ============================================================
static void ShotKaleidoscope(sEnemyShotSet* pEnemyShotSet)
{
    const double CX = 240.0;
    const double CY = 245.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const double gap = pEnemyShotSet->param_d[0];
        const int K = 3;

        // 外周
        for (int i = 0; i < 18 * K; i++) {
            double base = i * (2.0 * DX_PI / 18.0 / K);

            double diff = base - gap;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;

            if (fabs(diff) < 0.16)
                continue;

            sEnemyShot* pShot = new sEnemyShot;

            pShot->x = CX;
            pShot->y = CY;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotMediumBall[3];
            pShot->param_i[0] = 0;
            pShot->param_d[0] = base;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 内周
        for (int i = 0; i < 18 * K; i++) {
            double base = i * (2.0 * DX_PI / 18.0 / K);

            double diff = base - gap;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;

            if (fabs(diff) < 0.16)
                continue;

            sEnemyShot* pShot = new sEnemyShot;

            pShot->x = CX;
            pShot->y = CY;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotSmallBall[6];
            pShot->param_i[0] = 1;
            pShot->param_d[0] = base;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 中心の菱形弾
        for (int i = 0; i < 6; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->x = CX;
            pShot->y = CY;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotDiamond[1];
            pShot->param_i[0] = 2;
            pShot->param_d[0] = i * DX_PI / 3.0;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    const int t = pEnemyShotSet->count;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // ====================================================
        // 外周リング
        // ====================================================
        if (pShot->param_i[0] == 0) {
            double r;
            double rot;

            if (t < 120) {
                // 20 -> 206
                double u = SmoothStep(t / 120.0);

                r = 20.0 + 186.0 * u;
                rot = 0.010 * t;
            }
            else if (t < 240) {
                // 206 -> 206
                // 半径を維持しながら揺らす
                double u = (t - 120) / 120.0;

                r = 206.0 + 28.0 * sin(u * DX_PI);
                rot = 1.20 + 0.016 * (t - 120);
            }
            else if (t < 310) {
                // 206 -> 0 を滑らかに収束
                double u = SmoothStep((t - 240) / 70.0);

                r = 206.0 * (1.0 - u);
                rot = 3.12 + 0.023 * (t - 240);
            }
            else {
                // 0から滑らかに炸裂
                double u = t - 310;

                r = 7.5 * u;
                rot = 4.73 + 0.030 * u;
            }

            double theta = pShot->param_d[0] + rot * 0.4;

            pShot->x = CX + r * cos(theta);
            pShot->y = CY + r * sin(theta);
            pShot->muki = theta;
        }

        // ====================================================
        // 内周リング
        // ====================================================
        else if (pShot->param_i[0] == 1) {
            double r;
            double rot;

            if (t < 120) {
                // 12 -> 126
                double u = SmoothStep(t / 120.0);

                r = 12.0 + 114.0 * u;
                rot = -0.017 * t;
            }
            else if (t < 240) {
                // 126 -> 126
                double u = (t - 120) / 120.0;

                r = 126.0 + 18.0 * sin(u * DX_PI);
                rot = -2.04 - 0.022 * (t - 120);
            }
            else if (t < 310) {
                // 126 -> 18 を滑らかに収束
                double u = SmoothStep((t - 240) / 70.0);

                r = 126.0 - 108.0 * u;
                rot = -4.68 - 0.028 * (t - 240);
            }
            else {
                // 中心から徐々に炸裂
                double u = t - 310;

                r = 5.0 * u / 3;
                rot = -6.64 - 0.034 * u / 10;
            }

            double theta = pShot->param_d[0] + rot;

            pShot->x = CX + r * cos(theta);
            pShot->y = CY + r * sin(theta);
            pShot->muki = theta;
        }

        // ====================================================
        // 中心の菱形弾
        // ====================================================
        else {
            double r;
            double theta;

            if (t < 120) {
                double u = SmoothStep(t / 120.0);

                r = 8.0 + 8.0 * u;
                theta = pShot->param_d[0] + 0.028 * t;
            }
            else if (t < 240) {
                double u = (t - 120) / 120.0;

                r = 16.0 + 8.0 * sin(u * DX_PI);
                theta = pShot->param_d[0]
                    + 3.36
                    + 0.028 * (t - 120);
            }
            else if (t < 310) {
                double u = SmoothStep((t - 240) / 70.0);

                r = 24.0 * (1.0 - u);
                theta = pShot->param_d[0]
                    + 6.72
                    + 0.028 * (t - 240);
            }
            else {
                double u = t - 310;

                r = 2.8 * u;
                theta = pShot->param_d[0]
                    + 8.68
                    + 0.030 * u;
            }

            pShot->x = CX + r * cos(theta);
            pShot->y = CY + r * sin(theta);
            pShot->muki = theta;
        }

        pShot = pShot->next;
    }

    if (pEnemyShotSet->count == 310) {
        if (CheckSoundMem(sound_enemyShot_extreme))
            StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_TheMostFun_ChatGPT()
{
    static int muki;
    static int pattern_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 240.0;

        enemy.maxHp = enemy.hp = 60 * 30;

        muki = 1;
        pattern_count = 0;
    }
    else {
        enemy.x += 0.32 * muki;

        if (enemy.x < 90.0 + 100)
            muki = 1;

        if (enemy.x > 390.0 - 100)
            muki = -1;
    }
    enemy.hp--;

    if (count % 150 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKaleidoscope;

        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 245.0;

        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = pattern_count++;

        pEnemyShotSet->param_d[0] =
            (pattern_count % 8) * DX_PI / 4.0
            + DX_PI / 8.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;

        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;

        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}