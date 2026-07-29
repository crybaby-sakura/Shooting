// enemyPat_Tmp.cpp
// 正十二面体ワイヤフレーム描画弾幕
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 正十二面体データ（中心原点・辺長 2/φ）
// ============================================================
static const double PHI = 1.618033988749895; // (1+√5)/2

// 20頂点
static const double DODECA_VERT[20][3] = {
    {-1.0, -1.0, -1.0}, // 0
    {-1.0, -1.0,  1.0}, // 1
    {-1.0,  1.0, -1.0}, // 2
    {-1.0,  1.0,  1.0}, // 3
    { 1.0, -1.0, -1.0}, // 4
    { 1.0, -1.0,  1.0}, // 5
    { 1.0,  1.0, -1.0}, // 6
    { 1.0,  1.0,  1.0}, // 7
    { 0.0, -1.0 / PHI, -PHI}, // 8
    { 0.0, -1.0 / PHI,  PHI}, // 9
    { 0.0,  1.0 / PHI, -PHI}, // 10
    { 0.0,  1.0 / PHI,  PHI}, // 11
    {-1.0 / PHI, -PHI,  0.0}, // 12
    {-1.0 / PHI,  PHI,  0.0}, // 13
    { 1.0 / PHI, -PHI,  0.0}, // 14
    { 1.0 / PHI,  PHI,  0.0}, // 15
    {-PHI,  0.0, -1.0 / PHI}, // 16
    {-PHI,  0.0,  1.0 / PHI}, // 17
    { PHI,  0.0, -1.0 / PHI}, // 18
    { PHI,  0.0,  1.0 / PHI}  // 19
};

// 30辺（頂点インデックスのペア）
static const int DODECA_EDGE[30][2] = {
    {0, 8}, {0, 12}, {0, 16},
    {1, 9}, {1, 12}, {1, 17},
    {2, 10}, {2, 13}, {2, 16},
    {3, 11}, {3, 13}, {3, 17},
    {4, 8}, {4, 14}, {4, 18},
    {5, 9}, {5, 14}, {5, 19},
    {6, 10}, {6, 15}, {6, 18},
    {7, 11}, {7, 15}, {7, 19},
    {8, 10}, {9, 11},
    {12, 14}, {13, 15},
    {16, 17}, {18, 19}
};

// 3D回転（X軸→Y軸の順）後にXYを返す簡易投影
static void RotateProject(
    double x, double y, double z,
    double rx, double ry,
    double* outX, double* outY, double* outZ)
{
    // X軸回転
    double cosx = cos(rx), sinx = sin(rx);
    double y1 = y * cosx - z * sinx;
    double z1 = y * sinx + z * cosx;
    // Y軸回転
    double cosy = cos(ry), siny = sin(ry);
    double x2 = x * cosy + z1 * siny;
    double z2 = -x * siny + z1 * cosy;
    *outX = x2;
    *outY = y1;
    *outZ = z2;
}

// ============================================================
// 弾幕パターン本体
// フェーズ:
//   0〜29  : 辺を順に描画（構築）
//  30〜89  : 完成したワイヤーフレームを保持＋ゆっくり回転
//  90〜    : 弾を外側へ拡散＋頂点弾追加
// ============================================================
static void ShotDodecaWire(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int BUILD_FRAMES = 30;   // 辺を描く時間
    const int HOLD_FRAMES = 60;   // 保持時間
    const int EDGE_BULLETS = 9;    // 1辺あたりの弾数（両端含む）

    // ---- 初期化（最初の1フレーム） ----
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // param_d の使い方
        // [0]=中心X [1]=中心Y [2]=スケール
        // [3]=回転X [4]=回転Y [5]=回転速度X [6]=回転速度Y
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
        pEnemyShotSet->param_d[2] = 95.0;               // サイズ
        pEnemyShotSet->param_d[3] = 0.35;               // 初期傾きX
        pEnemyShotSet->param_d[4] = 0.55;               // 初期傾きY
        pEnemyShotSet->param_d[5] = 0.012;              // 回転速度X
        pEnemyShotSet->param_d[6] = 0.018;              // 回転速度Y

        // kind で色を少し変える（呼び出し側で加算）
        pEnemyShotSet->param_i[0] = pEnemyShotSet->kind % 9; // 基本色
    }

    double cx = pEnemyShotSet->param_d[0];
    double cy = pEnemyShotSet->param_d[1];
    double scale = pEnemyShotSet->param_d[2];
    double rx = pEnemyShotSet->param_d[3];
    double ry = pEnemyShotSet->param_d[4];
    int baseColor = pEnemyShotSet->param_i[0];

    // ---- 構築フェーズ：辺を順番に弾で描く ----
    if (pEnemyShotSet->count < BUILD_FRAMES) {
        // 1フレームあたり約1辺描画
        int edgeIdx = pEnemyShotSet->count;
        if (edgeIdx < 30) {
            int v0 = DODECA_EDGE[edgeIdx][0];
            int v1 = DODECA_EDGE[edgeIdx][1];

            double x0, y0, z0, x1, y1, z1;
            RotateProject(DODECA_VERT[v0][0], DODECA_VERT[v0][1], DODECA_VERT[v0][2], rx, ry, &x0, &y0, &z0);
            RotateProject(DODECA_VERT[v1][0], DODECA_VERT[v1][1], DODECA_VERT[v1][2], rx, ry, &x1, &y1, &z1);

            for (int i = 0; i < EDGE_BULLETS; i++) {
                double t = (double)i / (EDGE_BULLETS - 1);
                double px = x0 + (x1 - x0) * t;
                double py = y0 + (y1 - y0) * t;
                double pz = z0 + (z1 - z0) * t;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx + px * scale;
                pEnemyShot->y = cy + py * scale;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0; // 静止
                // 辺弾は小さめの玉で線を明確に
                pEnemyShot->kind = img_enemyShotSmallBall[(baseColor + 3) % 9]; // シアン寄り
                // param_i[0] = フェーズ用フラグ（0=辺弾）
                // param_d[0,1] = 正規化座標（拡散方向計算用）
                pEnemyShot->param_i[0] = 0;
                pEnemyShot->param_d[0] = px;
                pEnemyShot->param_d[1] = py;
                pEnemyShot->param_d[2] = pz;
                pEnemyShot->margin = 200;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    // ---- 保持フェーズ：回転しながら形を維持 ----
    else if (pEnemyShotSet->count < BUILD_FRAMES + HOLD_FRAMES) {
        // 回転角度を更新
        pEnemyShotSet->param_d[3] += pEnemyShotSet->param_d[5];
        pEnemyShotSet->param_d[4] += pEnemyShotSet->param_d[6];
        rx = pEnemyShotSet->param_d[3];
        ry = pEnemyShotSet->param_d[4];

        // 既存の辺弾の位置を再計算して回転させる
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) { // 辺弾のみ
                double ox = pShot->param_d[0];
                double oy = pShot->param_d[1];
                double oz = pShot->param_d[2];
                // 微小回転を適用して位置を更新
                double dRx = pEnemyShotSet->param_d[5];
                double dRy = pEnemyShotSet->param_d[6];
                double cosx = cos(dRx), sinx = sin(dRx);
                double y1 = oy * cosx - oz * sinx;
                double z1 = oy * sinx + oz * cosx;
                double cosy = cos(dRy), siny = sin(dRy);
                double x2 = ox * cosy + z1 * siny;
                double z2 = -ox * siny + z1 * cosy;
                pShot->param_d[0] = x2;
                pShot->param_d[1] = y1;
                pShot->param_d[2] = z2;
                pShot->x = cx + x2 * scale;
                pShot->y = cy + y1 * scale;
            }
            pShot = pShot->next;
        }
    }
    // ---- 拡散フェーズ開始時に速度を与える ----
    else if (pEnemyShotSet->count == BUILD_FRAMES + HOLD_FRAMES) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 辺弾に放射状の速度を付与
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                double dx = pShot->param_d[0];
                double dy = pShot->param_d[1];
                double len = sqrt(dx * dx + dy * dy);
                if (len < 0.001) len = 0.001;
                pShot->muki = atan2(dy, dx);
                // 外側ほど少し速い
                pShot->speed = 1.8 + len * 1.2;
            }
            pShot = pShot->next;
        }

        // 頂点から追加弾を発射（プレイヤー狙い寄り）
        double aim = atan2(player.y - cy, player.x - cx);
        for (int v = 0; v < 20; v++) {
            double vx, vy, vz;
            RotateProject(DODECA_VERT[v][0], DODECA_VERT[v][1], DODECA_VERT[v][2],
                pEnemyShotSet->param_d[3], pEnemyShotSet->param_d[4],
                &vx, &vy, &vz);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + vx * scale;
            pEnemyShot->y = cy + vy * scale;
            // 頂点から少し散らした狙い
            pEnemyShot->muki = aim + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.4 + GetRand(80) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumBall[(baseColor + 1) % 9]; // 少し目立つ色
            pEnemyShot->param_i[0] = 1; // 頂点弾フラグ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---- 毎フレーム：弾を移動 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->speed > 0.0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_Dodecahedron_Grok()
{
    static int phase = 0;
    static int shot_count = 0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        shot_count = 0;
    }
    else {
        // 緩やかに左右移動
        if (count % 180 < 90) {
            enemy.x += 0.6;
        }
        else {
            enemy.x -= 0.6;
        }
        // 画面端で反転しないよう制限
        if (enemy.x < 80.0) enemy.x = 80.0;
        if (enemy.x > 400.0) enemy.x = 400.0;
    }

    // 一定間隔で正十二面体弾幕を発動
    // 最初は早め、以降は間隔を空ける
    int interval = 150;
    if (count == 30 || (count > 30 && (count - 30) % interval == 0)) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDodecaWire;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 40.0; // 敵の少し下を中心に
        pEnemyShotSet->muki = 0.0;
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