// enemyPat_shuseiKyougetsu.cpp
//
// 「衆星拱月（しゅうせいきょうげつ）」
// エビングハウス錯視（同じ大きさの円でも、周囲を大きい円で囲むと小さく見え、
// 小さい円で囲むと大きく見える）をモチーフにした弾幕。
//
// ■ 構成
// Phase1+2（提示→拡散）：左右に同一サイズの核弾を配置し、片方は大玉、
//   もう片方は小玉のリングで取り囲んで静止呈示。その後リングは外側へ加速拡散する。
// Phase3（ゲート回廊・本体トリック）：実際の通過可能な隙間幅は全ゲート共通だが、
//   隙間の縁を大玉／小玉のどちらで装飾するかで見た目の広さが変わる列を降らせる。
// Phase4（開示・収束バースト）：核弾を一列に並べて全て同一サイズであることを
//   明滅で開示したのち、自機狙いの扇状弾を一斉発射してフィナーレとする。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  共通ヘルパー
// ============================================================
static sEnemyShot* AddShot(sEnemyShotSet* pSet, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->kind = kind;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

static sEnemyShotSet* NewShotSet(double x, double y, int kind, sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->x = x;
    pSet->y = y;
    pSet->kind = kind;
    pSet->patternFunc = func;

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
//  Phase1+2：呈示リング → 拡散バースト
// ============================================================
static const int    RING_SAT_NUM = 8;     // 衛星弾の数
static const double RING_HOLD = 150.0; // 静止呈示フレーム数
static const double RING_R = 40.0;  // リング半径（呈示中は固定）

static void ShotIllusionRing(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double coreX = pSet->x;
        double coreY = pSet->y;
        // kind: 0=大玉(橙)で囲む→中心が小さく見える側 / 1=小玉(シアン)で囲む→中心が大きく見える側
        int accentKind = (pSet->kind == 0) ? img_enemyShotLargeBall[8] : img_enemyShotSmallBall[3];

        // 核弾（左右で完全に同一サイズ・同一色）
        sEnemyShot* pCore = AddShot(pSet, img_enemyShotMediumBall[6]); // 白
        pCore->param_i[0] = 0; // role: core
        pCore->param_d[0] = coreX;
        pCore->param_d[1] = coreY;
        pCore->x = coreX;
        pCore->y = coreY;

        // 周囲を取り囲む衛星弾
        for (int i = 0; i < RING_SAT_NUM; i++) {
            sEnemyShot* pSat = AddShot(pSet, accentKind);
            double angle0 = i * (2.0 * DX_PI / RING_SAT_NUM);
            pSat->param_i[0] = 1; // role: satellite
            pSat->param_d[0] = coreX;
            pSat->param_d[1] = coreY;
            pSat->param_d[2] = angle0;
            pSat->param_d[3] = RING_R;
            pSat->x = coreX + RING_R * cos(angle0);
            pSat->y = coreY + RING_R * sin(angle0);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        if (pShot->param_i[0] == 0) {
            // 核弾：呈示中は静止、その後ゆっくり画面上へ退場
            pShot->x = pShot->param_d[0];
            if (t < RING_HOLD) {
                pShot->y = pShot->param_d[1];
            }
            else {
                pShot->y = pShot->param_d[1] - (t - RING_HOLD) * 2.0;
            }
        }
        else {
            // 衛星弾：呈示中はゆっくり回転、その後は最終角度のまま外側へ加速拡散
            double cx = pShot->param_d[0];
            double cy = pShot->param_d[1];
            double angle0 = pShot->param_d[2];
            double r0 = pShot->param_d[3];
            if (t < RING_HOLD) {
                double angle = angle0 + t * 0.02;
                pShot->x = cx + r0 * cos(angle);
                pShot->y = cy + r0 * sin(angle);
            }
            else {
                double angleHold = angle0 + RING_HOLD * 0.02;
                double dt = t - RING_HOLD;
                double r = r0 + dt * 2.5 + dt * dt * 0.02;
                pShot->x = cx + r * cos(angleHold);
                pShot->y = cy + r * sin(angleHold);
            }
        }
        pShot = pShot->next;
    }
}

static void SpawnRing(double x, double y, int accentMode)
{
    NewShotSet(x, y, accentMode, ShotIllusionRing);
}

// ============================================================
//  Phase3：ゲート回廊（本体トリック）
// ============================================================
static const double GATE_GAP_W = 70.0; // 実際の通過可能幅（全ゲート共通・固定）
static const double GATE_WALL_SPACING = 9.0;
static const double GATE_DESCEND_SPEED = 1.5;

static void ShotGate(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int accentMode = pSet->kind; // 0:大玉で縁取り→隙間が狭く見える / 1:小玉で縁取り→隙間が広く見える
        double gapX = pSet->x;
        double gapY = pSet->y;

        // 実際の当たり判定を構成する壁（縁取りの見た目に関わらず全ゲート共通の隙間幅）
        for (double x = 0.0; x <= gapX - GATE_GAP_W * 0.5 - GATE_WALL_SPACING * 0.5; x += GATE_WALL_SPACING) {
            sEnemyShot* pWall = AddShot(pSet, img_enemyShotMediumBall[6]); // 白
            pWall->param_i[0] = 0; // role: wall
            pWall->param_d[0] = x;
            pWall->param_d[1] = gapY;
            pWall->x = x;
            pWall->y = gapY;
        }
        for (double x = gapX + GATE_GAP_W * 0.5 + GATE_WALL_SPACING * 0.5; x <= 480.0; x += GATE_WALL_SPACING) {
            sEnemyShot* pWall = AddShot(pSet, img_enemyShotMediumBall[6]);
            pWall->param_i[0] = 0;
            pWall->param_d[0] = x;
            pWall->param_d[1] = gapY;
            pWall->x = x;
            pWall->y = gapY;
        }

        // 見た目の隙間感だけを操作する縁取り弾（当たり判定上の隙間幅には影響しない演出弾）
        double edgeX[2] = { gapX - GATE_GAP_W * 0.5, gapX + GATE_GAP_W * 0.5 };
        if (accentMode == 0) {
            // 大玉4個を隙間の四隅に密着させる → 隙間が狭く見える
            double offY[2] = { -14.0, 14.0 };
            for (int e = 0; e < 2; e++) {
                double offX = (e == 0) ? -14.0 : 14.0;
                for (int i = 0; i < 2; i++) {
                    sEnemyShot* pAcc = AddShot(pSet, img_enemyShotLargeBall[8]); // 橙
                    pAcc->param_i[0] = 1; // role: accent
                    pAcc->param_d[0] = edgeX[e] + offX;
                    pAcc->param_d[1] = gapY + offY[i];
                    pAcc->param_d[2] = GetRand(628) / 100.0; // 演出用の揺れ位相
                    pAcc->x = pAcc->param_d[0];
                    pAcc->y = pAcc->param_d[1];
                }
            }
        }
        else {
            // 小玉8個を隙間の縁より外側へ広く散らす → 隙間が広く見える
            double offY[4] = { -26.0, -9.0, 9.0, 26.0 };
            for (int e = 0; e < 2; e++) {
                double offX = (e == 0) ? -26.0 : 26.0;
                for (int i = 0; i < 4; i++) {
                    sEnemyShot* pAcc = AddShot(pSet, img_enemyShotSmallBall[3]); // シアン
                    pAcc->param_i[0] = 1;
                    pAcc->param_d[0] = edgeX[e] + offX;
                    pAcc->param_d[1] = gapY + offY[i];
                    pAcc->param_d[2] = GetRand(628) / 100.0;
                    pAcc->x = pAcc->param_d[0];
                    pAcc->y = pAcc->param_d[1];
                }
            }
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->y = pShot->param_d[1] + t * GATE_DESCEND_SPEED;
        if (pShot->param_i[0] == 0) {
            pShot->x = pShot->param_d[0]; // 壁は左右にぶれず真の隙間幅を保つ
        }
        else {
            pShot->x = pShot->param_d[0] + 4.0 * sin(t * 0.05 + pShot->param_d[2]); // 縁取り弾は軽く揺れて演出
        }
        pShot = pShot->next;
    }
}

static void SpawnGate(double gapX, double gapY, int accentMode)
{
    NewShotSet(gapX, gapY, accentMode, ShotGate);
}

// ============================================================
//  Phase4：開示（核弾整列・明滅）→ 収束バースト
// ============================================================
static const int    FINALE_NUM = 17;
static const double FINALE_HOLD = 60.0;

static void ShotFinaleCore(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < FINALE_NUM; i++) {
            double x = 80.0 + i * (320.0 / (FINALE_NUM - 1));
            sEnemyShot* pCore = AddShot(pSet, img_enemyShotMediumBall[6]);
            pCore->param_d[0] = x;
            pCore->param_d[1] = pSet->y;
            pCore->x = x;
            pCore->y = pSet->y;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0];
        if (t < FINALE_HOLD) {
            // 全ての核弾が同一サイズであることを示す明滅演出（色のみ切替）
            pShot->kind = (((int)t / 10) % 2 == 0) ? img_enemyShotMediumBall[6] : img_enemyShotMediumBall[1];
            pShot->y = pShot->param_d[1];
        }
        else {
            pShot->y = pShot->param_d[1] - (t - FINALE_HOLD) * 2.5;
        }
        pShot = pShot->next;
    }
}

static void SpawnFinaleCore(double x, double y)
{
    NewShotSet(x, y, 0, ShotFinaleCore);
}

static void ShotAimedFan(sEnemyShotSet* pSet)
{
    const int    FAN_NUM = 9;
    const double FAN_SPREAD = 60.0 * DX_PI / 180.0;
    const double FAN_SPEED = 2.6;

    if (pSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < FINALE_NUM; i++) {
            double x0 = 80.0 + i * (320.0 / (FINALE_NUM - 1));
            double y0 = pSet->y;
            double baseMuki = atan2(player.y - y0, player.x - x0);
            for (int f = 0; f < FAN_NUM; f++) {
                double muki = baseMuki + (f - (FAN_NUM - 1) / 2.0) * (FAN_SPREAD / (FAN_NUM - 1));
                sEnemyShot* pFan = AddShot(pSet, img_enemyShotBullet[1]); // 黄
                pFan->muki = muki;
                pFan->speed = FAN_SPEED;
                pFan->param_d[0] = x0;
                pFan->param_d[1] = y0;
                pFan->x = x0;
                pFan->y = y0;
            }
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->speed * cos(pShot->muki) * t;
        pShot->y = pShot->param_d[1] + pShot->speed * sin(pShot->muki) * t;
        pShot = pShot->next;
    }
}

static void SpawnAimedFan(double y)
{
    NewShotSet(240.0, y, 0, ShotAimedFan);
}

// ============================================================
//  敵本体パターン：衆星拱月
// ============================================================
void EnemyPat_Ebbinghaus_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    else {
        enemy.x = 240.0 + 120.0 * sin(count * 0.006);
        enemy.y = 60.0 + 10.0 * sin(count * 0.014);
    }

    // ---- Phase1+2：呈示 → 拡散（左右で衛星弾の大小のみ異なる） ----
    if (count % 800 == 1) {
        SpawnRing(150.0, 140.0, 0); // 左：大玉で囲む→小さく見える
        SpawnRing(330.0, 140.0, 1); // 右：小玉で囲む→大きく見える
    }

    // ---- Phase3：ゲート回廊（本体トリック、6波） ----
    static const int GATE_TIMES[] = { 180, 240, 300, 360, 420, 480 };
    for (int i = 0; i < 6; i++) {
        if (count % 800 == GATE_TIMES[i]) {
            int accentMode = GetRand(1);           // 0 or 1
            double gapX = 150.0 + GetRand(180);     // 150〜330
            SpawnGate(gapX, -20.0, accentMode);
        }
    }

    // ---- Phase4：開示・収束バースト ----
    if (count % 700 == 540) {
        SpawnFinaleCore(240.0, 200.0);
    }
    if (count % 700 == 540 + (int)FINALE_HOLD) {
        SpawnAimedFan(200.0);
    }
}