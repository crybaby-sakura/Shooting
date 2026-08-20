// enemyPat_Rainbow.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 虹色七連弾
//
// 既存素材の中玉を使い、赤・橙・黄・緑・シアン・青・マゼンタの
// 7色で虹を表現する。レーザー弾は使わない。
// ============================================================
static void ShotRainbow(sEnemyShotSet* pEnemyShotSet)
{
    const double CX = 240.0;
    const double CY = 255.0;
    const double PI = DX_PI;
    const double RAINBOW_R = 175.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 1組につき 7色×3発。3発を少しずらして虹を点線ではなく帯状に見せる。
        for (int color = 0; color < 7; color++) {
            const int N = 11;
            for (int sample = 0; sample < N; sample++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                const double arcAngle =
                    -PI + color * (PI / 6.0)
                    + (sample - N / 2) * 0.045;

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = arcAngle;
                pEnemyShot->speed = 0.0;

                // 虹色: 赤・橙・黄・緑・シアン・青・マゼンタ
                const int colorIndex[7] = { 0, 8, 1, 2, 3, 4, 5 };
                pEnemyShot->kind = img_enemyShotMediumBall[colorIndex[color]];

                // 弾ごとの軌道情報。フレームごとの移動積算ではなく count から直接位置を求める。
                pEnemyShot->param_d[0] = arcAngle;
                pEnemyShot->param_d[1] = CX + RAINBOW_R * cos(arcAngle);
                pEnemyShot->param_d[2] = CY + RAINBOW_R * sin(arcAngle);
                pEnemyShot->param_d[3] = ((pEnemyShotSet->kind & 1) == 0) ? 1.0 : -1.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int c = pShot->count;
        const double baseAngle = pShot->param_d[0];
        const double targetX = pShot->param_d[1];
        const double targetY = pShot->param_d[2];
        const double sign = pShot->param_d[3];

        if (c < 60) {
            // 1. 虹が形作られる: ボスから所定の虹の位置まで滑らかに移動。
            const double u = c / 60.0;
            const double e = u * u * (3.0 - 2.0 * u);
            pShot->x = pEnemyShotSet->x + (targetX - pEnemyShotSet->x) * e;
            pShot->y = pEnemyShotSet->y + (targetY - pEnemyShotSet->y) * e;
        }
        else if (c < 120) {
            // 2. 虹が大きく回り、プレイヤーを包み込む。
            const double u = (c - 60) / 60.0;
            const double angle = baseAngle + sign * 1.05 * u;
            pShot->x = CX + RAINBOW_R * cos(angle);
            pShot->y = CY + RAINBOW_R * sin(angle);
        }
        else if (c < 180) {
            // 3. 七色の弾が中央へ巻き込まれる。
            const double u = (c - 120) / 60.0;
            const double angle = baseAngle + sign * 1.05;
            const double radius = RAINBOW_R * (1.0 - u);
            pShot->x = CX + radius * cos(angle);
            pShot->y = CY + radius * sin(angle);
        }
        else if (c < 240) {
            // 4. 虹の崩壊。中央から一斉に外へ弾ける。
            const double u = (c - 180) / 60.0;
            const double angle = baseAngle + sign * 1.05 + PI;
            const double radius = RAINBOW_R * u;
            pShot->x = CX + radius * cos(angle);
            pShot->y = CY + radius * sin(angle);
        }
        else {
            // 最後は自然に画面外へ抜けるよう、そのまま外向きに進ませる。
            const double u = (c - 240) / 60.0;
            const double angle = baseAngle + sign * 1.05 + PI;
            const double radius = RAINBOW_R + 180.0 * u;
            pShot->x = CX + radius * cos(angle);
            pShot->y = CY + radius * sin(angle);
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Rainbow_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.95 * (double)muki;
        if (count % 120 == 60)
            muki *= -1;
    }

    // 虹のアーチを少しずつ重ね、完成→包囲→収束→崩壊を繰り返す。
    if (count % 30 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRainbow;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}