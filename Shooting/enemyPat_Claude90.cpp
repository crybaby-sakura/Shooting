// enemyPat_KafunFuyuu.cpp
//
// 「花粉浮遊 〜ブラウン運動〜」
//
// モチーフ：
//   水中に浮かぶ花粉粒(大玉クラスター)が、目に見えない溶媒分子(小弾)からの
//   絶え間ない衝突によってランダムウォークする様子を、専用素材を使わず
//   既存の敵弾を組み合わせて表現する。
//
// ランダムウォークの実現方法：
//   花粉粒の経路は count==1 のタイミングで GetRand により「経由点(waypoint)
//   の並び」として一度だけ確定し、以後は毎フレーム count から現在位置を
//   補間計算する(GetGrainCenter)。速度の積分は一切行わず、常に count の
//   純粋な関数として位置を求めるため、他パターンと同じくリプレイ安全。
//
//   花粉粒が方向転換する瞬間には、必ず直前にその方向から分子弾(銃弾)が
//   飛来して着弾するように仕込んである。統計的にはランダムでも、プレイ
//   ヤーからは「分子が当たったから花粉粒が動いた」という因果関係が見え
//   るようにし、弾幕としての公正さを担保している。
//
// 素材の割り当て：
//   花粉粒本体：大玉(コア,橙) + 中玉(中間リング,黄) + 小玉(外周の縁,白)
//   溶媒分子　：小玉(常時漂う環境分子,シアン) + 銃弾(衝突狙い弾,シアン)
//   剥離片　　：鱗弾(衝突で弾け飛ぶ殻,マゼンタ)
//   自機狙い　：菱形弾(3way / フィナーレ5way,赤)
//   フィナーレ：中楕円弾(放射リング,青)
//
// フェーズ構成(900フレーム=15秒で1サイクル、以後ループ)：
//   0   - 90  : 静止観察　　花粉粒が組み上がり、環境分子だけがふわふわ漂う
//   90  - 450 : 衝突乱舞　　分子弾が周期的に飛来し花粉粒が方向転換、殻剥離
//   450 - 810 : 拡散暴走　　衝突頻度が増加、自機狙い3wayも混じる
//   810 - 900 : 熱平衡拡散　予告音の後、花粉粒本体が放射状に加速飛散して終幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const int    WP_COUNT = 22 + 10;   // ランダムウォーク経由点の数
static const int    STEP_FRAMES = 45 - 10;   // 経由点間の遷移フレーム数
static const int    FLIGHT_FRAMES = 30;  // 分子弾が着弾するまでの飛行フレーム数
static const int    PHASE1_END = 90;   // 静止観察フェーズの終端
static const int    PHASE2_END = 450 - 100;  // 衝突乱舞フェーズの終端
static const int    PHASE3_END = 810 - 100;  // 拡散暴走フェーズの終端
static const int    CYCLE = 900;  // 1サイクルの総フレーム数

// ============================================================
//  ランダムウォーク経路(花粉粒の経由点列)
// ============================================================
static double g_wayX[WP_COUNT];
static double g_wayY[WP_COUNT];
static bool   g_wayInit = false;

// count==1 のタイミングで一度だけ呼ぶこと。GetRand の呼び出し順を
// 他のGetRand呼び出しより必ず先頭に置き、リプレイの決定性を保つ。
static void InitBrownianWaypoints()
{
//    if (g_wayInit) return;
    g_wayInit = true;

    g_wayX[0] = 240.0;
    g_wayY[0] = 190.0;

    for (int i = 1; i < WP_COUNT; i++) {
        double ang = GetRand(359) / 180.0 * DX_PI;
        double len = 40.0 + GetRand(60); // 40〜100の範囲でランダムな歩幅

        double nx = g_wayX[i - 1] + len * cos(ang);
        double ny = g_wayY[i - 1] + len * sin(ang);

        // 画面外へ迷子にならないよう、安全範囲の外に出たら反射させる
        if (nx < 90.0)  nx = 180.0 - nx;
        if (nx > 390.0) nx = 780.0 - nx;
        if (ny < 90.0)  ny = 180.0 - ny;
        if (ny > 340.0) ny = 680.0 - ny;

        g_wayX[i] = nx;
        g_wayY[i] = ny;
    }
}

// 与えられた c (0〜CYCLE-1) における花粉粒中心座標を求める。
// c が PHASE3_END を超えた分は PHASE3_END の位置に固定される
// (＝崩壊フェーズでは花粉粒が凍結した位置を基準に飛散する)。
static void GetGrainCenter(int c, double* outX, double* outY)
{
    int cc = c;
    if (cc > PHASE3_END) cc = PHASE3_END;

    int idx = cc / STEP_FRAMES;
    if (idx > WP_COUNT - 2) idx = WP_COUNT - 2;

    double t = (cc % STEP_FRAMES) / (double)STEP_FRAMES;
    double e = t * t * (3.0 - 2.0 * t); // smoothstepイージング

    *outX = g_wayX[idx] + (g_wayX[idx + 1] - g_wayX[idx]) * e;
    *outY = g_wayY[idx] + (g_wayY[idx + 1] - g_wayY[idx]) * e;
}

// ============================================================
//  弾生成の共通ヘルパー
// ============================================================
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind)
{
    sEnemyShotSet* s = new sEnemyShotSet;
    s->count = 0;
    s->patternFunc = func;
    s->x = x;
    s->y = y;
    s->muki = muki;
    s->kind = kind;

    s->pEnemyShotHead = new sEnemyShot;
    s->pEnemyShotHead->prev = s->pEnemyShotHead;
    s->pEnemyShotHead->next = s->pEnemyShotHead;

    s->prev = enemyShotSetHead.prev;
    s->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = s;
    enemyShotSetHead.prev = s;
    return s;
}

static sEnemyShot* AddShot(sEnemyShotSet* s, int kind)
{
    sEnemyShot* shot = new sEnemyShot;
    shot->margin = 240;
    shot->kind = kind;
    shot->prev = s->pEnemyShotHead->prev;
    shot->next = s->pEnemyShotHead;
    s->pEnemyShotHead->prev->next = shot;
    s->pEnemyShotHead->prev = shot;
    return shot;
}

// ============================================================
//  (A) 花粉粒本体：大玉コア + 中玉リング + 小玉の縁
// ============================================================
static const int GRAIN_CORE_N = 7;
static const int GRAIN_MID_N = 16;
static const int GRAIN_EDGE_N = 28;

static void ShotGrainCluster(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // コア：大玉7発を六角状+中心1発で密集配置(role=0)
        static const double coreOffX[GRAIN_CORE_N] = { 0, 16,  8, -8, -16,  -8,  8 };
        static const double coreOffY[GRAIN_CORE_N] = { 0,  0, 14, 14,   0, -14,-14 };
        for (int i = 0; i < GRAIN_CORE_N; i++) {
            sEnemyShot* shot = AddShot(pSet, img_enemyShotLargeBall[8]); // 橙
            shot->param_i[0] = 0;
            shot->param_d[0] = coreOffX[i];
            shot->param_d[1] = coreOffY[i];
        }
        // 中間リング：中玉16発、半径28でゆっくり自転(role=1)
        for (int i = 0; i < GRAIN_MID_N; i++) {
            sEnemyShot* shot = AddShot(pSet, img_enemyShotMediumBall[1]); // 黄
            shot->param_i[0] = 1;
            shot->param_d[0] = 28.0;
            shot->param_d[1] = (DX_PI * 2.0 / GRAIN_MID_N) * i;
        }
        // 外周の縁：小玉28発、半径40で個別に微振動しながら逆回転(role=2)
        for (int i = 0; i < GRAIN_EDGE_N; i++) {
            sEnemyShot* shot = AddShot(pSet, img_enemyShotSmallBall[6]); // 白
            shot->param_i[0] = 2;
            shot->param_d[0] = 40.0;
            shot->param_d[1] = (DX_PI * 2.0 / GRAIN_EDGE_N) * i;
            shot->param_d[2] = GetRand(1000) / 1000.0 * DX_PI * 2.0; // 個別位相
        }
    }

    int c = count % CYCLE;
    double cx, cy;
    GetGrainCenter(c, &cx, &cy);
    bool shattered = (c >= PHASE3_END);
    double shatterT = shattered ? (double)(c - PHASE3_END) : 0.0;

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (!shattered) {
            if (pShot->param_i[0] == 0) {
                // コア：中心にほぼ固定、わずかに脈動
                double pulse = 1.0 + 0.06 * sin(c / 20.0);
                pShot->x = cx + pShot->param_d[0] * pulse;
                pShot->y = cy + pShot->param_d[1] * pulse;
            }
            else if (pShot->param_i[0] == 1) {
                double ang = pShot->param_d[1] + c / 260.0;
                pShot->x = cx + pShot->param_d[0] * cos(ang);
                pShot->y = cy + pShot->param_d[0] * sin(ang);
            }
            else {
                double ang = pShot->param_d[1] - c / 180.0; // 中間リングと逆回転
                double jitter = 4.0 * sin(c / 9.0 + pShot->param_d[2]); // ブラウン運動らしい微振動
                double r = pShot->param_d[0] + jitter;
                pShot->x = cx + r * cos(ang);
                pShot->y = cy + r * sin(ang);
            }
        }
        else {
            // 崩壊フェーズ：初回のみ凍結位置と発散角を確定してparam_dへ保存
            if (pShot->param_i[1] == 0) {
                double ang;
                if (pShot->param_i[0] == 0) {
                    ang = (pShot->param_d[0] == 0.0 && pShot->param_d[1] == 0.0)
                        ? 0.0 : atan2(pShot->param_d[1], pShot->param_d[0]);
                }
                else if (pShot->param_i[0] == 1) {
                    ang = pShot->param_d[1] + PHASE3_END / 260.0;
                }
                else {
                    ang = pShot->param_d[1] - PHASE3_END / 180.0;
                }
                pShot->param_d[3] = ang;       // 発散角(固定)
                pShot->param_d[4] = pShot->x;  // 凍結原点X(前フレームの位置)
                pShot->param_d[5] = pShot->y;  // 凍結原点Y
                pShot->param_i[1] = 1;
            }
            double accel = 0.03;
            double disp = 0.5 * accel * shatterT * shatterT;
            pShot->x = pShot->param_d[4] + disp * cos(pShot->param_d[3]);
            pShot->y = pShot->param_d[5] + disp * sin(pShot->param_d[3]);
        }
        if (pSet->count >= CYCLE) pShot->margin = -9999;
        pShot = pShot->next;
    }
}

static void SpawnGrainCluster()
{
    CreateShotSet(ShotGrainCluster, 240.0, 190.0, 0.0, 0);
}

// ============================================================
//  (B) 常時漂う環境分子(装飾寄り、ゆるく揺れながら降りてくる)
// ============================================================
static void ShotAmbientMolecule(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        sEnemyShot* shot = AddShot(pSet, img_enemyShotSmallBall[3]); // シアン
        shot->param_d[0] = pSet->x; // 基準X
        shot->param_d[1] = pSet->y; // 基準Y
        shot->param_d[2] = GetRand(1000) / 1000.0 * DX_PI * 2.0; // 位相
        shot->param_d[3] = 10.0 + GetRand(15); // 揺れ幅
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = pShot->count;
        pShot->x = pShot->param_d[0] + pShot->param_d[3] * sin(t / 28.0 + pShot->param_d[2]);
        pShot->y = pShot->param_d[1] + t * 0.55 + pShot->param_d[3] * 0.6 * cos(t / 33.0 + pShot->param_d[2]);
        pShot = pShot->next;
    }
}

static void SpawnAmbientMolecule()
{
    double x = 60.0 + GetRand(360);
    double y = 20.0 + GetRand(60);
    CreateShotSet(ShotAmbientMolecule, x, y, 0.0, 0);
}

// ============================================================
//  (C) 衝突弾(分子)：花粉粒の次の経由点めがけて直進着弾する
// ============================================================
static void ShotMoleculeImpact(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        double ang = GetRand(359) / 180.0 * DX_PI;
        double r = 240.0 + GetRand(60);
        double originX = pSet->x + r * cos(ang);
        double originY = pSet->y + r * sin(ang);

        sEnemyShot* shot = AddShot(pSet, img_enemyShotBullet[3]); // シアンの銃弾
        shot->param_d[0] = originX;
        shot->param_d[1] = originY;
        shot->param_d[2] = pSet->x; // 着弾目標X(=花粉粒の次の中心位置)
        shot->param_d[3] = pSet->y; // 着弾目標Y
        shot->muki = atan2(pSet->y - originY, pSet->x - originX);
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = pShot->count / (double)FLIGHT_FRAMES;
        // tは1を超えても構わない(着弾後もそのまま直進して画面外へ抜ける)
        pShot->x = pShot->param_d[0] + (pShot->param_d[2] - pShot->param_d[0]) * t;
        pShot->y = pShot->param_d[1] + (pShot->param_d[3] - pShot->param_d[1]) * t;
        pShot = pShot->next;
    }
}

static void SpawnMoleculeImpact(int targetFrame)
{
    double tx, ty;
    GetGrainCenter(targetFrame % CYCLE, &tx, &ty);
    CreateShotSet(ShotMoleculeImpact, tx, ty, 0.0, 0);
}

// ============================================================
//  (D) 殻剥離弾：衝突の瞬間に花粉粒表面から弾け飛ぶ破片
// ============================================================
static void ShotShellShed(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        double baseAng = pSet->muki; // 呼び出し側で設定した基準角
        int n = pSet->param_i[0];
        for (int i = 0; i < n; i++) {
            double ang = baseAng + (GetRand(200) - 100) / 100.0 * (DX_PI / 3.0); // ±60度でばらける
            double spd = 1.0 + GetRand(150) / 100.0;
            sEnemyShot* shot = AddShot(pSet, img_enemyShotScale[5]); // マゼンタの鱗弾
            shot->muki = ang;
            shot->speed = spd;
        }
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = pShot->count;
        pShot->x = pSet->x + pShot->speed * t * cos(pShot->muki);
        pShot->y = pSet->y + pShot->speed * t * sin(pShot->muki);
        pShot = pShot->next;
    }
}

static void SpawnShellShed(int n)
{
    int c = count % CYCLE;
    double cx, cy;
    GetGrainCenter(c, &cx, &cy);

    int idx = c / STEP_FRAMES;
    if (idx > WP_COUNT - 2) idx = WP_COUNT - 2;
    double moveAng = atan2(g_wayY[idx + 1] - cy, g_wayX[idx + 1] - cx);

    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

    // 進行方向と逆側(＝弾かれた側)を基準角として殻を散らす
    sEnemyShotSet* s = CreateShotSet(ShotShellShed, cx, cy, moveAng + DX_PI, 0);
    s->param_i[0] = n;
}

// ============================================================
//  (E) 自機狙い3way(拡散暴走フェーズの合いの手)
// ============================================================
static void ShotAimed3Way(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        double baseAng = atan2(player.y - pSet->y, player.x - pSet->x);
        double offs[3] = { -0.18, 0.0, 0.18 };
        for (int i = 0; i < 3; i++) {
            sEnemyShot* shot = AddShot(pSet, img_enemyShotDiamond[0]); // 赤の菱形弾
            shot->muki = baseAng + offs[i];
            shot->speed = 2.6;
        }
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = pShot->count;
        pShot->x = pSet->x + pShot->speed * t * cos(pShot->muki);
        pShot->y = pSet->y + pShot->speed * t * sin(pShot->muki);
        pShot = pShot->next;
    }
}

static void SpawnAimedBurst()
{
    double cx, cy;
    GetGrainCenter(count % CYCLE, &cx, &cy);
    CreateShotSet(ShotAimed3Way, cx, cy, 0.0, 0);
}

// ============================================================
//  (F) フィナーレ：大型リング + 自機狙い5way
// ============================================================
static void ShotFinaleRing(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int RING_N = 24;
        for (int i = 0; i < RING_N; i++) {
            double ang = (DX_PI * 2.0 / RING_N) * i;
            sEnemyShot* shot = AddShot(pSet, img_enemyShotMediumOval[4]); // 青の中楕円弾
            shot->muki = ang;
            shot->speed = 1.8;
        }
        double aimAng = atan2(player.y - pSet->y, player.x - pSet->x);
        double offs[5] = { -0.3, -0.15, 0.0, 0.15, 0.3 };
        for (int i = 0; i < 5; i++) {
            sEnemyShot* shot = AddShot(pSet, img_enemyShotDiamond[0]); // 赤の菱形弾
            shot->muki = aimAng + offs[i];
            shot->speed = 3.2;
        }
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = pShot->count;
        pShot->x = pSet->x + pShot->speed * t * cos(pShot->muki);
        pShot->y = pSet->y + pShot->speed * t * sin(pShot->muki);
        pShot = pShot->next;
    }
}

static void SpawnFinaleRing()
{
    double cx, cy;
    GetGrainCenter(PHASE3_END, &cx, &cy); // 凍結位置を中心に使う
    CreateShotSet(ShotFinaleRing, cx, cy, 0.0, 0);
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_BrownianMotion_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // ボス本体は花粉粒本体とは独立して、上部でゆったり左右に揺れるだけ
    enemy.x = 240.0 + 30.0 * sin(count / 130.0);

    int c = count % CYCLE;

    // サイクル開始ごとに花粉粒本体(弾クラスター)を新規生成
    if (c == 1) {
        InitBrownianWaypoints();
        SpawnGrainCluster();
    }

    // ブラウン運動は静止中も止まらない、を表現するため常時背景分子を発生
    if (c % 12 == 1) {
        SpawnAmbientMolecule();
    }

    // フェーズ2・3：分子衝突→花粉粒の方向転換＆殻剥離
    if (c >= PHASE1_END && c < PHASE3_END) {
        int stepIdx = c / STEP_FRAMES;
        int intoStep = c % STEP_FRAMES;
        bool intensified = (c >= PHASE2_END);
        int molCount = intensified ? 5 : 2;
        int shardCount = intensified ? 10 : 4;

        // 次の経由点への到達フレームからFLIGHT_FRAMES分だけ手前で分子弾を発射
        if (intoStep == STEP_FRAMES - FLIGHT_FRAMES) {
            int nextTransition = (stepIdx + 1) * STEP_FRAMES;
            for (int i = 0; i < molCount; i++) {
                SpawnMoleculeImpact(nextTransition);
            }
        }

        // 経由点到達(＝着弾)の瞬間に殻剥離。最初の遷移だけは対応する分子が
        // 間に合わないため1ステップ後ろにずらす
        if (intoStep == 0 && c >= PHASE1_END + STEP_FRAMES) {
            SpawnShellShed(shardCount);
            if (intensified && (stepIdx % 2 == 0)) {
                SpawnAimedBurst();
            }
        }
    }

    // フェーズ4：予告→大分解
    if (c == PHASE3_END) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (c == PHASE3_END + 30) {
        SpawnFinaleRing();
    }
}