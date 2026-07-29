// enemyPat_Dodecahedron.cpp
// 正十二面体モチーフ弾幕:「正十二面体・展開陣」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  正十二面体の幾何データ
// ============================================================
struct Vec3 { double x, y, z; };

static constexpr double PHI = 1.6180339887498949;      // 黄金比
static constexpr double INV_PHI = 1.0 / PHI;

static constexpr double THETA_SPEED = 0.006;   // 立体の自転速度(全パターン共通)
static constexpr double WIRE_RADIUS = 135.0;   // ワイヤーフレームの外接半径(拡大)
static constexpr int    WIRE_HOLD_FRAME = 110; // 解放までホールドするフレーム数
static constexpr int    EDGE_SUBDIV = 4;       // 辺1本あたりに配置する中間点の数(中点だけでなく4分割点)

static Vec3 g_vertex[20];      // 頂点の単位ベクトル
static Vec3 g_face[12];        // 面中心方向の単位ベクトル(=正二十面体の頂点方向)
static int  g_edge[30][2];     // 辺(頂点インデックスのペア)
static int  g_edgeCount = 0;
static bool g_geomInit = false;

static double VecLen(const Vec3& v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static double Dist(const Vec3& a, const Vec3& b)
{
    double dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

// Y軸回りの回転
static Vec3 RotY(const Vec3& v, double theta)
{
    double c = cos(theta), s = sin(theta);
    return { v.x * c + v.z * s, v.y, -v.x * s + v.z * c };
}

// 頂点20個・面中心12個・辺30本を初期化する。
// 辺は「全頂点対の最短距離＝辺長」とみなし、その距離にある頂点対をすべて拾うことで検出する
// (正十二面体は各頂点の次数が3なので、20*3/2=30本が見つかるはず)。
static void InitGeometry()
{
    if (g_geomInit) return;
    g_geomInit = true;

    Vec3 raw[20] = {
        { 1, 1, 1}, { 1, 1,-1}, { 1,-1, 1}, { 1,-1,-1},
        {-1, 1, 1}, {-1, 1,-1}, {-1,-1, 1}, {-1,-1,-1},
        { 0,  INV_PHI,  PHI}, { 0,  INV_PHI, -PHI}, { 0, -INV_PHI,  PHI}, { 0, -INV_PHI, -PHI},
        { INV_PHI,  PHI, 0}, { INV_PHI, -PHI, 0}, {-INV_PHI,  PHI, 0}, {-INV_PHI, -PHI, 0},
        { PHI, 0,  INV_PHI}, { PHI, 0, -INV_PHI}, {-PHI, 0,  INV_PHI}, {-PHI, 0, -INV_PHI},
    };

    for (int i = 0; i < 20; i++) {
        double len = VecLen(raw[i]);
        g_vertex[i] = { raw[i].x / len, raw[i].y / len, raw[i].z / len };
    }

    double minDist = 1e18;
    for (int i = 0; i < 20; i++)
        for (int j = 0; j < 20; j++)
            if (i != j) {
                double d = Dist(raw[i], raw[j]);
                if (d < minDist) minDist = d;
            }
    g_edgeCount = 0;
    for (int i = 0; i < 20; i++) {
        for (int j = i + 1; j < 20; j++) {
            if (Dist(raw[i], raw[j]) < minDist * 1.01) {
                g_edge[g_edgeCount][0] = i;
                g_edge[g_edgeCount][1] = j;
                g_edgeCount++;
            }
        }
    }

    Vec3 faceRaw[12] = {
        { 0,  1,  PHI}, { 0,  1, -PHI}, { 0, -1,  PHI}, { 0, -1, -PHI},
        { 1,  PHI, 0}, { 1, -PHI, 0}, {-1,  PHI, 0}, {-1, -PHI, 0},
        { PHI, 0,  1}, { PHI, 0, -1}, {-PHI, 0,  1}, {-PHI, 0, -1},
    };
    for (int i = 0; i < 12; i++) {
        double len = VecLen(faceRaw[i]);
        g_face[i] = { faceRaw[i].x / len, faceRaw[i].y / len, faceRaw[i].z / len };
    }
}

// 弾セット生成の共通処理
static sEnemyShotSet* SpawnShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// ============================================================
//  弾幕：正十二面体ワイヤーフレーム（形状を提示 → 一斉解放）
//  20頂点＋30辺×4分割点＝140点を同時出現させ、自転させながらホールドし、
//  「正十二面体だ」と視認させたのち、頂点は自機狙い、辺は放射状弾として解放する。
// ============================================================
static void ShotWireframe(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 頂点20個(鋭い形状＝ダイヤ弾で「角」を強調、大玉相当のダイヤなので目立つ)
        for (int i = 0; i < 20; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->param_d[0] = g_vertex[i].x;
            pEnemyShot->param_d[1] = g_vertex[i].y;
            pEnemyShot->param_d[2] = g_vertex[i].z;
            pEnemyShot->param_i[0] = 1;           // 1:頂点 → 解放時は自機狙い
            pEnemyShot->param_i[1] = GetRand(15);  // 解放タイミングを少しばらけさせる
            pEnemyShot->kind = img_enemyShotDiamond[i % 9];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 辺30本 × EDGE_SUBDIV点(中点だけでなく等分点を複数配置して「線」を強調)
        for (int e = 0; e < g_edgeCount; e++) {
            const Vec3& a = g_vertex[g_edge[e][0]];
            const Vec3& b = g_vertex[g_edge[e][1]];

            for (int si = 1; si <= EDGE_SUBDIV; si++) {
                double s = (double)si / (double)(EDGE_SUBDIV + 1);

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->param_d[0] = a.x + (b.x - a.x) * s;
                pEnemyShot->param_d[1] = a.y + (b.y - a.y) * s;
                pEnemyShot->param_d[2] = a.z + (b.z - a.z) * s;
                pEnemyShot->param_i[0] = 0;           // 0:辺上の点 → 解放時は放射状
                pEnemyShot->param_i[1] = GetRand(15);
                pEnemyShot->kind = img_enemyShotMediumBall[e % 9]; // 中玉で辺をやや太く見せる

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        int releaseFrame = WIRE_HOLD_FRAME + pEnemyShot->param_i[1];

        if (pEnemyShot->count < releaseFrame) {
            // ホールド中：グローバル回転角に合わせて位置を再計算 → 自転する立体として見せる
            double theta = count * THETA_SPEED;
            Vec3 local = { pEnemyShot->param_d[0], pEnemyShot->param_d[1], pEnemyShot->param_d[2] };
            Vec3 rv = RotY(local, theta);
            pEnemyShot->x = pEnemyShotSet->x + rv.x * WIRE_RADIUS;
            pEnemyShot->y = pEnemyShotSet->y - rv.y * WIRE_RADIUS * 0.9 - rv.z * WIRE_RADIUS * 0.25;
        }
        else {
            if (pEnemyShot->count == releaseFrame) {
                if (pEnemyShot->param_i[0] == 1) {
                    pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
                    pEnemyShot->speed = 3.4;
                }
                else {
                    pEnemyShot->muki = atan2(pEnemyShot->y - pEnemyShotSet->y, pEnemyShot->x - pEnemyShotSet->x);
                    pEnemyShot->speed = 2.2;
                }
            }
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  弾幕：面ごとの五方陣（正五角形として静止して見せてから拡散）
//  頂点5個＋各辺3分割点(5辺×3=15個)で五角形の輪郭をしっかり明示する。
//  奥行き(param_d[0])に応じて拡散速度・回転速度・弾の大きさを変える。
// ============================================================
static void ShotPentagonFace(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double depth = pEnemyShotSet->param_d[0]; // およそ-1.0〜+1.0
        bool front = (depth > 0.0);
        double growRate = (front ? 0.050 : 0.026) * 30;
        double spinRate = front ? 0.020 : 0.010;
        double baseAngle = (double)(pEnemyShotSet->kind) * 0.35;

        // 頂点5個(手前は大玉、奥は中玉＝全体的に一段階大きい玉を使用)
        for (int k = 0; k < 5; k++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->param_i[0] = (int)pEnemyShotSet->x; // 中心X
            pEnemyShot->param_i[1] = (int)pEnemyShotSet->y; // 中心Y
            pEnemyShot->param_d[0] = baseAngle + k * (2.0 * DX_PI / 5.0);
            pEnemyShot->param_d[1] = spinRate;
            pEnemyShot->param_d[2] = 1.0; // 半径係数(頂点=1.0)
            pEnemyShot->param_d[3] = growRate;
            pEnemyShot->kind = front ? img_enemyShotLargeBall[pEnemyShotSet->kind % 9]
                : img_enemyShotMediumBall[pEnemyShotSet->kind % 9];
            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 各辺を3分割する点(5辺×3点=15個)で五角形の輪郭線をはっきり見せる
        for (int k = 0; k < 5; k++) {
            double angleK = baseAngle + k * (2.0 * DX_PI / 5.0);
            double angleK1 = baseAngle + (k + 1) * (2.0 * DX_PI / 5.0);

            for (int si = 1; si <= 3; si++) {
                double s = (double)si / 4.0; // 0.25, 0.5, 0.75
                double vx = (1.0 - s) * cos(angleK) + s * cos(angleK1);
                double vy = (1.0 - s) * sin(angleK) + s * sin(angleK1);
                double factor = sqrt(vx * vx + vy * vy);       // 頂点間の直線上にあるため中心からの距離は1未満
                double angleInterp = atan2(vy, vx);

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->param_i[0] = (int)pEnemyShotSet->x;
                pEnemyShot->param_i[1] = (int)pEnemyShotSet->y;
                pEnemyShot->param_d[0] = angleInterp;
                pEnemyShot->param_d[1] = spinRate;
                pEnemyShot->param_d[2] = factor;
                pEnemyShot->param_d[3] = growRate;
                pEnemyShot->kind = img_enemyShotScale[pEnemyShotSet->kind % 9];
                pEnemyShot->margin = 240;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    const double RISE_TIME = 18.0; // 中心から広がって五角形の形になるまで
    const double HOLD_TIME = 36.0; // 五角形の形を保ったまま自転する時間
    const double R_HOLD = 65.0;    // 静止時の五角形の半径(拡大)

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pEnemyShot->count;
        double angle = pEnemyShot->param_d[0] + pEnemyShot->param_d[1] * t * 0.3;
        double factor = pEnemyShot->param_d[2];
        double r;
        if (t < RISE_TIME) {
            r = factor * R_HOLD * (t / RISE_TIME);
        }
        else if (t < RISE_TIME + HOLD_TIME) {
            r = factor * R_HOLD;
        }
        else {
            r = factor * R_HOLD + pEnemyShot->param_d[3] * (t - RISE_TIME - HOLD_TIME);
        }
        pEnemyShot->x = pEnemyShot->param_i[0] + r * cos(angle);
        pEnemyShot->y = pEnemyShot->param_i[1] + r * sin(angle);
        pEnemyShot->muki = angle;

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体：正十二面体・展開陣
//  1段階目=完成形(ワイヤーフレーム全体を提示)、2段階目=演出(面ごとの五方陣)の2段構成。
// ============================================================
void EnemyPat_Dodecahedron_Claude()
{
    constexpr int PHASE_WIRE_START = 1;
    constexpr int WIRE_CLEAR_BUFFER = 150; // 解放後、画面から捌けるまでの猶予
    constexpr int PHASE2_START = PHASE_WIRE_START + WIRE_HOLD_FRAME + WIRE_CLEAR_BUFFER; // 261
    constexpr int FACE_INTERVAL = 10;
    constexpr int PHASE2_LEN = 12 * FACE_INTERVAL + 20; // 140
    constexpr int CYCLE_LEN = PHASE2_START + PHASE2_LEN + 100; // 501 → ここから1段階目に戻る

    if (count == 1) {
        InitGeometry();
        enemy.x = 240.0;
        enemy.y = 160.0;               // 拡大した立体が画面に収まるようやや下げた
        enemy.maxHp = enemy.hp = 200; // 要調整
    }

    // ボスは緩やかに左右移動しつつ多面体を自転させる(拡大に合わせて振幅を抑えた)
    enemy.x = 240.0 + sin(count * 0.006) * 60.0;

    double theta = count * THETA_SPEED;
    int t = count % CYCLE_LEN;

    // ---- ワイヤーフレーム提示 → 一斉解放 ----
    if (t == PHASE_WIRE_START) {
        SpawnShotSet(ShotWireframe, enemy.x, enemy.y, 0.0, 0);
    }

    // ---- 面ごとの五方陣（手前は速く大きく、奥は遅く小さく） ----
    if (t >= PHASE2_START && t < PHASE2_START + PHASE2_LEN &&
        (t - PHASE2_START) % FACE_INTERVAL == 0) {
        int fIdx = (t - PHASE2_START) / FACE_INTERVAL;
        if (fIdx < 12) {
            Vec3 rv = RotY(g_face[fIdx], theta);
            double ox = enemy.x + rv.x * 100.0;
            double oy = enemy.y - rv.y * 90.0 - rv.z * 26.0;

            sEnemyShotSet* pSet = SpawnShotSet(ShotPentagonFace, ox, oy, 0.0, fIdx);
            pSet->param_d[0] = rv.z;
        }
    }
}