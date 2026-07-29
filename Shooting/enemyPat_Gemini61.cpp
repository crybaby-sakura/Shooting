// enemyPat_Dodecahedron.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

const double SCALE = 280;

const double PHI = 1.618033988749895;
const double INV_PHI = 0.618033988749895;

const double L_F = 1.902113032590307;
const double F0 = 0.0;
const double F1 = 1.0 / L_F;
const double FP = PHI / L_F;

// 12の面の法線ベクトル
static const double d_faces[12][3] = {
    {F0, F1, FP}, {F0, F1, -FP}, {F0, -F1, FP}, {F0, -F1, -FP},
    {F1, FP, F0}, {F1, -FP, F0}, {-F1, FP, F0}, {-F1, -FP, F0},
    {FP, F0, F1}, {FP, F0, -F1}, {-FP, F0, F1}, {-FP, F0, -F1}
};

const double L_V = 1.732050807568877;
const double V1 = 1.0 / L_V;
const double VIP = INV_PHI / L_V;
const double VP = PHI / L_V;
const double V0 = 0.0;

// 20の頂点ベクトル
static const double d_vertices[20][3] = {
    { V1,  V1,  V1}, { V1,  V1, -V1}, { V1, -V1,  V1}, { V1, -V1, -V1},
    {-V1,  V1,  V1}, {-V1,  V1, -V1}, {-V1, -V1,  V1}, {-V1, -V1, -V1},
    { V0, VIP,  VP}, { V0, VIP, -VP}, { V0,-VIP,  VP}, { V0,-VIP, -VP},
    {VIP,  VP,  V0}, {VIP, -VP,  V0}, {-VIP,  VP,  V0}, {-VIP, -VP,  V0},
    { VP,  V0, VIP}, { VP,  V0,-VIP}, {-VP,  V0, VIP}, {-VP,  V0,-VIP}
};

// 3D回転用関数
static void Rotate3D(double& x, double& y, double& z, double rx, double ry, double rz) {
    double tmpY = y * cos(rx) - z * sin(rx);
    double tmpZ = y * sin(rx) + z * cos(rx);
    y = tmpY; z = tmpZ;

    double tmpX = x * cos(ry) + z * sin(ry);
    tmpZ = -x * sin(ry) + z * cos(ry);
    x = tmpX; z = tmpZ;

    tmpX = x * cos(rz) - y * sin(rz);
    tmpY = x * sin(rz) + y * cos(rz);
    x = tmpX; y = tmpY;
}

// 弾幕：黄金正多面体陣 -Dodecahedron-
static void ShotDodecahedron(sEnemyShotSet* pSet)
{
    int t = pSet->count;
    int local_t = t % 400;

    // 3D回転角度 (時間経過とともに回転)
    double rx = t * 0.015 / 2;
    double ry = t * 0.011 / 2;
    double rz = t * 0.007 / 2;

    // 予告音
    if (local_t == 0 || local_t == 240) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // Phase 1: 骨格の形成（1フレーム目にワイヤーフレームとなる弾を生成）
    if (local_t == 1) {
        for (int i = 0; i < 20; i++) {
            // 頂点に中玉を配置
            sEnemyShot* pVShot = new sEnemyShot;
            pVShot->speed = 0;
            pVShot->param_i[0] = 1; // ワイヤーフレーム弾フラグ
            pVShot->param_d[0] = d_vertices[i][0];
            pVShot->param_d[1] = d_vertices[i][1];
            pVShot->param_d[2] = d_vertices[i][2];
            pVShot->kind = img_enemyShotMediumBall[4];
            pVShot->margin = 240;

            pVShot->prev = pSet->pEnemyShotHead->prev;
            pVShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pVShot;
            pSet->pEnemyShotHead->prev = pVShot;

            // 頂点間を計算して「辺」を検出し、間に小玉を配置する
            for (int j = i + 1; j < 20; j++) {
                double dx = d_vertices[i][0] - d_vertices[j][0];
                double dy = d_vertices[i][1] - d_vertices[j][1];
                double dz = d_vertices[i][2] - d_vertices[j][2];
                double distSq = dx * dx + dy * dy + dz * dz;

                // 距離の2乗が約0.509なら隣接する頂点（辺）とみなす
                if (distSq < 0.6) {
                    for (int k = 1; k <= 5; k++) { // 頂点間に3発の小玉を並べる
                        double lerp = k / 6.0;
                        double lx = d_vertices[i][0] * (1.0 - lerp) + d_vertices[j][0] * lerp;
                        double ly = d_vertices[i][1] * (1.0 - lerp) + d_vertices[j][1] * lerp;
                        double lz = d_vertices[i][2] * (1.0 - lerp) + d_vertices[j][2] * lerp;

                        sEnemyShot* pEdgeShot = new sEnemyShot;
                        pEdgeShot->speed = 0;
                        pEdgeShot->param_i[0] = 1; // ワイヤーフレーム弾フラグ
                        pEdgeShot->param_d[0] = lx;
                        pEdgeShot->param_d[1] = ly;
                        pEdgeShot->param_d[2] = lz;
                        pEdgeShot->kind = img_enemyShotSmallBall[4];
                        pEdgeShot->margin = 240;

                        pEdgeShot->prev = pSet->pEnemyShotHead->prev;
                        pEdgeShot->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = pEdgeShot;
                        pSet->pEnemyShotHead->prev = pEdgeShot;
                    }
                }
            }
        }
    }

    // Phase 2: 面からの五角形パルス (60〜239フレームまで、30フレーム毎)
    //if (local_t >= 60 && local_t < 240) {
    //    if (local_t % 30 == 0) {
    //        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
    //        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

    //        for (int i = 0; i < 12; i++) {
    //            double nx = d_faces[i][0];
    //            double ny = d_faces[i][1];
    //            double nz = d_faces[i][2];

    //            // 法線に対する直交基底 u, v を計算
    //            double ux, uy, uz;
    //            if (fabs(ny) < 0.9) {
    //                ux = nz; uy = 0; uz = -nx;
    //            }
    //            else {
    //                ux = 0; uy = nz; uz = -ny;
    //            }
    //            double ulen = sqrt(ux * ux + uy * uy + uz * uz);
    //            ux /= ulen; uy /= ulen; uz /= ulen;

    //            double vx = ny * uz - nz * uy;
    //            double vy = nz * ux - nx * uz;
    //            double vz = nx * uy - ny * ux;

    //            // 五角形を成す5発の弾を生成
    //            for (int j = 0; j < 5; j++) {
    //                double angle = j * (DX_PI * 2.0 / 5.0) + (t * 0.02);
    //                double r = 0.4;

    //                double px = nx + ux * r * cos(angle) + vx * r * sin(angle);
    //                double py = ny + uy * r * cos(angle) + vy * r * sin(angle);
    //                double pz = nz + uz * r * cos(angle) + vz * r * sin(angle);
    //                Rotate3D(px, py, pz, rx, ry, rz);

    //                double scale = SCALE; // 骨格の内側から発射
    //                double emitX = pSet->x + px * scale;
    //                double emitY = pSet->y + py * scale;

    //                double dx = nx + ux * (r * 0.2) * cos(angle) + vx * (r * 0.2) * sin(angle);
    //                double dy = ny + uy * (r * 0.2) * cos(angle) + vy * (r * 0.2) * sin(angle);
    //                double dz = nz + uz * (r * 0.2) * cos(angle) + vz * (r * 0.2) * sin(angle);
    //                Rotate3D(dx, dy, dz, rx, ry, rz);
    //                double muki = atan2(dy, dx);

    //                int color = (pz > 0) ? 3 : 4; // 3:シアン(手前), 4:青(奥)
    //                double spd = (pz > 0) ? 2.5 : 1.5;

    //                sEnemyShot* pShot = new sEnemyShot;
    //                pShot->x = emitX;
    //                pShot->y = emitY;
    //                pShot->muki = muki;
    //                pShot->speed = spd;
    //                pShot->kind = img_enemyShotDiamond[color];

    //                pShot->prev = pSet->pEnemyShotHead->prev;
    //                pShot->next = pSet->pEnemyShotHead;
    //                pSet->pEnemyShotHead->prev->next = pShot;
    //                pSet->pEnemyShotHead->prev = pShot;
    //            }
    //        }
    //    }
    //}

    // Phase 3: 20頂点からの自機狙い＆ワイヤーフレームの爆発拡散 (300フレーム目)
    if (local_t == 300) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 自機狙い弾の生成
        for (int i = 0; i < 20; i++) {
            double px = d_vertices[i][0];
            double py = d_vertices[i][1];
            double pz = d_vertices[i][2];
            Rotate3D(px, py, pz, rx, ry, rz);

            double scale = SCALE;
            double emitX = pSet->x + px * scale;
            double emitY = pSet->y + py * scale;
            double targetMuki = atan2(player.y - emitY, player.x - emitX);

            for (int j = -1; j <= 1; j++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = emitX;
                pShot->y = emitY;
                pShot->muki = targetMuki + j * 0.15;
                pShot->speed = 3.0 + GetRand(10) * 0.05;
                pShot->kind = img_enemyShotMediumBall[0]; // 赤中玉

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }

        // 骨格を形成していたワイヤーフレーム弾を白玉にして拡散させる
        sEnemyShot* pExp = pSet->pEnemyShotHead->next;
        while (pExp != pSet->pEnemyShotHead) {
            if (pExp->param_i[0] == 1) {
                pExp->param_i[0] = 0; // 拘束を解除
                pExp->speed = 1.0 + GetRand(20) / 10.0;
                pExp->muki = atan2(pExp->y - pSet->y, pExp->x - pSet->x);

                // 色を白玉に変更して散らす
                if (pExp->kind == img_enemyShotMediumBall[4] || pExp->kind == img_enemyShotMediumBall[3]) {
                    pExp->kind = img_enemyShotMediumBall[6];
                }
                else {
                    pExp->kind = img_enemyShotSmallBall[6];
                }
            }
            pExp = pExp->next;
        }
    }

    // 既存の弾の移動処理 兼 ワイヤーフレーム弾の座標更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // ワイヤーフレーム拘束中の弾は、3D回転に応じて座標を強制上書き
            double px = pShot->param_d[0];
            double py = pShot->param_d[1];
            double pz = pShot->param_d[2];
            Rotate3D(px, py, pz, rx, ry, rz);

            double scale = SCALE; // 骨格の大きさ
            pShot->x = pSet->x + px * scale;
            pShot->y = pSet->y + py * scale;

            // 手前にある弾は明るく(シアン)、奥にある弾は暗く(青)見せる
            if (pShot->kind == img_enemyShotMediumBall[4] || pShot->kind == img_enemyShotMediumBall[3]) {
                pShot->kind = (pz > 0) ? img_enemyShotMediumBall[3] : img_enemyShotMediumBall[4];
            }
            else if (pShot->kind == img_enemyShotSmallBall[4] || pShot->kind == img_enemyShotSmallBall[3]) {
                pShot->kind = (pz > 0) ? img_enemyShotSmallBall[3] : img_enemyShotSmallBall[4];
            }
        }
        else {
            // 通常の弾（射出されたもの）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Dodecahedron_Gemini()
{
    static int move_muki;

    if (count == 1) {
        // ゲーム画面は 480x480 を想定、上部中央へ
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
        move_muki = 1;

        // 弾幕セットの登録 (1度だけ登録し、破棄せず使い続ける)
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotDodecahedron;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        // 敵はゆっくり左右に揺れる
        enemy.x += 0.5 * (double)move_muki;
        if (count % 180 == 90) move_muki *= -1;

        // 弾幕セットの基点を敵に追従させる
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        if (pSet != &enemyShotSetHead) {
            pSet->x = enemy.x;
            pSet->y = enemy.y;
        }
    }
}