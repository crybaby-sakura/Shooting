// enemyPat_ShuuenShoumei.cpp
//
// 「終焉証明 〜Q.E.D.〜」
// 数学の背理法をモチーフにした、TAS前提の超高難易度弾幕パターン。
// 生存可能な座標を「複数の独立した式が同時に満たされる交点」として定義し、
// 乱数を使わず、互いに素な周期を持つ三角関数の合成のみで先読み不能な弾幕を構成する。
//
// フェーズ構成（フレーム基準）：
//   第1フェーズ「公理」   count [   1, 901) : 4層同心リング。層ごとに独立した位相式で回転
//   第2フェーズ「反証」   count [ 901,1801) : 自機狙い15way扇を発射時点のみ自機座標で更新
//   第3フェーズ「帰謬」   count [1801,2701) : 横5本＋縦5本のコンベア壁。各壁が独立位相でスクロール
//   第4フェーズ「証明終了」count [2701,2881) : 全レイヤーが同時展開された状態でフィナーレバースト
//
// 各レイヤーは一度生成されたら以後ずっと自分自身のpatternFuncで自走し続けるため、
// 第4フェーズの時点では第1〜3フェーズの弾幕が自然に重なり合った状態になる。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 画面・フェーズ定数
// ============================================================
static constexpr double kScreenW = 480.0;
static constexpr double kScreenH = 480.0;
static constexpr double kScreenCx = 240.0;
static constexpr double kScreenCy = 240.0;

static constexpr int kPhase1Start = 1;
static constexpr int kPhase1Len   = 400;   // 公理
static constexpr int kPhase2Start = kPhase1Start + kPhase1Len;
static constexpr int kPhase2Len   = 400;   // 反証
static constexpr int kPhase3Start = kPhase2Start + kPhase2Len;
static constexpr int kPhase3Len   = 400;   // 帰謬
static constexpr int kPhase4Start = kPhase3Start + kPhase3Len;
static constexpr int kPhase4Len   = 0;   // 証明終了

// ============================================================
// 共通ヘルパー：ShotSetをリストへ登録し、頭ノードを初期化する
// ============================================================
static void AttachEnemyShotSet(sEnemyShotSet* pSet)
{
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ============================================================
// 第1フェーズ「公理」：4層同心リング
// ============================================================
static constexpr int    kRingBulletCount = 44 + 10;   // 1層あたりの弾数（残り1スロット分が隙間になる）
static constexpr double kRingConvergeTime = 150.0;
static constexpr double kRingStartRadius  = 340.0;
static constexpr double kRingTargetRadius[4] = { 95.0, 155.0, 215.0, 275.0 };

// 層ごとに互いに素な周期を持つ正弦波を合成し、絶対回転角として使う。
// 値域は概ね[-3,3]radにわたり大きく振動するため、4層の隙間が同時に重なる角度は
// ごく限られたタイミングでしか存在しない。
static double RingGateAngle(int layer, int globalCount)
{
    double t = (double)globalCount;
    switch (layer) {
    case 0: return sin(t / 97.0)  + sin(t / 151.0) + sin(t / 223.0);
    case 1: return sin(t / 113.0) + sin(t / 167.0) + sin(t / 239.0);
    case 2: return sin(t / 131.0) + sin(t / 179.0) + sin(t / 251.0);
    default:return sin(t / 149.0) + sin(t / 191.0) + sin(t / 269.0);
    }
}

static void ShotRingWall(sEnemyShotSet* pSet)
{
    // param_i[0] = layer(0-3)、param_i[1] = 召喚時点のglobalCount
    int layer      = pSet->param_i[0];
    int spawnCount = pSet->param_i[1];

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < kRingBulletCount; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            // 全kRingBulletCount+1スロットのうちkRingBulletCount個だけを埋め、
            // 残り1スロット分（末尾）を恒久的な隙間として空けておく
            double slotAngle = (2.0 * DX_PI) * (double)i / (double)(kRingBulletCount + 1);
            pShot->param_d[0] = slotAngle;
            pShot->kind  = img_enemyShotMediumBall[layer % 8];
            pShot->muki  = slotAngle;
            pShot->speed = 0.0;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->margin = 480;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    int globalCount = spawnCount + pSet->count;
    double phaseOffset = RingGateAngle(layer, globalCount);

    double t = (double)pSet->count;
    double convergeT = (t < kRingConvergeTime) ? t / kRingConvergeTime : 1.0;
    double ease = 1.0 - (1.0 - convergeT) * (1.0 - convergeT); // イーズアウト
    double radius = kRingStartRadius + (kRingTargetRadius[layer] - kRingStartRadius) * ease;

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double angle = pShot->param_d[0] + phaseOffset;
        pShot->x = pSet->x + radius * cos(angle);
        pShot->y = pSet->y + radius * sin(angle);
        pShot->muki = angle;
        pShot = pShot->next;
    }
}

// ============================================================
// 第3フェーズ「帰謬」：横5本＋縦5本のコンベア壁（碁盤目状）
// ============================================================
static constexpr int kGridSlotsPerLine = 26 + 13;  // 1本あたりの仮想スロット数（1スロット分が隙間）
static constexpr int kGridLineCount    = 5;

// 壁ごとに独立した周期のスクロール位相を返す（[0,1)相当、稀に僅かにはみ出すがfracで吸収）
static double GridGatePhase(int lineIdx, int globalCount)
{
    static const double periodsA[kGridLineCount] = { 173.0, 197.0, 211.0, 229.0, 241.0 };
    static const double periodsB[kGridLineCount] = { 281.0, 307.0, 331.0, 353.0, 379.0 };
    int idx = lineIdx % kGridLineCount;
    double t = (double)globalCount;
    double raw = sin(t / periodsA[idx]) + sin(t / periodsB[idx]); // 値域[-2,2]
    return 0.5 + 0.25 * raw;
}

static void ShotGridLine(sEnemyShotSet* pSet)
{
    // param_i[0] = orientation(0:横壁/x方向スクロール, 1:縦壁/y方向スクロール)
    // param_i[1] = lineIdx(0-4)
    // param_i[2] = 召喚時点のglobalCount
    // param_d[0] = 壁の固定座標（横壁ならy、縦壁ならx）
    int orientation = pSet->param_i[0];
    int lineIdx     = pSet->param_i[1];
    int spawnCount  = pSet->param_i[2];
    double fixedPos = pSet->param_d[0];

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 1; i < kGridSlotsPerLine; i++) { // i=0は隙間スロットとして空ける
            sEnemyShot* pShot = new sEnemyShot;
            pShot->param_d[0] = (double)i / (double)kGridSlotsPerLine; // 固定スロット位置 u_i
            pShot->kind  = (orientation == 0) ? img_enemyShotDiamond[6] : img_enemyShotDiamond[7]; // 横:白 縦:黒
            pShot->muki  = (orientation == 0) ? 0.0 : (DX_PI / 2.0);
            pShot->speed = 0.0;
            pShot->x = 0.0;
            pShot->y = 0.0;
            pShot->margin = 480;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    int globalCount = spawnCount + pSet->count;
    double phase = GridGatePhase(lineIdx, globalCount);

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double u = pShot->param_d[0] + phase;
        u -= floor(u); // frac(u) → [0,1)を周回スクロール（コンベア方式）

        if (orientation == 0) {
            pShot->x = u * kScreenW;
            pShot->y = fixedPos;
        } else {
            pShot->x = fixedPos;
            pShot->y = u * kScreenH;
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 第2フェーズ「反証」：発射時点のみ自機座標を参照する15way扇
// ============================================================
static constexpr int    kFanBulletCount = 15;
static constexpr double kFanSpreadRad   = 0.9;
static constexpr double kFanSpeed       = 3.6;

static void ShotAimedFan(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < kFanBulletCount; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double offset = kFanSpreadRad * ((double)i / (double)(kFanBulletCount - 1) - 0.5);
            double angle = pSet->muki + offset; // 発射時点の角度を固定
            pShot->param_d[0] = angle;
            pShot->kind  = img_enemyShotLaser[8]; // 橙
            pShot->muki  = angle;
            pShot->speed = kFanSpeed;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->margin = 480;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    double t = (double)pSet->count;
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double angle = pShot->param_d[0];
        pShot->x = pSet->x + kFanSpeed * cos(angle) * t;
        pShot->y = pSet->y + kFanSpeed * sin(angle) * t;
        pShot = pShot->next;
    }
}

// ============================================================
// 第4フェーズ「証明終了」：フィナーレの放射バースト
// ============================================================
static constexpr int    kFinaleRingBulletCount = 72;
static constexpr int    kFinaleAimedCount      = 5;
static constexpr double kFinaleSpeed           = 4.2;

static void ShotFinaleBurst(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 全方位リングバースト（赤・大玉）
        for (int i = 0; i < kFinaleRingBulletCount; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = 2.0 * DX_PI * (double)i / (double)kFinaleRingBulletCount;
            pShot->param_d[0] = angle;
            pShot->kind  = img_enemyShotLargeBall[0];
            pShot->muki  = angle;
            pShot->speed = kFinaleSpeed;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->margin = 480;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        // 自機狙い追加弾（橙・中玉）
        for (int i = 0; i < kFinaleAimedCount; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double offset = 0.5 * ((double)i / (double)(kFinaleAimedCount - 1) - 0.5);
            double angle = pSet->muki + offset;
            pShot->param_d[0] = angle;
            pShot->kind  = img_enemyShotMediumBall[8];
            pShot->muki  = angle;
            pShot->speed = kFinaleSpeed * 1.3;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->margin = 480;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    double t = (double)pSet->count;
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double angle = pShot->param_d[0];
        double spd = pShot->speed;
        pShot->x = pSet->x + spd * cos(angle) * t;
        pShot->y = pSet->y + spd * sin(angle) * t;
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_TAS_Claude()
{
    if (count == kPhase1Start) {
        enemy.maxHp = enemy.hp = 200;

        // 第1フェーズ：4層同心リングを一括召喚（以後ずっと自走し続ける）
        for (int layer = 0; layer < 4; layer++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotRingWall;
            pSet->x = kScreenCx;
            pSet->y = kScreenCy;
            pSet->param_i[0] = layer;
            pSet->param_i[1] = count;
            AttachEnemyShotSet(pSet);
        }
    }

    // ボスは緩やかに揺れるだけ（速度積分ではなくcountからの直接計算）
    enemy.x = kScreenCx + 80.0 * sin((double)count / 240.0);
    enemy.y = 90.0 + 20.0 * sin((double)count / 97.0);

    // 第3フェーズ：横5本＋縦5本のコンベア壁を一括召喚
    if (count == kPhase3Start) {
        for (int i = 0; i < kGridLineCount; i++) {
            sEnemyShotSet* pRow = new sEnemyShotSet;
            pRow->count = 0;
            pRow->patternFunc = ShotGridLine;
            pRow->param_i[0] = 0; // 横壁
            pRow->param_i[1] = i;
            pRow->param_i[2] = count;
            pRow->param_d[0] = 40.0 + i * (kScreenH - 80.0) / (double)(kGridLineCount - 1);
            AttachEnemyShotSet(pRow);

            sEnemyShotSet* pCol = new sEnemyShotSet;
            pCol->count = 0;
            pCol->patternFunc = ShotGridLine;
            pCol->param_i[0] = 1; // 縦壁
            pCol->param_i[1] = i;
            pCol->param_i[2] = count;
            pCol->param_d[0] = 40.0 + i * (kScreenW - 80.0) / (double)(kGridLineCount - 1);
            AttachEnemyShotSet(pCol);
        }
    }

    // 第2フェーズ：40フレームごとに自機狙い15way扇を発射
    if (count >= kPhase2Start && count < kPhase2Start + kPhase2Len) {
        if ((count - kPhase2Start) % 40 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotAimedFan;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x); // 発射時点のみ自機座標を参照
            AttachEnemyShotSet(pSet);
        }
    }

    // 第4フェーズ：フィナーレバースト
    if (count == kPhase4Start + kPhase4Len - 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFinaleBurst;
        pSet->x = kScreenCx;
        pSet->y = kScreenCy;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        AttachEnemyShotSet(pSet);
    }
}