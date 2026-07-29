// enemyPat_Tmp.cpp
// 正十二面体をモチーフにした弾幕「ドデカヘドラル・ケージ」
// 敵本体関数：void EnemyPat_Dodecahedron_DeepSeek()

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 定数
static const double DODEC_RADIUS = 250.0;         // 正十二面体の半径
static const int    EDGE_BULLETS = 6;             // 各辺の弾数
static const double PHI = (1.0 + sqrt(5.0)) / 2.0; // 黄金比

// ------------------------------------------------------------------
// 前方宣言
// ------------------------------------------------------------------
static void CagePattern(sEnemyShotSet* pSet);
static void FaceBurst(sEnemyShotSet* pSet);
static void EdgeExplosion(sEnemyShotSet* pSet);

// ------------------------------------------------------------------
// 3次元ベクトル演算（簡易）
// ------------------------------------------------------------------
struct Vec3 { double x, y, z; };

static Vec3 vec3(double x, double y, double z) { Vec3 v = { x, y, z }; return v; }

static Vec3 normalize(const Vec3& v) {
    double len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len == 0) return v;
    return { v.x / len, v.y / len, v.z / len };
}

static Vec3 rotateX(const Vec3& v, double angle) {
    double c = cos(angle), s = sin(angle);
    return { v.x, v.y * c - v.z * s, v.y * s + v.z * c };
}
static Vec3 rotateY(const Vec3& v, double angle) {
    double c = cos(angle), s = sin(angle);
    return { v.x * c + v.z * s, v.y, -v.x * s + v.z * c };
}
static Vec3 rotateZ(const Vec3& v, double angle) {
    double c = cos(angle), s = sin(angle);
    return { v.x * c - v.y * s, v.x * s + v.y * c, v.z };
}

// ------------------------------------------------------------------
// ケージ本体の処理（回転するワイヤーフレーム＋面放射＋辺炸裂）
// ------------------------------------------------------------------
static void CagePattern(sEnemyShotSet* pSet)
{
    // ---- 形状データ（静的） ----
    static bool geomReady = false;
    static Vec3 dodecVertices[20];   // 正十二面体の頂点（正規化済み×半径）
    static Vec3 icosaVertices[12];   // 正二十面体の頂点＝正十二面体の面中心
    static int  edges[30][2];        // 辺（頂点インデックス対）

    if (pSet->count == 0) {
        // 正十二面体の頂点（±1,±1,±1）＋（0,±φ,±1/φ）＋…
        double rawVerts[20][3] = {
            { 1, 1, 1}, { 1, 1,-1}, { 1,-1, 1}, { 1,-1,-1},
            {-1, 1, 1}, {-1, 1,-1}, {-1,-1, 1}, {-1,-1,-1},
            { 0, PHI, 1.0 / PHI}, { 0, PHI, -1.0 / PHI}, { 0, -PHI, 1.0 / PHI}, { 0, -PHI, -1.0 / PHI},
            { 1.0 / PHI, 0, PHI}, { 1.0 / PHI, 0, -PHI}, { -1.0 / PHI, 0, PHI}, { -1.0 / PHI, 0, -PHI},
            { PHI, 1.0 / PHI, 0}, { PHI, -1.0 / PHI, 0}, { -PHI, 1.0 / PHI, 0}, { -PHI, -1.0 / PHI, 0}
        };
        for (int i = 0; i < 20; ++i) {
            Vec3 v = { rawVerts[i][0], rawVerts[i][1], rawVerts[i][2] };
            dodecVertices[i] = vec3(v.x * DODEC_RADIUS, v.y * DODEC_RADIUS, v.z * DODEC_RADIUS);
            dodecVertices[i] = normalize(dodecVertices[i]);
            dodecVertices[i] = vec3(dodecVertices[i].x * DODEC_RADIUS, dodecVertices[i].y * DODEC_RADIUS, dodecVertices[i].z * DODEC_RADIUS);
        }

        // 正二十面体の頂点（面中心として使う）
        double icoRaw[12][3] = {
            { 0, 1, PHI}, { 0, 1, -PHI}, { 0, -1, PHI}, { 0, -1, -PHI},
            { PHI, 0, 1}, { PHI, 0, -1}, { -PHI, 0, 1}, { -PHI, 0, -1},
            { 1, PHI, 0}, { 1, -PHI, 0}, { -1, PHI, 0}, { -1, -PHI, 0}
        };
        for (int i = 0; i < 12; ++i) {
            Vec3 v = { icoRaw[i][0], icoRaw[i][1], icoRaw[i][2] };
            v = normalize(v);
            icosaVertices[i] = vec3(v.x * DODEC_RADIUS, v.y * DODEC_RADIUS, v.z * DODEC_RADIUS);
        }

        // 辺のリスト作成
        double edgeLen2 = 4.0 / (PHI + 1.0);  // (2/φ)^2
        int edgeIdx = 0;
        for (int i = 0; i < 20; ++i) {
            for (int j = i + 1; j < 20; ++j) {
                double dx = rawVerts[i][0] - rawVerts[j][0];
                double dy = rawVerts[i][1] - rawVerts[j][1];
                double dz = rawVerts[i][2] - rawVerts[j][2];
                double d2 = dx * dx + dy * dy + dz * dz;
                if (fabs(d2 - edgeLen2) < 0.001) {
                    edges[edgeIdx][0] = i;
                    edges[edgeIdx][1] = j;
                    ++edgeIdx;
                }
            }
        }
        geomReady = true;
    }

    // ---- 静的な弾ポインタ（頂点弾と辺弾） ----
    static sEnemyShot* vertexBullets[20] = {};
    static sEnemyShot* edgeBullets[30][EDGE_BULLETS] = {};

    // 初期化（最初のフレーム）
    if (pSet->count == 0) {
        // 頂点弾（青い大玉、速度0）
        for (int i = 0; i < 20; ++i) {
            sEnemyShot* s = new sEnemyShot;
            s->kind = img_enemyShotLargeBall[4];  // 青
            s->speed = 0;
            s->margin = 240;
            s->prev = pSet->pEnemyShotHead->prev;
            s->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = s;
            pSet->pEnemyShotHead->prev = s;
            vertexBullets[i] = s;
        }
        // 辺弾（赤い小玉、速度0）
        for (int e = 0; e < 30; ++e) {
            for (int j = 0; j < EDGE_BULLETS; ++j) {
                sEnemyShot* s = new sEnemyShot;
                s->kind = img_enemyShotSmallBall[0]; // 赤
                s->speed = 0;
                s->margin = 240;
                s->prev = pSet->pEnemyShotHead->prev;
                s->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = s;
                pSet->pEnemyShotHead->prev = s;
                edgeBullets[e][j] = s;
            }
        }
    }

    // ---- 回転とスケールの更新 ----
    static double rotX = 0.0, rotY = 0.0, rotZ = 0.0;
    if (pSet->count == 0) {
        rotX = rotY = rotZ = 0.0;
    }
    rotX += 0.018 / 3;
    rotY += 0.025 / 3;
    rotZ += 0.012 / 3;

    double scale = 1.0;
    static int T0 = -1;
    if (pSet->count == 0) {
        T0 = -1;
    }
    if (enemy.hp < enemy.maxHp / 2) {
        if (T0 == -1) {
            T0 = count;
        }
        scale = 1.0 + 0.25 * sin((count - T0) * 0.04);  // 収縮・膨張
    }

    // 全頂点の回転後座標を計算
    Vec3 rotV[20];
    for (int i = 0; i < 20; ++i) {
        Vec3 v = vec3(dodecVertices[i].x * scale, dodecVertices[i].y * scale, dodecVertices[i].z * scale);
        v = rotateX(v, rotX);
        v = rotateY(v, rotY);
        v = rotateZ(v, rotZ);
        rotV[i] = v;
    }

    // 頂点弾の位置更新
    for (int i = 0; i < 20; ++i) {
        vertexBullets[i]->x = enemy.x + rotV[i].x;
        vertexBullets[i]->y = enemy.y + rotV[i].y;
    }

    // 辺弾の位置更新
    for (int e = 0; e < 30; ++e) {
        int i0 = edges[e][0];
        int i1 = edges[e][1];
        double x0 = enemy.x + rotV[i0].x;
        double y0 = enemy.y + rotV[i0].y;
        double x1 = enemy.x + rotV[i1].x;
        double y1 = enemy.y + rotV[i1].y;
        for (int j = 0; j < EDGE_BULLETS; ++j) {
            double t = (j + 0.5) / (double)EDGE_BULLETS;
            edgeBullets[e][j]->x = x0 + (x1 - x0) * t;
            edgeBullets[e][j]->y = y0 + (y1 - y0) * t;
        }
    }

    // ---- 面放射フェーズ（一定間隔で可視面から発射） ----
    //if (pSet->count % 150 == 60) {   // 2.5秒ごと（60fps想定）
    //    for (int f = 0; f < 12; ++f) {
    //        Vec3 v = vec3(icosaVertices[f].x * scale, icosaVertices[f].y * scale, icosaVertices[f].z * scale);
    //        v = rotateX(v, rotX);
    //        v = rotateY(v, rotY);
    //        v = rotateZ(v, rotZ);
    //        if (v.z > 0.0) {   // 手前を向いている面のみ
    //            double fx = enemy.x + v.x;
    //            double fy = enemy.y + v.y;
    //            double nx = v.x, ny = v.y;
    //            double len = sqrt(nx * nx + ny * ny);
    //            if (len > 0.0) { nx /= len; ny /= len; }
    //            double angle = atan2(ny, nx);

    //            sEnemyShotSet* pNew = new sEnemyShotSet;
    //            pNew->count = 0;
    //            pNew->patternFunc = FaceBurst;
    //            pNew->x = fx;
    //            pNew->y = fy;
    //            pNew->muki = angle;
    //            pNew->kind = 0;
    //            pNew->pEnemyShotHead = new sEnemyShot;
    //            pNew->pEnemyShotHead->prev = pNew->pEnemyShotHead;
    //            pNew->pEnemyShotHead->next = pNew->pEnemyShotHead;
    //            pNew->prev = enemyShotSetHead.prev;
    //            pNew->next = &enemyShotSetHead;
    //            enemyShotSetHead.prev->next = pNew;
    //            enemyShotSetHead.prev = pNew;
    //        }
    //    }
    //}

    // ---- 辺炸裂フェーズ（一定間隔でランダムな辺を炸裂） ----
    //if (pSet->count % 280 == 140) {
    //    for (int k = 0; k < 5; ++k) {
    //        int e = GetRand(29); // 0..29
    //        int i0 = edges[e][0];
    //        int i1 = edges[e][1];
    //        double mx = enemy.x + (rotV[i0].x + rotV[i1].x) * 0.5;
    //        double my = enemy.y + (rotV[i0].y + rotV[i1].y) * 0.5;

    //        sEnemyShotSet* pNew = new sEnemyShotSet;
    //        pNew->count = 0;
    //        pNew->patternFunc = EdgeExplosion;
    //        pNew->x = mx;
    //        pNew->y = my;
    //        pNew->muki = 0;
    //        pNew->kind = 0;
    //        pNew->pEnemyShotHead = new sEnemyShot;
    //        pNew->pEnemyShotHead->prev = pNew->pEnemyShotHead;
    //        pNew->pEnemyShotHead->next = pNew->pEnemyShotHead;
    //        pNew->prev = enemyShotSetHead.prev;
    //        pNew->next = &enemyShotSetHead;
    //        enemyShotSetHead.prev->next = pNew;
    //        enemyShotSetHead.prev = pNew;
    //    }
    //}
}

// ------------------------------------------------------------------
// 面放射用パターン（黄色中玉の扇形）
// ------------------------------------------------------------------
static void FaceBurst(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double base = pSet->muki;
        for (int i = 0; i < 9; ++i) {
            double spread = (i - 4) * 10.0 * DX_PI / 180.0; // -40°～+40°
            sEnemyShot* s = new sEnemyShot;
            s->x = pSet->x;
            s->y = pSet->y;
            s->muki = base + spread;
            s->speed = (200 + GetRand(100)) / 100.0;
            s->kind = img_enemyShotMediumBall[1]; // 黄
            s->prev = pSet->pEnemyShotHead->prev;
            s->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = s;
            pSet->pEnemyShotHead->prev = s;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------------
// 辺炸裂用パターン（ピンクの鱗弾リング）
// ------------------------------------------------------------------
static void EdgeExplosion(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12; ++i) {
            double angle = (i * 30.0 + GetRand(15)) * DX_PI / 180.0;
            sEnemyShot* s = new sEnemyShot;
            s->x = pSet->x;
            s->y = pSet->y;
            s->muki = angle;
            s->speed = 1.6;
            s->kind = img_enemyShotScale[5]; // マゼンタ（ピンク）
            s->prev = pSet->pEnemyShotHead->prev;
            s->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = s;
            pSet->pEnemyShotHead->prev = s;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------------
void EnemyPat_Dodecahedron_DeepSeek()
{
    if (count == 1) {
        // 初期配置（画面中央）
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;

        // ケージ用弾幕セット生成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = CagePattern;
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
    // 敵本体は中央に固定（必要に応じて動かしてもよい）
    else {
        enemy.x = 240.0;
        enemy.y = 240.0;
    }
}