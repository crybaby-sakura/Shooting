// enemyPat_dodecahedron.cpp
// 正十二面体モチーフ弾幕：五重奏の十二審判（Pentagonal Dodecahedral Verdict）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// フェーズ1：十二面の星
// 正十二面体の12面に対応する12方向（面核）から、各5wayの弾を放射。
// 合計60本。面法線は正二十面体の頂点方向（黄金比に基づく角度）を使用。
// ------------------------------------------------------------
static void ShotDodecaPhase1(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    const double RAD = DX_PI / 180.0;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 正十二面体の12面の法線方向（方位角）と速度（仰角に応じて変化）
        // 正二十面体の12頂点 = 正十二面体の12面法線
        // 0°×2, 58.28°, 90°×2, 121.72°, 180°×2, 238.28°, 270°×2, 301.72°
        const double baseAngles[12] = {
            0.0, 0.0,
            58.2825255885389,
            90.0, 90.0,
            121.717474411461,
            180.0, 180.0,
            238.282525588539,
            270.0, 270.0,
            301.717474411461
        };
        // 仰角が大きい（上/下向き）ほど2D上では遅く見える
        const double speedBase[12] = {
            1.6, 1.6,   // (±φ, 0, ±1)  z=±1  中速
            2.0,        // ( 1,  φ,  0)  z=0   最速
            1.2, 1.2,   // ( 0, ±1, ±φ)  z=±φ  遅め
            2.0,        // (-1,  φ,  0)  z=0   最速
            1.6, 1.6,   // (-φ, 0, ±1)  z=±1  中速
            2.0,        // (-1, -φ,  0)  z=0   最速
            1.2, 1.2,   // ( 0, -1, ±φ)  z=±φ  遅め
            2.0         // ( 1, -φ,  0)  z=0   最速
        };

        for (int face = 0; face < 12; face++) {
            double baseA = baseAngles[face] * RAD;
            for (int k = 0; k < 5; k++) {
                pShot = new sEnemyShot;

                // 面核位置：敵位置から基準角度方向に少しオフセット
                double offsetDist = 8.0;
                pShot->x = pSet->x + offsetDist * cos(baseA);
                pShot->y = pSet->y + offsetDist * sin(baseA);

                // 五角形の頂点方向をイメージした扇状5way（基準角度中心に±72°）
                double spread = (k * 72.0 - 144.0) * RAD;
                pShot->muki = baseA + spread;
                pShot->speed = speedBase[face] + GetRand(10) / 100.0;

                // 面ごとに色を変えて幾何学的パターンを強調
                pShot->kind = img_enemyShotSmallBall[face % 8];

                // パラメータ保存（後続フェーズで参照する場合用）
                pShot->param_d[0] = baseA;

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 弾更新：直進
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
// フェーズ2：頂点の交差
// 正十二面体の20頂点に対応する20方向に弾を発射。
// 一定距離進むと「双対多面体（正二十面体）の面中心＝プレイヤー方向」へ折射。
// ------------------------------------------------------------
static void ShotDodecaPhase2(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    const double RAD = DX_PI / 180.0;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 正十二面体の20頂点の方位角（z重複は速度で区別）
        const double vtxAngle[20] = {
            0.0, 0.0,           // (±φ, 0, ±1/φ)
            45.0, 45.0,         // (±1, ±1, ±1)
            69.0948425521207,   // (±1/φ, ±φ, 0)
            90.0, 90.0,         // (0, ±1/φ, ±φ)
            110.905157447879,   // (±1/φ, ±φ, 0)
            135.0, 135.0,       // (±1, ±1, ±1)
            180.0, 180.0,       // (±φ, 0, ±1/φ)
            225.0, 225.0,       // (±1, ±1, ±1)
            249.094842552121,   // (±1/φ, ±φ, 0)
            270.0, 270.0,       // (0, ±1/φ, ±φ)
            290.905157447879,   // (±1/φ, ±φ, 0)
            315.0, 315.0        // (±1, ±1, ±1)
        };
        // z成分に応じた速度（水平に近いほど速い）
        const double vtxSpeed[20] = {
            1.5, 1.9,
            1.6, 2.0,
            2.2,
            1.4, 1.8,
            2.2,
            1.6, 2.0,
            1.5, 1.9,
            1.6, 2.0,
            2.2,
            1.4, 1.8,
            2.2,
            1.6, 2.0
        };

        for (int i = 0; i < 20; i++) {
            pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = vtxAngle[i] * RAD;
            pShot->speed = vtxSpeed[i] * 4;
            pShot->kind = img_enemyShotMediumBall[i % 8];

            // 0:直進中, 1:折射後
            pShot->param_i[0] = 0;
            // 折射開始距離（敵からの距離）
            pShot->param_d[0] = (55.0 + GetRand(25)) * 4;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 0) {
            // 直進フェーズ
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);

            double dx = p->x - pSet->x;
            double dy = p->y - pSet->y;
            if (dx * dx + dy * dy > p->param_d[0] * p->param_d[0]) {
                // 頂点に到達 → 双対多面体の面中心（プレイヤー方向）へ折射
                p->param_i[0] = 1;
                p->muki = atan2(player.y - p->y, player.x - p->x);
                p->speed *= 0.75 / 4; // 曲がったら少し減速
            }
        }
        else {
            // 折射後は直進
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// ------------------------------------------------------------
// フェーズ3：五芒星の連鎖
// 敵位置を中心とする正五角形の頂点を、五芒星の描き順で追いかける。
// 3重の同心円で15本の弾が螺旋状に広がりながら五芒星を描く。
// ------------------------------------------------------------
static void ShotDodecaPhase3(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    const double RAD = DX_PI / 180.0;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 五芒星の描き順：0→2→4→1→3→0
        const int starOrder[5] = { 0, 2, 4, 1, 3 };
        double cx = pSet->x;
        double cy = pSet->y;

        // 3重の同心円で連鎖
        for (int ring = 0; ring < 3; ring++) {
            double r = 35.0 + ring * 28.0;
            for (int i = 0; i < 5; i++) {
                pShot = new sEnemyShot;

                int vtx = starOrder[i];
                double angle = vtx * 72.0 * RAD;
                pShot->x = cx + r * cos(angle);
                pShot->y = cy + r * sin(angle);
                pShot->muki = 0.0;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotLargeBall[(ring * 5 + i) % 8];

                // 現在の頂点インデックス（starOrder内）
                pShot->param_i[0] = i;
                // 中心座標と半径
                pShot->param_d[0] = cx;
                pShot->param_d[1] = cy;
                pShot->param_d[2] = r;
                // 半径の拡大速度（螺旋状に広がる）
                pShot->param_d[3] = (0.12 + ring * 0.04) * 3;

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        const int starOrder[5] = { 0, 2, 4, 1, 3 };
        int idx = p->param_i[0];
        int currentVtx = starOrder[idx];
        int nextVtx = starOrder[(idx + 1) % 5];

        double cx = p->param_d[0];
        double cy = p->param_d[1];
        // 時間経過で半径が拡大（螺旋状の五芒星）
        double r = p->param_d[2] + p->count * p->param_d[3];

        // 目標頂点座標
        double targetX = cx + r * cos(nextVtx * 72.0 * RAD);
        double targetY = cy + r * sin(nextVtx * 72.0 * RAD);

        double dx = targetX - p->x;
        double dy = targetY - p->y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < 3.0) {
            // 目標頂点に到達 → 次の頂点へ
            p->param_i[0] = (idx + 1) % 5;
        }
        else {
            p->muki = atan2(dy, dx);
            p->speed = 2.0;
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }

        p = p->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// 3フェーズを約15秒周期で循環。
// ------------------------------------------------------------
void EnemyPat_Dodecahedron_Kimi()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // ゆるやかなホバリング（正十二面体の中心に佇むイメージ）
        enemy.x = 240.0 + 30.0 * sin(count * 0.015);
        enemy.y = 160.0 + 15.0 * sin(count * 0.025);
    }

    // 900フレーム（約15秒）で1サイクル
    int cycle = (count - 1) % 270;

    if (cycle == 0) {
        // フェーズ1：十二面の星（12面×5way＝60本）
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotDodecaPhase1;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else if (cycle == 90) {
        // フェーズ2：頂点の交差（20頂点）
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotDodecaPhase2;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 1;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else if (cycle == 180) {
        // フェーズ3：五芒星の連鎖（3重×5本＝15本）
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotDodecaPhase3;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 2;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}