// enemyPat_GohouseiKengen.cpp
//
// 「五芒星顕現」 五芒星(ペンタグラム)をモチーフにした4フェーズパターン
//
// Phase1 一筆書き顕現 : 五芒星は「1つ飛ばしで頂点を結ぶ」一筆書き図形(0→2→4→1→3→0)。
//                        この軌跡を筆先がなぞりながら弾を置いていき、星の輪郭を描き上げる。
// Phase2 二重回転定常 : 完成した外周の星と、自己交差でできる内接五角形(黄金比 1/φ² の相似形)
//                        の2層が互いに逆回転しながら常駐。外周の5頂点が順に自機狙い3wayを発射。
// Phase3 交差光条     : 星の5本の対角線(一筆書きと同じ辺)に沿ってレーザー片が流れる。
//                        奇数波/偶数波で向きを反転させ、伸縮するような呼吸を表現。
// Phase4 崩壊放散     : 予告ののち、それまでの全ての星形弾がその場から放射状に加速飛散し、
//                        中心から自機狙い5way弾とリングバーストが放たれて締めくくる。
//
// 全体は TOTAL_CYCLE フレームで1周する無限ループ構成。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const double CX = 240.0;              // 五芒星の中心(画面480x480のやや上寄り)
static const double CY_ = 220.0;
static const double R1 = 160.0;              // 外周(星の頂点)半径
static const double INNER_RATIO = 0.382;     // 黄金比 1/φ² による内接五角形との半径比
static const double R2 = R1 * INNER_RATIO;   // 内接五角形(自己交差)の半径

// フェーズ長(フレーム数)
static const int PHASE1_LEN = 350-100;           // 一筆書き顕現
static const int PHASE2_LEN = 480-100;           // 二重回転定常
static const int PHASE3_LEN = 450-100;           // 交差光条
static const int TELEGRAPH_LEN = 40;         // 崩壊予告
static const int FINALE_LEN = 200-50;           // 崩壊放散

static const int PHASE2_START = PHASE1_LEN + 1;
static const int PHASE3_START = PHASE2_START + PHASE2_LEN;
static const int TELEGRAPH_START = PHASE3_START + PHASE3_LEN;
static const int FINALE_START = TELEGRAPH_START + TELEGRAPH_LEN;
static const int TOTAL_CYCLE = FINALE_START + FINALE_LEN - 1;

// Phase1: 一筆書き順(0→2→4→1→3→0)
static const int STAR_ORDER[6] = { 0, 2, 4, 1, 3, 0 };
static const int SEG_DUR = PHASE1_LEN / 5;   // 70
static const int DRAW_INTERVAL = 2;

// Phase2: 内接五角形の顕現 + 二重回転
static const int INNER_REVEAL_LEN = 60;
static const int INNER_SEG_DUR = INNER_REVEAL_LEN / 5; // 12
static const int INNER_DRAW_INTERVAL = 2;
static const double ROT_SPEED_OUTER = 0.0100; // 外周ラインの自転速度(rad/frame)
static const double ROT_SPEED_INNER = 0.0170; // 内接五角形の自転速度(外周と逆回転)
static const int FIRE_INTERVAL = 40-20;
static const int PULSE_INTERVAL = 80-40;

// Phase3: 対角線(辺)を伝う光条
static const int WAVE_INTERVAL = 90;
static const int LASER_COUNT = 8;
static const double LASER_SPEED = 3.4+2.0;

// Phase4: 崩壊放散
static const double FINALE_SPEED = 2.5;
static const double FINALE_ACCEL = 0.04;

// 崩壊フラグ(ShotSealPointから参照するのでファイルスコープに置く)
static bool   g_finaleActive = false;
static double g_finaleTriggerT = 0.0;

// ============================================================
//  汎用ヘルパー
// ============================================================
static void AddShot(sEnemyShotSet* pSet, sEnemyShot* pShot)
{
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// wayN/mode/speed/spreadRad は ShotStraightFan 専用パラメータ。
// ShotSealPoint を使う場合は 0 を渡しておけばよい。
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind,
    int wayN, int mode, double speed, double spreadRad)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x; pSet->y = y; pSet->muki = muki; pSet->kind = kind;
    pSet->param_i[0] = wayN;
    pSet->param_i[1] = mode;
    pSet->param_d[0] = speed;
    pSet->param_d[2] = spreadRad;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

static void GetOuterVertex(int i, double* x, double* y)
{
    double angle = -DX_PI / 2.0 + i * 2.0 * DX_PI / 5.0;
    *x = CX + R1 * cos(angle);
    *y = CY_ + R1 * sin(angle);
}

static void GetInnerVertex(int i, double* x, double* y)
{
    double angle = -DX_PI / 2.0 + DX_PI / 5.0 + i * 2.0 * DX_PI / 5.0;
    *x = CX + R2 * cos(angle);
    *y = CY_ + R2 * sin(angle);
}

// 現在(ローカル絶対時刻t)における外周頂点iの回転後座標
static void GetRotatedOuterVertex(int i, double t, double* x, double* y)
{
    double angle = -DX_PI / 2.0 + i * 2.0 * DX_PI / 5.0 + ROT_SPEED_OUTER * t;
    *x = CX + R1 * cos(angle);
    *y = CY_ + R1 * sin(angle);
}

// ============================================================
//  弾幕: 星の輪郭点(常駐・自転し、崩壊時にその場から放射状に飛散する)
//
//  param_i[0] : ロール(0=外周の星ライン, 1=内接五角形ライン)
//  param_i[1] : 崩壊時の捕捉済みフラグ
//  param_d[0] : 生成時点での中心からの半径 r0
//  param_d[1] : 生成時点での中心からの角度 theta0
//  param_d[2] : 生成時のローカル絶対時刻(spawnT)
//  param_d[3],[4] : 崩壊捕捉時の座標(捕捉後の飛散起点)
//  param_d[5] : 崩壊捕捉時のローカル絶対時刻
//  param_d[6] : 崩壊時の放散方向角
// ============================================================
static void ShotSealPoint(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double rotSpeed = (pShot->param_i[0] == 0) ? ROT_SPEED_OUTER : -ROT_SPEED_INNER;
        // spawnT + count で「現在のローカル絶対時刻」を復元できる(countは自動でスポーン時0から加算されるため)
        double absT = pShot->param_d[2] + pShot->count;

        if (!g_finaleActive) {
            double theta = pShot->param_d[1] + rotSpeed * absT;
            pShot->x = CX + pShot->param_d[0] * cos(theta);
            pShot->y = CY_ + pShot->param_d[0] * sin(theta);
        }
        else {
            if (pShot->param_i[1] == 0) {
                // 崩壊トリガー時点の回転位置を一度だけ捕捉し、そこを飛散の起点にする
                double thetaAtTrigger = pShot->param_d[1] + rotSpeed * g_finaleTriggerT;
                double cx0 = CX + pShot->param_d[0] * cos(thetaAtTrigger);
                double cy0 = CY_ + pShot->param_d[0] * sin(thetaAtTrigger);
                pShot->param_d[3] = cx0;
                pShot->param_d[4] = cy0;
                pShot->param_d[5] = g_finaleTriggerT;
                pShot->param_d[6] = atan2(cy0 - CY_, cx0 - CX);
                pShot->param_i[1] = 1;
            }
            double elapsedSinceCapture = absT - pShot->param_d[5];
            double dist = FINALE_SPEED * elapsedSinceCapture + 0.5 * FINALE_ACCEL * elapsedSinceCapture * elapsedSinceCapture;
            pShot->x = pShot->param_d[3] + dist * cos(pShot->param_d[6]);
            pShot->y = pShot->param_d[4] + dist * sin(pShot->param_d[6]);
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕: 直線弾(自機狙いway弾・等間隔リング・単発レーザー片を1つの関数で共用)
//
//  pSet->param_i[0] : 発射数 N
//  pSet->param_i[1] : モード(0=muki中心の扇状, 1=muki始点の全周等間隔リング)
//  pSet->param_d[0] : 速さ
//  pSet->param_d[2] : 扇の全開き角(モード0のみ使用)
// ============================================================
static void ShotStraightFan(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        int n = pSet->param_i[0];
        int mode = pSet->param_i[1];
        double speed = pSet->param_d[0];

        for (int i = 0; i < n; i++) {
            double ang;
            if (mode == 1) {
                ang = pSet->muki + 2.0 * DX_PI * i / n;
            }
            else if (n == 1) {
                ang = pSet->muki;
            }
            else {
                ang = pSet->muki - pSet->param_d[2] / 2.0 + pSet->param_d[2] * i / (double)(n - 1);
            }

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x; pShot->y = pSet->y;
            pShot->kind = pSet->kind;
            pShot->muki = ang;
            pShot->param_d[0] = pSet->x;
            pShot->param_d[1] = pSet->y;
            pShot->param_d[2] = ang;
            pShot->param_d[3] = speed;
            AddShot(pSet, pShot);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0] + pShot->param_d[3] * cos(pShot->param_d[2]) * pShot->count;
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * sin(pShot->param_d[2]) * pShot->count;
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Pentagram_Claude()
{
    static sEnemyShotSet* outerTrailSet = nullptr;
    static sEnemyShotSet* innerTrailSet = nullptr;

    if (count == 1) {
        enemy.x = CX;
        enemy.y = CY_;
        enemy.maxHp = enemy.hp = 200;
    }

    int t = (count - 1) % TOTAL_CYCLE + 1; // 1周TOTAL_CYCLEフレームのローカル絶対時刻

    if (t == 1) {
        // 新しい周期の開始:状態を初期化し、星を再顕現させる
        g_finaleActive = false;
        innerTrailSet = nullptr;
        outerTrailSet = CreateShotSet(ShotSealPoint, CX, CY_, 0.0, 0, 0, 0, 0.0, 0.0);

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // ---- Phase1: 一筆書き顕現 ----
    if (t <= PHASE1_LEN) {
        int elapsed = t - 1;
        if (elapsed % DRAW_INTERVAL == 0) {
            int seg = elapsed / SEG_DUR; if (seg > 4) seg = 4;
            double frac = (elapsed - seg * SEG_DUR) / (double)SEG_DUR; if (frac > 1.0) frac = 1.0;

            double ax, ay, bx, by;
            GetOuterVertex(STAR_ORDER[seg], &ax, &ay);
            GetOuterVertex(STAR_ORDER[seg + 1], &bx, &by);
            double px = ax + (bx - ax) * frac;
            double py = ay + (by - ay) * frac;

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = px; pShot->y = py;
            pShot->kind = img_enemyShotMediumBall[3]; // シアン
            pShot->param_i[0] = 0; // 外周ロール
            pShot->param_i[1] = 0; // 未捕捉
            double dx = px - CX, dy = py - CY_;
            pShot->param_d[0] = sqrt(dx * dx + dy * dy);
            pShot->param_d[1] = atan2(dy, dx);
            pShot->param_d[2] = (double)t;
            AddShot(outerTrailSet, pShot);
        }
    }

    // ---- Phase2: 内接五角形の顕現 + 二重回転定常 ----
    if (t >= PHASE2_START && t < PHASE2_START + PHASE2_LEN) {
        int e2 = t - PHASE2_START;

        if (e2 == 0) {
            innerTrailSet = CreateShotSet(ShotSealPoint, CX, CY_, 0.0, 0, 0, 0, 0.0, 0.0);
        }
        if (e2 < INNER_REVEAL_LEN && e2 % INNER_DRAW_INTERVAL == 0) {
            int seg = e2 / INNER_SEG_DUR; if (seg > 4) seg = 4;
            double frac = (e2 - seg * INNER_SEG_DUR) / (double)INNER_SEG_DUR; if (frac > 1.0) frac = 1.0;

            double ax, ay, bx, by;
            GetInnerVertex(seg % 5, &ax, &ay);
            GetInnerVertex((seg + 1) % 5, &bx, &by);
            double px = ax + (bx - ax) * frac;
            double py = ay + (by - ay) * frac;

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = px; pShot->y = py;
            pShot->kind = img_enemyShotMediumBall[5]; // マゼンタ
            pShot->param_i[0] = 1; // 内接ロール
            pShot->param_i[1] = 0;
            double dx = px - CX, dy = py - CY_;
            pShot->param_d[0] = sqrt(dx * dx + dy * dy);
            pShot->param_d[1] = atan2(dy, dx);
            pShot->param_d[2] = (double)t;
            AddShot(innerTrailSet, pShot);
        }

        // 外周5頂点が順番に、現在の自転位置から自機狙い3wayを発射
        if (e2 % FIRE_INTERVAL == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            int idx = (e2 / FIRE_INTERVAL) % 5;
            double angle0 = -DX_PI / 2.0 + idx * 2.0 * DX_PI / 5.0;
            double theta = angle0 + ROT_SPEED_OUTER * t;
            double fx = CX + R1 * cos(theta), fy = CY_ + R1 * sin(theta);
            double muki = atan2(player.y - fy, player.x - fx);
            CreateShotSet(ShotStraightFan, fx, fy, muki, img_enemyShotDiamond[0], 3, 0, 2.6, 30.0 * DX_PI / 180.0);
        }
        // 内接五角形からの小規模な予兆パルス(非狙い6way)
        if (e2 % PULSE_INTERVAL == PULSE_INTERVAL / 2) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            
            int idx2 = (e2 / PULSE_INTERVAL) % 5;
            double angleIn = (-DX_PI / 2.0 + DX_PI / 5.0) + idx2 * 2.0 * DX_PI / 5.0;
            double thetaIn = angleIn - ROT_SPEED_INNER * t;
            double ix = CX + R2 * cos(thetaIn), iy = CY_ + R2 * sin(thetaIn);
            CreateShotSet(ShotStraightFan, ix, iy, 0.0, img_enemyShotSmallBall[6], 6, 1, 1.8, 0.0);
        }
    }

    // ---- Phase3: 交差光条(対角線を伝うレーザー、波ごとに向きを反転) ----
    if (t >= PHASE3_START && t < PHASE3_START + PHASE3_LEN) {
        int e3 = t - PHASE3_START;
        if (e3 % WAVE_INTERVAL == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            int waveIdx = e3 / WAVE_INTERVAL;
            bool forward = (waveIdx % 2 == 0);

            for (int k = 0; k < 5; k++) {
                double ax, ay, bx, by;
                GetRotatedOuterVertex(STAR_ORDER[k], (double)t, &ax, &ay);
                GetRotatedOuterVertex(STAR_ORDER[k + 1], (double)t, &bx, &by);
                if (!forward) { double tx = ax, ty = ay; ax = bx; ay = by; bx = tx; by = ty; }

                double dirx = bx - ax, diry = by - ay;
                double dirAngle = atan2(diry, dirx);

                for (int j = 0; j < LASER_COUNT; j++) {
                    double frac = j / (double)(LASER_COUNT - 1);
                    double px = ax + dirx * frac, py = ay + diry * frac;
                    CreateShotSet(ShotStraightFan, px, py, dirAngle, img_enemyShotLaser[3], 1, 0, LASER_SPEED, 0.0);
                    CreateShotSet(ShotStraightFan, px, py, dirAngle + DX_PI, img_enemyShotLaser[3], 1, 0, LASER_SPEED, 0.0);
                }
            }
            double muki = atan2(player.y - CY_, player.x - CX);
            CreateShotSet(ShotStraightFan, CX, CY_, muki, img_enemyShotDiamond[0], 3, 0, 2.4, 30.0 * DX_PI / 180.0);
        }
    }

    // ---- Phase4: 崩壊予告 → 崩壊放散 ----
    if (t == TELEGRAPH_START) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (t == FINALE_START) {
        g_finaleActive = true;
        g_finaleTriggerT = (double)t;

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double muki = atan2(player.y - CY_, player.x - CX);
        CreateShotSet(ShotStraightFan, CX, CY_, muki, img_enemyShotLargeBall[8], 5, 0, 3.0, 60.0 * DX_PI / 180.0);
        CreateShotSet(ShotStraightFan, CX, CY_, 0.0, img_enemyShotSmallBall[6], 24, 1, 2.2, 0.0);
    }
    if (t == FINALE_START + 20) {
        CreateShotSet(ShotStraightFan, CX, CY_, DX_PI / 32.0, img_enemyShotSmallBall[3], 32, 1, 3.4, 0.0);
    }
}