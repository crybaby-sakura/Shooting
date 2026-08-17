// enemyPat_ZankouChouyakuJin.cpp
// 残光跳躍陣（ざんこうちょうやくじん）
// ボスが瞬間移動を繰り返し、跳躍元に「残光（独自周期で撃ち続ける発射源）」を
// 置き去りにしていく弾幕。跳躍が進むほど画面上の発射源が増えていき、
// 全跳躍完了後はボス本体＋全残光が安全回廊を共有した収束リングで同期一斉解放する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 定数
// ============================================================
static const int    NUM_JUMPS = 6;    // 跳躍回数
static const int    TELEPORT_CYCLE = 90;   // 1跳躍あたりのフレーム数
static const int    TELEPORT_FRAME = 31;   // サイクル内で跳躍が起きるフレーム
static const int    TELEGRAPH_INTERVAL = 10;   // 予兆パルスの間隔
static const int    RESIDUAL_INTERVAL = 70;   // 残光の発射間隔
static const int    FINALE_WAVE_INTERVAL = 50;
static const int    FINALE_WAVE_COUNT = 3;
static const int    FINALE_DURATION = FINALE_WAVE_INTERVAL * FINALE_WAVE_COUNT + 60;
static const double GAP_HALF_WIDTH = 22.0 / 180.0 * DX_PI; // 収束リングの安全回廊の半幅

// 瞬間移動先の候補座標（画面は480x480）
static const double TELEPORT_POINTS[][2] = {
    {240.0,  60.0},
    { 70.0,  90.0},
    {410.0,  90.0},
    { 90.0, 260.0},
    {390.0, 260.0},
    {240.0, 160.0},
};
static const int TELEPORT_POINT_COUNT = 6;

// ============================================================
// 弾幕：予兆パルス（跳躍前、その場で小さく脈打つリング）
// ============================================================
static void TelegraphPulse(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyCharge)) PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        int n = 8;
        for (int i = 0; i < n; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = 2.0 * DX_PI * i / n;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->param_d[0] = angle; // 発射角度
            pEnemyShot->param_d[1] = 0.9;   // 拡散速度
            pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double r = 12.0 + pShot->param_d[1] * t;
        pShot->x = pEnemyShotSet->x + r * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y + r * sin(pShot->param_d[0]);
        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕：残光パルス（跳躍元に居残り、独自周期でリングを撃ち続ける）
// param_i[1] = 跳躍インデックス（値が大きいほど密度が濃い＝後から出た残光ほど強い）
// ============================================================
static void ResidualPulse(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    static const int tierColor[] = { 3, 4, 5, 1, 8, 0 }; // シアン→青→マゼンタ→黄→橙→赤

    if (pEnemyShotSet->count % RESIDUAL_INTERVAL == 0 &&
            (pEnemyShotSet->param_i[2] == 0 || count < pEnemyShotSet->param_i[2])) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int tier = pEnemyShotSet->param_i[1];
        if (tier < 0) tier = 0;
        if (tier > 5) tier = 5;
        int n = 12 + tier * 4; // 跳躍段階が進むほど濃くなる
        int wave = pEnemyShotSet->count / RESIDUAL_INTERVAL;
        double baseAngle = (2.0 * DX_PI / n) * 0.5 * wave; // 発射のたびに半歩ずつ角度をずらす

        for (int i = 0; i < n; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = baseAngle + 2.0 * DX_PI * i / n;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = 1.3 + tier * 0.15;    // 段階が進むほど速い
            pEnemyShot->param_i[0] = pEnemyShotSet->count; // このシリーズの基準時刻
            pEnemyShot->kind = img_enemyShotSmallBall[tierColor[tier]];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)(pEnemyShotSet->count - pShot->param_i[0]);
        double r = pShot->param_d[1] * t;
        pShot->x = pEnemyShotSet->x + r * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y + r * sin(pShot->param_d[0]);
        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕：着地際の奇襲（新しい座標に出現した瞬間、自機狙い3way）
// ============================================================
static void LandingBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double spread = 18.0 / 180.0 * DX_PI;
        for (int i = -1; i <= 1; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = pEnemyShotSet->muki + spread * i;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = 3.0;
            pEnemyShot->kind = img_enemyShotBullet[8]; // 橙

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double r = pShot->param_d[1] * t;
        pShot->x = pEnemyShotSet->x + r * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y + r * sin(pShot->param_d[0]);
        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕：収束リング（フィナーレ。全発射源が安全回廊を共有して一斉解放）
// param_d[0] = このセット内で固定の安全回廊中心角度
// ============================================================
static void ConvergenceRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int n = 40;
        double safeAngle = pEnemyShotSet->param_d[0];
        for (int i = 0; i < n; i++) {
            double angle = 2.0 * DX_PI * i / n;
            double diff = fmod(angle - safeAngle + 3.0 * DX_PI, 2.0 * DX_PI) - DX_PI;
            if (fabs(diff) < GAP_HALF_WIDTH) continue; // 安全回廊部分は弾を撃たない

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = 2.4;
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double r = pShot->param_d[1] * t;
        pShot->x = pEnemyShotSet->x + r * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y + r * sin(pShot->param_d[0]);
        pShot = pShot->next;
    }
}

// ============================================================
// 内部ヘルパー：新しいショットセットを生成して登録する
// ============================================================
static sEnemyShotSet* SpawnShotSet(double x, double y, double muki, sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    return pEnemyShotSet;
}

// ============================================================
// 敵本体：残光跳躍陣
// ============================================================
void EnemyPat_Warp_Claude()
{
    static int    jumpIndex;
    static int    cycleBase;
    static int    currentPosIndex;
    static double residualX[NUM_JUMPS];
    static double residualY[NUM_JUMPS];
    static int    residualCount;
    static sEnemyShotSet* residualSet[NUM_JUMPS];
    static int    phaseState;      // 0:跳躍サイクル中 1:フィナーレ中
    static int    finaleStartCount;

    if (count == 1) {
        currentPosIndex = 0;
        enemy.x = TELEPORT_POINTS[currentPosIndex][0];
        enemy.y = TELEPORT_POINTS[currentPosIndex][1];
        enemy.maxHp = enemy.hp = 200;

        jumpIndex = 0;
        cycleBase = 1;
        residualCount = 0;
        phaseState = 0;
        finaleStartCount = 0;
        return;
    }

    if (phaseState == 0) {
        int localFrame = count - cycleBase + 1;

        // 予兆パルス（跳躍直前まで一定間隔で）
        if (localFrame >= 1 && localFrame <= TELEPORT_FRAME - 1 &&
            (localFrame - 1) % TELEGRAPH_INTERVAL == 0) {
            SpawnShotSet(enemy.x, enemy.y, 0.0, TelegraphPulse);
        }

        // 瞬間移動
        if (localFrame == TELEPORT_FRAME) {
            // 跳躍元に残光を配置
            sEnemyShotSet* pResidual = SpawnShotSet(enemy.x, enemy.y, 0.0, ResidualPulse);
            pResidual->param_i[1] = jumpIndex;
            if (residualCount < NUM_JUMPS) {
                residualX[residualCount] = enemy.x;
                residualY[residualCount] = enemy.y;
                residualSet[residualCount] = pResidual;
                residualCount++;
            }

            // 次の瞬間移動先を決定（現在地とは異なる候補をリプレイ安全な乱数で選ぶ）
            int nextIndex;
            do {
                nextIndex = GetRand(TELEPORT_POINT_COUNT - 1);
            } while (nextIndex == currentPosIndex);
            currentPosIndex = nextIndex;
            enemy.x = TELEPORT_POINTS[currentPosIndex][0];
            enemy.y = TELEPORT_POINTS[currentPosIndex][1];

            // 着地際の奇襲（自機狙い3way）
            double aimAngle = atan2(player.y - enemy.y, player.x - enemy.x);
            SpawnShotSet(enemy.x, enemy.y, aimAngle, LandingBurst);

            jumpIndex++;
        }

        // サイクル終了判定
        if (localFrame == TELEPORT_CYCLE) {
            if (jumpIndex >= NUM_JUMPS) {
                phaseState = 1;
                finaleStartCount = count + 1;
                for (int i = 0; i < residualCount; i++) {
                    residualSet[i]->param_i[2] = finaleStartCount;
                }
            }
            else {
                cycleBase = count + 1;
            }
        }
    }
    else {
        int finaleFrame = count - finaleStartCount + 1;

        for (int w = 0; w < FINALE_WAVE_COUNT; w++) {
            if (finaleFrame == 1 + w * FINALE_WAVE_INTERVAL) {
                double safeAngle = (2.0 * DX_PI / 3.0) * w; // 波ごとに安全回廊が120度回転
                for (int i = 0; i < residualCount; i++) {
                    sEnemyShotSet* pSet = SpawnShotSet(residualX[i], residualY[i], 0.0, ConvergenceRing);
                    pSet->param_d[0] = safeAngle;
                }
                sEnemyShotSet* pBossSet = SpawnShotSet(enemy.x, enemy.y, 0.0, ConvergenceRing);
                pBossSet->param_d[0] = safeAngle;
            }
        }

        // フィナーレ終了、次の周へループ
        if (finaleFrame >= FINALE_DURATION) {
            phaseState = 0;
            jumpIndex = 0;
            residualCount = 0;
            cycleBase = count + 1;
        }
    }
}