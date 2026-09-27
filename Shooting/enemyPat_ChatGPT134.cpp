#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {

    static constexpr double SCREEN_W = 480.0;
    static constexpr double SCREEN_H = 480.0;
    static constexpr int CAGE_BULLET = 0;
    static constexpr int LARGE_BULLET = 1;

    static void AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind, int shotType)
    {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = x;
        pEnemyShot->y = y;
        pEnemyShot->muki = muki;
        pEnemyShot->speed = speed;
        pEnemyShot->kind = kind;
        pEnemyShot->param_i[0] = shotType;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    static void AddCageBullet(sEnemyShotSet* pEnemyShotSet, double x, double y)
    {
        if (x < -8.0 || x > SCREEN_W + 8.0 || y < -8.0 || y > SCREEN_H + 8.0)
            return;

        AddShot(pEnemyShotSet, x, y, 0.0, 0.0, img_enemyShotMediumBall[2], CAGE_BULLET);
    }

    static void BuildCageGrid(sEnemyShotSet* pEnemyShotSet)
    {
        // 縦横の格子。自機を含む画面全体が「檻」になる構成。
        const double spacing = 30.0*2;

        for (int row = 0; row <= 16; ++row) {
            const double y = row * spacing;
            for (int x = 0; x <= 16; ++x)
                AddCageBullet(pEnemyShotSet, x * spacing, y);
        }

        for (int col = 0; col <= 16; ++col) {
            const double x = col * spacing;
            for (int y = 0; y <= 16; ++y)
                AddCageBullet(pEnemyShotSet, x, y * spacing);
        }
    }

    static void AddDiagonal(sEnemyShotSet* pEnemyShotSet, double offset, bool downRight)
    {
        // y = x + offset または y = -x + offset の直線上に弾を並べる。
        double xStart;
        double xEnd;

        if (downRight) {
            xStart = fmax(0.0, -offset);
            xEnd = fmin(SCREEN_W, SCREEN_H - offset);
            if (xStart > xEnd)
                return;

            for (double x = xStart; x <= xEnd + 0.1; x += 30.0)
                AddCageBullet(pEnemyShotSet, x, x + offset);
        }
        else {
            xStart = fmax(0.0, offset - SCREEN_H);
            xEnd = fmin(SCREEN_W, offset);
            if (xStart > xEnd)
                return;

            for (double x = xStart; x <= xEnd + 0.1; x += 30.0)
                AddCageBullet(pEnemyShotSet, x, -x + offset);
        }
    }

    static void BuildCageDiagonalA(sEnemyShotSet* pEnemyShotSet)
    {
        const double offsetsA[] = { -300.0, -150.0, 0.0, 150.0, 300.0 };
        const double offsetsB[] = { 180.0, 330.0, 480.0, 630.0, 780.0 };

        for (double offset : offsetsA)
            AddDiagonal(pEnemyShotSet, offset, true);
        for (double offset : offsetsB)
            AddDiagonal(pEnemyShotSet, offset, false);
    }

    static void BuildCageDiagonalB(sEnemyShotSet* pEnemyShotSet)
    {
        const double offsetsA[] = { -360.0, -180.0, 0.0, 180.0, 360.0 };
        const double offsetsB[] = { 120.0, 300.0, 480.0, 660.0, 840.0 };

        for (double offset : offsetsA)
            AddDiagonal(pEnemyShotSet, offset, true);
        for (double offset : offsetsB)
            AddDiagonal(pEnemyShotSet, offset, false);
    }

    static void BuildCage(sEnemyShotSet* pEnemyShotSet, int pattern)
    {
        if (pattern == 0)
            BuildCageGrid(pEnemyShotSet);
        else if (pattern == 1)
            BuildCageDiagonalA(pEnemyShotSet);
        else
            BuildCageDiagonalB(pEnemyShotSet);
    }

    static void ScatterCageBullets(sEnemyShotSet* pEnemyShotSet)
    {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == CAGE_BULLET && pShot->param_i[1] == 0) {
                pShot->param_i[1] = 1;
                pShot->kind = img_enemyShotSmallBall[2];
                pShot->speed = 2.4 + GetRand(140) / 100.0-2;
                pShot->muki += (GetRand(80) - 40) / 180.0 * DX_PI;
            }
            pShot = pShot->next;
        }
    }

    static void SpawnLargeBall(sEnemyShotSet* pEnemyShotSet, int shotIndex)
    {
        double muki;
        if (shotIndex == 3 || shotIndex == 5) {
            // 4発目・6発目は自機狙い。
            muki = atan2(player.y - (pEnemyShotSet->y + 22.0), player.x - pEnemyShotSet->x);
        }
        else {
            // その他は下方向を中心にしたランダム軌道。
            muki = DX_PI * 0.5 + (GetRand(130) - 65) / 180.0 * DX_PI;
        }

        AddShot(
            pEnemyShotSet,
            pEnemyShotSet->x,
            pEnemyShotSet->y + 22.0,
            muki,
            2.7 + GetRand(40) / 100.0,
            img_enemyShotLargeBall[1],
            LARGE_BULLET
        );

        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    static void BreakCageNearLargeBall(sEnemyShot* pLargeBall, sEnemyShotSet* pEnemyShotSet)
    {
        const double breakRadius = 27.0;
        const double breakRadiusSq = breakRadius * breakRadius;

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == CAGE_BULLET && pShot->param_i[1] == 0) {
                const double dx = pShot->x - pLargeBall->x;
                const double dy = pShot->y - pLargeBall->y;
                const double distSq = dx * dx + dy * dy;

                if (distSq <= breakRadiusSq) {
                    double muki = atan2(dy, dx);
                    if (distSq < 0.01)
                        muki = pLargeBall->muki + DX_PI * 0.5;

                    pShot->param_i[1] = 1;
                    pShot->kind = img_enemyShotSmallBall[2];
                    pShot->speed = 2.8 + GetRand(180) / 100.0-2;
                    pShot->muki = muki + (GetRand(50) - 25) / 180.0 * DX_PI;
                }
            }
            pShot = pShot->next;
        }
    }

    static void MoveShots(sEnemyShotSet* pEnemyShotSet)
    {
        // 大玉が緑弾の檻を横切り、接触した緑弾だけを崩す。
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == CAGE_BULLET && pShot->param_i[1] == 0) {
                // 設置中の緑弾はその場に留まる。
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }

            if (pShot->param_i[0] == LARGE_BULLET)
                BreakCageNearLargeBall(pShot, pEnemyShotSet);

            pShot = pShot->next;
        }
    }

    static void ShotKagome(sEnemyShotSet* pEnemyShotSet)
    {
        static const int largeBallFrames[] = { 24*2, 46*2, 78*2, 100*2, 132*2, 154*2 };

        if (pEnemyShotSet->count == 0) {
            pEnemyShotSet->param_i[0] = 0; // 発射済み大玉数
            pEnemyShotSet->param_i[1] = 0; // 現在の檻パターン
            pEnemyShotSet->param_i[2] = 0; // 終了処理済みか

            if (CheckSoundMem(sound_enemyCharge))
                StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            BuildCage(pEnemyShotSet, pEnemyShotSet->param_i[1]);
        }

        const int largeBallCount = pEnemyShotSet->param_i[0];
        if (largeBallCount < 6 && pEnemyShotSet->count >= largeBallFrames[largeBallCount]) {
            SpawnLargeBall(pEnemyShotSet, largeBallCount);
            ++pEnemyShotSet->param_i[0];

            // 2発目・4発目の後に檻を組み直し、次の面を作る。
            if (largeBallCount == 1 || largeBallCount == 3) {
                ScatterCageBullets(pEnemyShotSet);
                pEnemyShotSet->param_i[1] = (pEnemyShotSet->param_i[1] + 1) % 3;
                BuildCage(pEnemyShotSet, pEnemyShotSet->param_i[1]);
            }
        }

        // 6発目を撃ち終えたら、残った檻も崩壊させて次のループへ移行する。
        if (pEnemyShotSet->param_i[0] >= 6 && pEnemyShotSet->count >= 188*2 && pEnemyShotSet->param_i[2] == 0) {
            ScatterCageBullets(pEnemyShotSet);
            pEnemyShotSet->param_i[2] = 1;
        }

        MoveShots(pEnemyShotSet);
    }

} // namespace

// 敵本体のパターン
void EnemyPat_KagomeKagome_ChatGPT()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 72.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと。
        muki = 1;
    }
    else {
        enemy.x += 0.65 * (double)muki;
        if (enemy.x < 105.0 || enemy.x > 375.0)
            muki *= -1;
    }

    if (count % (228*2) == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKagome;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
