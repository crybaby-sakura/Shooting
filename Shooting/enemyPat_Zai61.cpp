// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 正十二面体の幾何学データ
// ============================================================
static const double PHI = (1.0 + sqrt(5.0)) / 2.0;
static const double INV_PHI = 1.0 / PHI;

// 正十二面体の頂点座標 (20個)
static const double vertices[20][3] = {
    { 1,  1,  1}, { 1,  1, -1}, { 1, -1,  1}, { 1, -1, -1},
    {-1,  1,  1}, {-1,  1, -1}, {-1, -1,  1}, {-1, -1, -1},
    { 0,  INV_PHI,  PHI}, { 0,  INV_PHI, -PHI}, { 0, -INV_PHI,  PHI}, { 0, -INV_PHI, -PHI},
    { INV_PHI,  PHI, 0}, { INV_PHI, -PHI, 0}, {-INV_PHI,  PHI, 0}, {-INV_PHI, -PHI, 0},
    { PHI, 0,  INV_PHI}, { PHI, 0, -INV_PHI}, {-PHI, 0,  INV_PHI}, {-PHI, 0, -INV_PHI}
};

// 正十二面体の辺のインデックスペア (30本)
static const int edges[30][2] = {
    {0, 8}, {0, 12}, {0, 16},
    {1, 9}, {1, 12}, {1, 17},
    {2, 10}, {2, 13}, {2, 16},
    {3, 11}, {3, 13}, {3, 17},
    {4, 8}, {4, 14}, {4, 18},
    {5, 9}, {5, 14}, {5, 19},
    {6, 10}, {6, 15}, {6, 18},
    {7, 11}, {7, 15}, {7, 19},
    {8, 10}, {9, 11}, {12, 14}, {13, 15}, {16, 17}, {18, 19}
};


// ============================================================
// 弾幕パターン：正十二面陣・虚数結界
// ============================================================
static void ShotDodecahedron(sEnemyShotSet* pEnemyShotSet)
{
    // --- フェーズ1: ノードとエッジの生成 ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // ノード弾（頂点）を20個生成
        for (int v = 0; v < 20; v++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->param_i[0] = 0; // 0:ノード弾
            pShot->param_i[1] = v; // 頂点インデックス
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotLargeBall[3]; // 大玉・シアン

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // エッジ弾（辺）を150個生成 (30辺 × 5個)
        for (int e = 0; e < 30; e++) {
            for (int k = 0; k < 5; k++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->param_i[0] = 1; // 1:エッジ弾
                pShot->param_i[1] = e; // 辺インデックス
                pShot->param_d[0] = k / 4.0; // 辺上の位置 (0.0 ～ 1.0)
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotDiamond[3]; // 菱形弾・シアン

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // --- 3D座標の回転と2D投影計算 ---
    double angY = pEnemyShotSet->count * 0.02;
    double angX = pEnemyShotSet->count * 0.013;

    // フェーズ3以降は回転を停止
    if (pEnemyShotSet->count >= 300) {
        angY = 300 * 0.02;
        angX = 300 * 0.013;
    }

    double nodeX[20], nodeY[20];
    double scale = 60.0; // 画面に収まるようスケール調整

    for (int v = 0; v < 20; v++) {
        double x3d = vertices[v][0], y3d = vertices[v][1], z3d = vertices[v][2];

        // Y軸回転
        double x1 = x3d * cos(angY) - z3d * sin(angY);
        double z1 = x3d * sin(angY) + z3d * cos(angY);
        double y1 = y3d;

        // X軸回転
        double y2 = y1 * cos(angX) - z1 * sin(angX);
        double x2 = x1;

        // 2D投影 (正射影)
        nodeX[v] = x2 * scale + pEnemyShotSet->x;
        nodeY[v] = y2 * scale + pEnemyShotSet->y;
    }

    // --- フェーズ2: エッジに沿ったレーザー発射 ---
    if (pEnemyShotSet->count >= 60 && pEnemyShotSet->count < 300) {
        if (pEnemyShotSet->count % 3 == 0) {
            // ランダムな辺を選んで短レーザーを発射
            for (int i = 0; i < 2; i++) {
                int e = GetRand(29); // 0～29
                int v1 = edges[e][0], v2 = edges[e][1];

                sEnemyShot* pLaser = new sEnemyShot;
                pLaser->param_i[0] = 2; // 2:レーザー
                pLaser->x = nodeX[v1];
                pLaser->y = nodeY[v1];
                pLaser->muki = atan2(nodeY[v2] - nodeY[v1], nodeX[v2] - nodeX[v1]);
                pLaser->speed = 8.0;
                pLaser->kind = img_enemyShotLaser[6]; // 短レーザー・白

                pLaser->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pLaser->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pLaser;
                pEnemyShotSet->pEnemyShotHead->prev = pLaser;
            }
            // パチパチ感のある効果音
            if (!CheckSoundMem(sound_enemyShot_light)) {
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
        }
    }

    // --- フェーズ3: 結界の崩壊と収束 ---
    if (pEnemyShotSet->count == 300) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 30本の辺の方向に極太弾を一斉発射
        for (int e = 0; e < 30; e++) {
            int v1 = edges[e][0], v2 = edges[e][1];

            sEnemyShot* pHeavy = new sEnemyShot;
            pHeavy->param_i[0] = 3; // 3:極太弾
            pHeavy->x = nodeX[v1];
            pHeavy->y = nodeY[v1];
            pHeavy->muki = atan2(nodeY[v2] - nodeY[v1], nodeX[v2] - nodeX[v1]);
            pHeavy->speed = 5.0;
            pHeavy->kind = img_enemyShotMediumOval[5]; // 中楕円弾・マゼンタ

            pHeavy->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pHeavy->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pHeavy;
            pEnemyShotSet->pEnemyShotHead->prev = pHeavy;
        }

        // ノード弾とエッジ弾をプレイヤー方向に散開させて消滅させる
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
                pShot->param_i[0] = 4;
                // GetRand(120) は 0～120 を返すので、-60～60 のブレになる
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x) + (GetRand(120) - 60) / 180.0 * DX_PI;
                pShot->speed = 1.0 + GetRand(10) / 10.0;
                pShot->kind = img_enemyShotMediumBall[5]; // マゼンタの中玉に変化
            }
            pShot = pShot->next;
        }
    }

    // --- 弾の移動・座標更新処理 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // ノード弾：計算済みの投影座標に強制配置
            pShot->x = nodeX[pShot->param_i[1]];
            pShot->y = nodeY[pShot->param_i[1]];
        }
        else if (pShot->param_i[0] == 1) {
            // エッジ弾：繋がる頂点同士を線形補間して強制配置
            int e = pShot->param_i[1];
            double t = pShot->param_d[0];
            int v1 = edges[e][0], v2 = edges[e][1];
            pShot->x = nodeX[v1] + (nodeX[v2] - nodeX[v1]) * t;
            pShot->y = nodeY[v1] + (nodeY[v2] - nodeY[v1]) * t;
        }
        else {
            // レーザー・極太弾・散開後のノード・エッジ：通常の等速直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}


// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Dodecahedron_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 120.0; // 投影範囲を見やすくするため少し下げる
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 400フレーム周期で正十二面体弾幕を展開
    if (count % 400 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDodecahedron;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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