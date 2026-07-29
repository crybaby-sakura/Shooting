// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：正十二面体の鏡像（ミラーイメージ）
static void ShotDodecahedron(sEnemyShotSet* pEnemyShotSet)
{
    // 黄金比
    const double phi = (1.0 + sqrt(5.0)) / 2.0;
    const double inv_phi = 1.0 / phi;

    // 正十二面体の20個の頂点座標 (スケール前)
    const double v[20][3] = {
        { 1,  1,  1}, { 1,  1, -1}, { 1, -1,  1}, { 1, -1, -1},
        {-1,  1,  1}, {-1,  1, -1}, {-1, -1,  1}, {-1, -1, -1},
        { 0,  inv_phi,  phi}, { 0,  inv_phi, -phi}, { 0, -inv_phi,  phi}, { 0, -inv_phi, -phi},
        { inv_phi,  phi, 0}, { inv_phi, -phi, 0}, {-inv_phi,  phi, 0}, {-inv_phi, -phi, 0},
        { phi, 0,  inv_phi}, { phi, 0, -inv_phi}, {-phi, 0,  inv_phi}, {-phi, 0, -inv_phi}
    };

    // 正十二面体の頂点隣接リスト (幾何学的に正しい全30本の辺を網羅)
    const int adj[20][3] = {
        {8, 12, 16}, // 0
        {9, 12, 17}, // 1
        {10, 13, 16}, // 2
        {11, 13, 17}, // 3
        {8, 14, 18}, // 4
        {9, 14, 19}, // 5
        {10, 15, 18}, // 6
        {11, 15, 19}, // 7
        {0, 4, 10},  // 8
        {1, 5, 11},  // 9
        {2, 6, 8},   // 10
        {3, 7, 9},   // 11
        {0, 1, 14},  // 12
        {2, 3, 15},  // 13
        {4, 5, 12},  // 14
        {6, 7, 13},  // 15
        {0, 2, 17},  // 16
        {1, 3, 16},  // 17
        {4, 6, 19},  // 18
        {5, 7, 18}   // 19
    };

    // 回転角度 (kind=0:通常, kind=1:高速)
    double angle_speed = (pEnemyShotSet->kind == 0) ? 0.01 : 0.02;
    double angle = pEnemyShotSet->count * angle_speed;
    double cos_x = cos(angle), sin_x = sin(angle);
    double cos_y = cos(angle * 0.7), sin_y = sin(angle * 0.7);

    // 頂点の2D座標と奥行き(Z)を計算
    double px[20], py[20], pz[20];
    int front_vertex = 0;
    double max_pz = -9999.0;

    for (int i = 0; i < 20; i++) {
        double x = v[i][0] * 80.0; // スケール
        double y = v[i][1] * 80.0;
        double z = v[i][2] * 80.0;

        // Y軸回転
        double rx = x * cos_y - z * sin_y;
        double rz = x * sin_y + z * cos_y;
        double ry = y;

        // X軸回転
        double ry2 = ry * cos_x - rz * sin_x;
        double rz2 = ry * sin_x + rz * cos_x;

        // 透視投影 (視点距離 400)
        double scale = 400.0 / (400.0 + rz2);
        px[i] = pEnemyShotSet->x + rx * scale;
        py[i] = pEnemyShotSet->y + ry2 * scale;
        pz[i] = rz2;

        // 最前面の頂点を記録（狙い撃ち用に使用）
        if (pz[i] > max_pz) {
            max_pz = pz[i];
            front_vertex = i;
        }
    }

    // 1. 枠組みの軌跡弾（辺の表現）
    // 方向性のある「鱗弾」を頂点から隣接頂点の方向へ連続発射することで、
    // 回転する正十二面体の辺が帯状（点線）に残るような視覚効果を生む。
    if (pEnemyShotSet->count % 2 == 0 && pEnemyShotSet->count % 80 < 20) {
        if (pEnemyShotSet->count % 4 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        for (int i = 0; i < 20; i++) {
            for (int k = 0; k < 3; k++) {
                int j = adj[i][k];

                // 頂点iから隣接頂点jへの方向を計算
                double dx = px[j] - px[i];
                double dy = py[j] - py[i];
                double muki = atan2(dy, dx);

                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = px[i];
                pShot->y = py[i];
                pShot->muki = muki;
                pShot->speed = 3.0; // 辺をなぞるような速度
                pShot->margin = 100;

                // 奥行きによる色分けで立体感を強調
                if (pz[i] > 0.0) {
                    pShot->kind = img_enemyShotScale[1]; // 手前: 黄色の鱗弾
                }
                else {
                    pShot->kind = img_enemyShotScale[4]; // 奥: 青色の鱗弾
                }

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 2. 最前面の頂点からの狙い撃ち弾（枠組みの隙間を狭める）
    if (pEnemyShotSet->count % 30 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* pAim = new sEnemyShot;
        pAim->x = px[front_vertex];
        pAim->y = py[front_vertex];
        // GetRand(10)は0〜10を返すため、-5〜5の範囲でわずかなばらつきを持たせる
        pAim->muki = atan2(player.y - pAim->y, player.x - pAim->x) + (GetRand(10) - 5) / 180.0 * DX_PI;
        pAim->speed = 3.5;
        pAim->kind = img_enemyShotMediumBall[0]; // 赤い中玉

        pAim->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pAim->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pAim;
        pEnemyShotSet->pEnemyShotHead->prev = pAim;
    }

    // 既存の弾の移動処理
    // (画面外消去とcountのインクリメントはメインルーチンで行われる仕様)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Dodecahedron_Qwen()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // 正十二面体弾幕セットの初期化
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDodecahedron;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0;
        pEnemyShotSet->kind = 0; // 0: 通常回転, 1: 高速回転

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵の移動: 左右にゆっくり往復
        enemy.x += 0.4 * (double)muki;
        if (count % 180 == 90) muki *= -1;

        // 弾幕セットの位置を敵に追従させ、フェーズ管理を行う
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == ShotDodecahedron) {
                pSet->x = enemy.x;
                pSet->y = enemy.y + 10.0;

                // フェーズ変化の演出 (count 600 で予告音と高速回転へ)
                if (count == 600 && pSet->kind == 0) {
                    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
                    pSet->kind = 1;
                }
            }
            pSet = pSet->next;
        }
    }
}