// enemyPat_KayariUzuShoukou.cpp
// 「蚊遣り渦焼香」 — 蚊取り線香モチーフの弾幕パターン
//
// 蚊取り線香専用の素材は存在しないため、以下の通常素材の組み合わせで表現する。
//   ・img_enemyShotMediumBall（中玉）… 線香本体。色だけを 緑(未燃焼)→橙(点火)→黒(灰) と
//     切り替えることで「燃え進む線香」を1種類の弾形状のまま演出する。
//   ・img_enemyShotScale（鱗弾）… 白系で煙、シアン系で香りの波紋を表現。細長い形状が
//     ゆらぎ弾との相性が良い。
//   ・img_enemyShotBullet（銃弾）… 橙色で「火の粉」の自機狙い弾。
//   ・img_enemyShotLargeBall（大玉）… 燃え尽きフィナーレの自機狙い5way。存在感のある大玉で
//     大技感を出す。
// レーザー・菱形弾・中楕円弾は今回のモチーフに合わないため不使用。
//
// 構成（無限ループ、1周あたり局所カウント基準）:
//   フェーズ1 巻回形成   : count  0～169   渦巻き本体が中心から外側へ描かれていく
//   フェーズ2/3 点火巡行〜香煙拡散 : count 170～769  燃焼点が外周→中心へ移動しつつ
//                                                  煙・火の粉・香りの波紋を継続発生
//   フェーズ4 燃え尽き・灰散 : count 800(予告込み)で全弾が放射状に加速飛散＋自機狙い5way

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  渦巻き（コイル本体）のパラメータ
// ============================================================
static const double SPIRAL_CENTER_X = 240.0;
static const double SPIRAL_CENTER_Y = 240.0;
static const double SPIRAL_A = 3.0;              // r = SPIRAL_A * theta
static const double SPIRAL_DTHETA = 0.12;        // 隣接セグメント間の角度差
static const int    SPIRAL_N = 600;              // コイルを構成する弾の総数

static const double SPIRAL_BUILD_STEP = 0.25;    // 弾iの登場フレーム = i * SPIRAL_BUILD_STEP
static const double SPIRAL_REVEAL_EASE = 8.0;    // 登場時、中心から所定半径へ伸びるまでの緩和フレーム数
static const int    SPIRAL_BUILD_DONE = 170;     // 巻回形成が完了するローカルカウント(T1)

static const int    SPIRAL_BURN_INTERVAL = 1;    // 1セグメント燃えるのにかかるフレーム数
static const int    SPIRAL_BURN_TOTAL = SPIRAL_N * SPIRAL_BURN_INTERVAL; // 燃焼フェーズの長さ
static const int    SPIRAL_WARN_DELAY = 30;      // 燃え尽き直前の予告点滅フレーム数
static const int    SPIRAL_FINALE_LOCAL = SPIRAL_BUILD_DONE + SPIRAL_BURN_TOTAL + SPIRAL_WARN_DELAY; // 800
static const double SPIRAL_BURST_ACCEL = 0.025;   // フィナーレの放射飛散加速度係数

// ============================================================
//  弾幕：コイル本体（渦巻き）
//  ―フェーズ1(巻回形成)～フェーズ4(燃え尽き・灰散)までを一貫して担当する常駐ShotSet
// ============================================================
static void ShotSpiralCoil(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < SPIRAL_N; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double theta = i * SPIRAL_DTHETA;
            double r = SPIRAL_A * theta;

            // 登場前は中心に置いておき、formula側で登場タイミングまで半径0として扱う
            pEnemyShot->x = SPIRAL_CENTER_X;
            pEnemyShot->y = SPIRAL_CENTER_Y;
            pEnemyShot->muki = theta; // 中心から見た方向として利用（燃え尽き時の飛散方向にも流用）
            pEnemyShot->speed = 0.0;  // 速度加算は行わず、座標は毎フレーム式から算出する

            pEnemyShot->kind = img_enemyShotMediumBall[2]; // 緑：まだ燃えていない線香

            pEnemyShot->param_i[0] = i;                       // 螺旋上の並び順（0=中心寄り, N-1=外周端）
            pEnemyShot->param_d[0] = theta;                   // 角度
            pEnemyShot->param_d[1] = r;                       // 完成時の半径
            pEnemyShot->param_d[2] = i * SPIRAL_BUILD_STEP;   // 登場フレーム（このShotSetのcount基準）
            pEnemyShot->param_d[3] = -1.0;
            pEnemyShot->param_d[4] = -1.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 燃え尽き予告の開始タイミングでチャージ音を1回だけ鳴らす
    if (pEnemyShotSet->count == SPIRAL_BUILD_DONE + SPIRAL_BURN_TOTAL) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 現在の燃焼フロント位置（外周端 index=N-1 から中心 index=0 へ向かって進む）
    double tBurn = (double)pEnemyShotSet->count - (double)SPIRAL_BUILD_DONE;
    int burnFrontIndex;
    if (tBurn < 0.0) {
        burnFrontIndex = SPIRAL_N; // まだ点火前：誰も燃えていない
    }
    else {
        burnFrontIndex = (SPIRAL_N - 1) - (int)floor(tBurn / SPIRAL_BURN_INTERVAL);
    }

    bool finale = (pEnemyShotSet->count >= SPIRAL_FINALE_LOCAL);
    bool warning = (!finale) && (pEnemyShotSet->count >= SPIRAL_BUILD_DONE + SPIRAL_BURN_TOTAL);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int    i     = pShot->param_i[0];
        double theta = pShot->param_d[0];
        double r     = pShot->param_d[1];
        double appearFrame = pShot->param_d[2];

        if (finale) {
            // フェーズ4：燃え尽き・灰散。弾自身の元の角度をそのまま飛散方向に利用し、
            // 中心から放射状に二次関数的加速で吹き飛ぶ。
            double elapsed = (double)pEnemyShotSet->count - (double)SPIRAL_FINALE_LOCAL;
            double burstR = SPIRAL_BURST_ACCEL * elapsed * elapsed;

            if (pShot->param_d[3] < 0.0) {
                pShot->param_d[3] = pShot->x;
                pShot->param_d[4] = pShot->y;
            }

            pShot->x = pShot->param_d[3] + burstR * cos(theta);
            pShot->y = pShot->param_d[4] + burstR * sin(theta);
            pShot->muki = theta;

            int colorIdx = (((pEnemyShotSet->count - SPIRAL_FINALE_LOCAL) / 4) % 2 == 0) ? 0 : 8; // 赤/橙交互
            pShot->kind = img_enemyShotMediumBall[colorIdx];
        }
        else {
            // 登場イージング：中心(r=0)から完成半径rへ滑らかに伸びる
            double revealProgress = (pEnemyShotSet->count - appearFrame) / SPIRAL_REVEAL_EASE;
            if (revealProgress < 0.0) revealProgress = 0.0;
            if (revealProgress > 1.0) revealProgress = 1.0;
            double curR = r * revealProgress;

            double drawX = SPIRAL_CENTER_X + curR * cos(theta);
            double drawY = SPIRAL_CENTER_Y + curR * sin(theta);

            if (i > burnFrontIndex) {
                // 既に燃え尽きた区間：灰となり、ゆらぎながらゆっくり立ち上って自然に画面外へ消えていく
                double burntAt = (double)SPIRAL_BUILD_DONE + (double)(SPIRAL_N - i) * SPIRAL_BURN_INTERVAL;
                double ashElapsed = (double)pEnemyShotSet->count - burntAt;
                if (ashElapsed < 0.0) ashElapsed = 0.0;

                drawX += 6.0 * sin(0.05 * ashElapsed + i * 0.3);
                drawY -= 0.15 * ashElapsed;

                pShot->kind = img_enemyShotMediumBall[7]; // 黒：灰
            }
            else if (i == burnFrontIndex) {
                // 燃焼点：橙と赤を点滅させて点火の目印にする
                int colorIdx = ((pEnemyShotSet->count / 3) % 2 == 0) ? 8 : 0;
                pShot->kind = img_enemyShotMediumBall[colorIdx];
            }
            else {
                // まだ燃えていない区間：緑。燃え尽き直前は白フラッシュで警告
                if (warning && ((pEnemyShotSet->count / 4) % 2 == 0)) {
                    pShot->kind = img_enemyShotMediumBall[6]; // 白
                }
                else {
                    pShot->kind = img_enemyShotMediumBall[2]; // 緑
                }
            }

            pShot->x = drawX;
            pShot->y = drawY;
            pShot->muki = theta;
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：立ち上る煙（燃焼点から継続発生、ゆらぎながら上昇）
// ============================================================
static void ShotSmoke(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int n = 5 + GetRand(2); // 5～7本
        for (int i = 0; i < n; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = -DX_PI / 2.0;
            pEnemyShot->speed = 0.0;

            pEnemyShot->kind = img_enemyShotScale[6]; // 白系の鱗弾で煙を表現

            pEnemyShot->param_d[0] = pEnemyShotSet->x;                          // 基準x
            pEnemyShot->param_d[1] = pEnemyShotSet->y;                          // 基準y
            pEnemyShot->param_d[2] = 0.6 + GetRand(100) / 100.0;                // 上昇速度
            pEnemyShot->param_d[3] = 8.0 + GetRand(12);                         // 横揺れ幅
            pEnemyShot->param_d[4] = 0.03 + (GetRand(200) - 100) / 4000.0;      // 揺れ周波数
            pEnemyShot->param_d[5] = GetRand(628) / 100.0;                      // 位相

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double baseX     = pShot->param_d[0];
        double baseY     = pShot->param_d[1];
        double riseSpeed = pShot->param_d[2];
        double amp       = pShot->param_d[3];
        double freq      = pShot->param_d[4];
        double phase     = pShot->param_d[5];

        double t = (double)pShot->count;
        pShot->x = baseX + amp * sin(freq * t + phase);
        pShot->y = baseY - riseSpeed * t;

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：燃焼点からの自機狙い3way（火の粉）
// ============================================================
static void ShotEmberAimed(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double spread[3] = { -0.16, 0.0, 0.16 };

        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseMuki + spread[i];
            pEnemyShot->speed = 2.6;
            pEnemyShot->kind = img_enemyShotBullet[8]; // 橙：火の粉

            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->speed * t * cos(pShot->muki);
        pShot->y = pShot->param_d[1] + pShot->speed * t * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：香りの波紋（コイル中心から低速で広がる同心リング）
// ============================================================
static void ShotAromaRing(sEnemyShotSet* pEnemyShotSet)
{
    const int RING_N = 28;

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < RING_N; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double ang = DX_PI * 2.0 * i / RING_N;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 0.9;
            pEnemyShot->kind = img_enemyShotScale[3]; // シアン：香りの波紋

            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->speed * t * cos(pShot->muki);
        pShot->y = pShot->param_d[1] + pShot->speed * t * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：燃え尽きフィナーレの自機狙い5way
// ============================================================
static void Shot5WayAimed(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        for (int i = -2; i <= 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseMuki + i * 0.14;
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤の大玉

            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->speed * t * cos(pShot->muki);
        pShot->y = pShot->param_d[1] + pShot->speed * t * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン：蚊遣り渦焼香
// ============================================================
void EnemyPat_MosquitoCoil_Claude()
{
    static int muki;
    static int spiralSpawnCount; // 現在の渦巻きコイルを生成した時点の count
    static int loopIndex;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        spiralSpawnCount = -1;
        loopIndex = 0;
    }
    else {
        enemy.x += 0.4 * (double)muki;
        if (count % 200 == 100) muki *= -1;
    }

    // ---- コイル本体（渦巻き）の生成・再生成 ----
    // フィナーレの飛散が十分収まったら次のコイルを生成し、無限ループさせる
    if (spiralSpawnCount < 0 || count - spiralSpawnCount >= SPIRAL_FINALE_LOCAL + 120) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiralCoil;
        pEnemyShotSet->x = SPIRAL_CENTER_X;
        pEnemyShotSet->y = SPIRAL_CENTER_Y;
        pEnemyShotSet->kind = loopIndex++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        spiralSpawnCount = count;
    }

    int localCount = count - spiralSpawnCount;

    // ---- 燃焼中：煙・火の粉・香りの波紋を継続的に発生 ----
    bool burning = (localCount >= SPIRAL_BUILD_DONE) && (localCount < SPIRAL_BUILD_DONE + SPIRAL_BURN_TOTAL);
    if (burning) {
        double tBurnNow = (double)(localCount - SPIRAL_BUILD_DONE);
        double burnFrontIndexNow = (double)(SPIRAL_N - 1) - floor(tBurnNow / SPIRAL_BURN_INTERVAL);
        if (burnFrontIndexNow < 0.0) burnFrontIndexNow = 0.0;

        double theta = burnFrontIndexNow * SPIRAL_DTHETA;
        double r = SPIRAL_A * theta;
        double emberX = SPIRAL_CENTER_X + r * cos(theta);
        double emberY = SPIRAL_CENTER_Y + r * sin(theta);

        // 煙：燃焼点から継続的に立ち上らせる
        if (localCount % 6 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSmoke;
            pEnemyShotSet->x = emberX;
            pEnemyShotSet->y = emberY;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        // 火の粉：燃焼点からの自機狙い3way
        if (localCount % 55 == 20) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotEmberAimed;
            pEnemyShotSet->x = emberX;
            pEnemyShotSet->y = emberY;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        // 香りの波紋：コイル中心から低速で広がるリング
        if (localCount % 90 == 45) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotAromaRing;
            pEnemyShotSet->x = SPIRAL_CENTER_X;
            pEnemyShotSet->y = SPIRAL_CENTER_Y;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }

    // ---- 燃え尽き：自機狙い5wayを一度だけ発射 ----
    if (localCount == SPIRAL_FINALE_LOCAL) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = Shot5WayAimed;
        pEnemyShotSet->x = SPIRAL_CENTER_X;
        pEnemyShotSet->y = SPIRAL_CENTER_Y;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}