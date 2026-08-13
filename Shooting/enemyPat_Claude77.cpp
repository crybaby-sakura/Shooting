// enemyPat_ItokuriRanbu.cpp
// 「糸繰乱舞」 ヨーヨーをモチーフにした4フェーズ無限ループパターン
//
//   フェーズ1 投げ下ろし         : 本体(ヨーヨー)が紐に沿って左右に揺れながら降下する
//   フェーズ2 スリーピング       : 本体がその場で高速自転を続け、接線方向へ弾を撒き散らす
//   フェーズ3 アラウンド・ザ・ワールド : 本体が大きな円軌道で振り回され、
//                                       通過のたびに火花を残し、1周ごとに自機狙い3wayを発射
//   フェーズ4 巻き戻し・帰還     : 本体が加速しながら手元へ戻り、
//                                       帰還した瞬間に全方位バースト＋自機狙い5wayで締める
//
// 位置はすべて count(グローバルフレーム数) からの純粋な計算式で求めており、
// 速度の積算(velocity integration)は一切行っていない。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  フェーズ構成
// ============================================================
static const int    PHASE1_LEN = 80;    // 投げ下ろし
static const int    PHASE2_LEN = 160;   // スリーピング
static const int    PHASE3_LEN = 260;   // アラウンド・ザ・ワールド
static const int    PHASE4_LEN = 70;    // 巻き戻し・帰還
static const int    CYCLE_LEN = PHASE1_LEN + PHASE2_LEN + PHASE3_LEN + PHASE4_LEN;

static const double ANCHOR_OFFSET_Y = 20.0;   // 「手元」(アンカー)の敵本体からの下方オフセット
static const double DROP_DIST = 260.0;  // フェーズ1で落下する距離
static const double ORBIT_CENTER_DY = 150.0;  // フェーズ3の軌道中心の、手元からの下方オフセット
static const double ORBIT_RADIUS = 130.0;  // フェーズ3の軌道半径
static const double ORBIT_RAMP = 60.0;   // 軌道半径が0から最大まで立ち上がるフレーム数
static const double ORBIT_ANG_SPD = 0.05;   // フェーズ3の角速度[rad/frame]

static const int STRING_N = 18; // 紐を構成する弾の数
static const int RING_N = 20; // 本体(リング)を構成する弾の数

// ============================================================
//  「その瞬間の手元(アンカー)・本体(ヨーヨー)の状態」をまとめて計算する
// ============================================================
struct YoyoState {
    double anchorX, anchorY;   // 手元の位置
    double bodyX, bodyY;       // 本体(ヨーヨー)の位置
    double spinAngle;          // 本体の自転角
    double ringRadius;         // 本体の半径
    int    phase;              // 現在のフェーズ(1〜4)
    int    localT;             // フェーズ内での経過フレーム数
};

static double EaseOutSin(double p) { return sin(p * DX_PI / 2.0); }

static YoyoState CalcYoyoState(int globalCount)
{
    YoyoState s{};
    int t = globalCount % CYCLE_LEN;

    s.anchorX = enemy.x;
    s.anchorY = enemy.y + ANCHOR_OFFSET_Y;

    if (t < PHASE1_LEN) {
        // ---- フェーズ1: 投げ下ろし ----
        s.phase = 1;
        s.localT = t;
        double progress = t / (double)PHASE1_LEN;
        double eased = EaseOutSin(progress);
        s.bodyX = s.anchorX + 50.0 * sin(progress * DX_PI * 2.0) * (1.0 - progress);
        s.bodyY = s.anchorY + DROP_DIST * eased;
        s.spinAngle = progress * DX_PI * 4.0;
        s.ringRadius = 10.0 + 4.0 * progress;
    }
    else if (t < PHASE1_LEN + PHASE2_LEN) {
        // ---- フェーズ2: スリーピング ----
        s.phase = 2;
        s.localT = t - PHASE1_LEN;
        s.bodyX = s.anchorX;
        s.bodyY = s.anchorY + DROP_DIST;
        double spinAtP1End = DX_PI * 4.0;
        s.spinAngle = spinAtP1End + s.localT * 0.15;
        s.ringRadius = 18.0 + 2.0 * sin(s.localT * 0.1);
    }
    else if (t < PHASE1_LEN + PHASE2_LEN + PHASE3_LEN) {
        // ---- フェーズ3: アラウンド・ザ・ワールド ----
        s.phase = 3;
        s.localT = t - (PHASE1_LEN + PHASE2_LEN);
        double ocx = s.anchorX;
        double ocy = s.anchorY + ORBIT_CENTER_DY;
        double radius = ORBIT_RADIUS * (s.localT < ORBIT_RAMP ? (s.localT / ORBIT_RAMP) : 1.0);
        double orbitAngle = -DX_PI / 2.0 + ORBIT_ANG_SPD * s.localT;
        s.bodyX = ocx + radius * cos(orbitAngle);
        s.bodyY = ocy + radius * sin(orbitAngle);
        double spinAtP2End = DX_PI * 4.0 + PHASE2_LEN * 0.15;
        s.spinAngle = spinAtP2End + s.localT * 0.2;
        s.ringRadius = 16.0;
    }
    else {
        // ---- フェーズ4: 巻き戻し・帰還 ----
        s.phase = 4;
        s.localT = t - (PHASE1_LEN + PHASE2_LEN + PHASE3_LEN);

        // フェーズ3終了時点(localT3 = PHASE3_LEN)の位置を同じ式で再計算し、始点とする
        double ocx = s.anchorX;
        double ocy = s.anchorY + ORBIT_CENTER_DY;
        double orbitAngle = -DX_PI / 2.0 + ORBIT_ANG_SPD * PHASE3_LEN;
        double startX = ocx + ORBIT_RADIUS * cos(orbitAngle);
        double startY = ocy + ORBIT_RADIUS * sin(orbitAngle);

        double progress = s.localT / (double)PHASE4_LEN;
        double easeIn = progress * progress; // 加速しながら戻る「スナップ」感
        s.bodyX = startX + (s.anchorX - startX) * easeIn;
        s.bodyY = startY + (s.anchorY - startY) * easeIn;

        double spinAtP3End = DX_PI * 4.0 + PHASE2_LEN * 0.15 + PHASE3_LEN * 0.2;
        s.spinAngle = spinAtP3End + s.localT * 0.3;
        s.ringRadius = 16.0 - 4.0 * progress;
    }
    return s;
}

// ============================================================
//  sEnemyShotSet を1つ生成してリストに繋ぐ共通ヘルパー
// ============================================================
static sEnemyShotSet* SpawnShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki = 0.0)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;

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
//  紐(手元 ⇔ 本体 を結ぶ弾列) ： 常駐、param_d[0..3]で毎フレーム更新される
// ============================================================
static void ShotPattern_String(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < STRING_N; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->param_i[0] = i;
            pShot->kind = img_enemyShotScale[6]; // 鱗弾・白
            pShot->x = pSet->x;
            pShot->y = pSet->y;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    double ax = pSet->param_d[0], ay = pSet->param_d[1];
    double bx = pSet->param_d[2], by = pSet->param_d[3];
    double dirAngle = atan2(by - ay, bx - ax);

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double f = pShot->param_i[0] / (double)(STRING_N - 1);
        pShot->x = ax + (bx - ax) * f;
        pShot->y = ay + (by - ay) * f;
        pShot->muki = dirAngle;
        pShot = pShot->next;
    }
}

// ============================================================
//  本体(ヨーヨーのリング) ： 常駐、param_d[2..5]で毎フレーム更新される
// ============================================================
static void ShotPattern_Ring(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < RING_N; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->param_i[0] = i;
            pShot->kind = img_enemyShotMediumBall[8]; // 中玉・橙
            pShot->x = pSet->x;
            pShot->y = pSet->y;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    double cx = pSet->param_d[2], cy = pSet->param_d[3];
    double radius = pSet->param_d[4];
    double spin = pSet->param_d[5];

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double angle = pShot->param_i[0] * (2.0 * DX_PI / RING_N) + spin;
        pShot->x = cx + radius * cos(angle);
        pShot->y = cy + radius * sin(angle);
        pShot->muki = angle;
        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ2 ： スリーピング中に接線方向へ弾く弾(3方向)
// ============================================================
static void ShotPattern_TangentRelease(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int N = 4;
        const double speed = 1.6;
        for (int i = 0; i < N; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = pSet->muki + i * (2.0 * DX_PI / N);
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = speed;
            pShot->kind = img_enemyShotScale[1]; // 鱗弾・黄

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x = pSet->x + pShot->speed * cos(pShot->muki) * pShot->count;
        pShot->y = pSet->y + pShot->speed * sin(pShot->muki) * pShot->count;
        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ3 ： 軌道中心から外向きに飛ぶ火花(1発)
// ============================================================
static void ShotPattern_OrbitSpark(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = pSet->muki;
        pShot->speed = 1.2;
        pShot->kind = img_enemyShotDiamond[3]; // 菱形弾・シアン

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x = pSet->x + pShot->speed * cos(pShot->muki) * pShot->count;
        pShot->y = pSet->y + pShot->speed * sin(pShot->muki) * pShot->count;
        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ3/4 ： 自機狙いNwayの扇状バースト
//  param_i[0] = way数、param_d[0] = 開き角(rad)、param_d[1] = 速さ
// ============================================================
static void ShotPattern_AimedFan(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int N = pSet->param_i[0];
        double spread = pSet->param_d[0];
        double speed = pSet->param_d[1];
        double baseAngle = atan2(player.y - pSet->y, player.x - pSet->x);

        for (int i = 0; i < N; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = baseAngle + (N == 1 ? 0.0 : spread * (i / (double)(N - 1) - 0.5));
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = speed;
            pShot->kind = img_enemyShotMediumOval[0]; // 中楕円弾・赤

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x = pSet->x + pShot->speed * cos(pShot->muki) * pShot->count;
        pShot->y = pSet->y + pShot->speed * sin(pShot->muki) * pShot->count;
        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ4 ： 帰還の瞬間の全方位バースト(48way)
// ============================================================
static void ShotPattern_RadialBurst(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const int N = 48;
        const double speed = 2.0;
        for (int i = 0; i < N; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = i * (2.0 * DX_PI / N);
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = speed;
            pShot->kind = img_enemyShotLargeBall[8]; // 大玉・橙

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x = pSet->x + pShot->speed * cos(pShot->muki) * pShot->count;
        pShot->y = pSet->y + pShot->speed * sin(pShot->muki) * pShot->count;
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Yoyo_Claude()
{
    static sEnemyShotSet* pStringSet = nullptr;
    static sEnemyShotSet* pRingSet = nullptr;
    const int rotFrames = (int)round(2.0 * DX_PI / ORBIT_ANG_SPD); // フェーズ3が1周するフレーム数

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;

        pStringSet = SpawnShotSet(ShotPattern_String, enemy.x, enemy.y + ANCHOR_OFFSET_Y);
        pRingSet = SpawnShotSet(ShotPattern_Ring, enemy.x, enemy.y + ANCHOR_OFFSET_Y);
    }

    // 敵本体はゆるやかに左右へ揺れる
    enemy.x = 240.0 + 30.0 * sin(count * 0.01);

    YoyoState st = CalcYoyoState(count);

    // 紐・本体(リング)の状態を毎フレーム更新
    pStringSet->param_d[0] = st.anchorX;
    pStringSet->param_d[1] = st.anchorY;
    pStringSet->param_d[2] = st.bodyX;
    pStringSet->param_d[3] = st.bodyY;

    pRingSet->param_d[2] = st.bodyX;
    pRingSet->param_d[3] = st.bodyY;
    pRingSet->param_d[4] = st.ringRadius;
    pRingSet->param_d[5] = st.spinAngle;

    // ---- フェーズ1開始の投げ予告音 ----
    if (st.phase == 1 && st.localT == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ---- フェーズ2: スリーピング中、接線方向へ弾を撒く ----
    if (st.phase == 2 && st.localT % 2 == 0) {
        SpawnShotSet(ShotPattern_TangentRelease, st.bodyX, st.bodyY, st.spinAngle + DX_PI / 2.0);
    }

    // ---- フェーズ3: 常時、軌道中心から外向きに火花を飛ばす ----
    if (st.phase == 3 && st.localT % 2 == 0) {
        double ocx = st.anchorX;
        double ocy = st.anchorY + ORBIT_CENTER_DY;
        double sparkAngle = atan2(st.bodyY - ocy, st.bodyX - ocx);
        SpawnShotSet(ShotPattern_OrbitSpark, st.bodyX, st.bodyY, sparkAngle);
    }

    // ---- フェーズ3: 1周ごとに自機狙い3wayを発射 ----
    if (st.phase == 3 && st.localT >= (int)ORBIT_RAMP &&
        (st.localT - (int)ORBIT_RAMP) % rotFrames == 0) {
        sEnemyShotSet* pFan = SpawnShotSet(ShotPattern_AimedFan, st.bodyX, st.bodyY);
        pFan->param_i[0] = 3;             // 3way
        pFan->param_d[0] = DX_PI / 6.0;   // 開き角30度
        pFan->param_d[1] = 2.2;           // 速さ
    }

    // ---- フェーズ4: 手元に戻った瞬間、全方位バースト＋自機狙い5wayで締める ----
    if (st.phase == 4 && st.localT == PHASE4_LEN - 1) {
        SpawnShotSet(ShotPattern_RadialBurst, st.bodyX, st.bodyY);

        sEnemyShotSet* pFan = SpawnShotSet(ShotPattern_AimedFan, st.bodyX, st.bodyY);
        pFan->param_i[0] = 5;             // 5way
        pFan->param_d[0] = DX_PI / 4.0;   // 開き角45度
        pFan->param_d[1] = 2.4;           // 速さ
    }
}