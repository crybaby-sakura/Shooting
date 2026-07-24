// enemyPat_kujakuKenran.cpp
//
// 孔雀絢爛（くじゃくけんらん）
// 孔雀が尾羽を扇状に開き、羽先の目玉模様が明滅したのち、
// 目玉から螺旋弾が波打つように放たれ、
// 最後は尾羽全体が揺れながら自機狙い弾→全目玉同時放射で締める4段階パターン。
//
// フェーズ構成:
//   1. 開扇     (1           〜 PHASE1_END) : 尾羽が中央から左右対称に伸びる
//   2. 目玉閃輝 (PHASE1_END+1〜 PHASE2_END) : 羽先に目玉模様が現れ明滅
//   3. 波状放流 (PHASE2_END+1〜 PHASE3_END) : 目玉から螺旋弾が中央→外側の順に放たれる
//   4. 求愛乱舞 (PHASE4_START 〜 PHASE4_END) : 尾羽全体が左右に揺れつつ自機狙い弾を放ち、
//                                             最後に全目玉が同時に放射弾を放って終わる

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ── 尾羽の形状パラメータ ────────────────────────────
static const int    FEATHER_HALF = 6;     // 中央を0として左右6本ずつ、計13本
static const double FAN_SPAN_DEG = 150.0; // 扇の開き角（度）
static const double BASE_LEN = 150.0;  // 最短の羽の長さ
static const double EXTRA_LEN = 7.0;   // 中央に近いほど長くなる増分
static const int    SEG_COUNT = 7;     // 1本の羽を構成するセグメント数
static const int    GROW_DURATION = 60;    // 羽が伸びきるまでのフレーム数
static const int    GROW_STAGGER = 4;     // 隣の羽との伸長開始ズレ（フレーム）

// ── フェーズ境界（フレーム数）────────────────────────
static const int PHASE1_END = 90;              // 開扇終了
static const int PHASE2_END = 150;             // 目玉閃輝終了
static const int PHASE3_END = 240;             // 波状放流終了
static const int PHASE4_START = PHASE3_END + 1;  // 求愛乱舞開始
static const int PHASE4_END = 314;             // 求愛乱舞終了

static const int BURST_STAGGER = 6;  // 波状放流：中央から外側への発生ズレ
static const int AIM_INTERVAL = 6; // 求愛乱舞中の自機狙い弾の間隔

// ── 目玉模様パラメータ ──────────────────────────────
static const double EYE_RADIUS = 9.0; // 目玉の輪の半径
static const int    EYE_RING_N = 8;   // 目玉の輪を構成する弾数

// ── 波状放流パラメータ ──────────────────────────────
static const int    BURST_COUNT = 10;   // 1つの目玉から放たれる螺旋弾の本数
static const double BURST_SPEED = 1.6;  // 螺旋が外側へ広がる速さ
static const double BURST_SPIN = 0.09; // 螺旋の回転速度（ラジアン/フレーム）

// ── 求愛乱舞パラメータ ──────────────────────────────
static const double SWAY_AMP = 0.26;               // 揺れ角の振幅（約15度）
static const double SWAY_FREQ = 2.0 * DX_PI / 80.0; // 揺れの周期（80フレームで1往復）
static const int    AIM_SPREAD_DEG = 15;
static const double AIM_SPEED = 2.6;

// ── フィナーレパラメータ ────────────────────────────
static const int    FINALE_RAY_COUNT = 16;
static const double FINALE_SPEED = 2.0;

// 赤,橙,黄,緑,シアン,青,マゼンタ の順で虹色に並べたテーブル
static const int COLOR_TABLE[7] = { 0, 8, 1, 2, 3, 4, 5 };

// 負数にも対応した絶対値（<math.h>のabsとの衝突回避のため自前定義）
static int AbsI(int v)
{
    return (v < 0) ? -v : v;
}

// 羽の角度（真下を基準に扇状に広げる。j=0が中央）
static double FeatherAngle(int j)
{
    double deltaAngle = (FAN_SPAN_DEG * DX_PI / 180.0) / (2 * FEATHER_HALF);
    return DX_PI / 2.0 + j * deltaAngle;
}

// 羽の最大長（中央ほど長い）
static double FeatherMaxLen(int j)
{
    return BASE_LEN + (FEATHER_HALF - AbsI(j)) * EXTRA_LEN;
}

// 羽の色（左端の赤から右端のマゼンタまでの虹色グラデーション）
static int FeatherColor(int j)
{
    int idx = (j + FEATHER_HALF) * 6 / (2 * FEATHER_HALF);
    return COLOR_TABLE[idx];
}

// 求愛ダンスの揺れオフセット（フェーズ4以降のみ有効）
static double SwayOffset(int tGlobal)
{
    if (tGlobal < PHASE4_START) return 0.0;
    return SWAY_AMP * sin(SWAY_FREQ * (tGlobal - PHASE4_START));
}

// ============================================================
// フェーズ1+：尾羽本体（伸びていくセグメント弾の集合）
// ============================================================
static void ShotFeatherGrow(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        for (int j = -FEATHER_HALF; j <= FEATHER_HALF; j++) {
            double maxLen = FeatherMaxLen(j);
            int    color = FeatherColor(j);
            int    delay = AbsI(j) * GROW_STAGGER;

            for (int k = 0; k < SEG_COUNT; k++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->kind = img_enemyShotMediumBall[color];

                pEnemyShot->param_i[0] = j;
                pEnemyShot->param_d[0] = maxLen;
                pEnemyShot->param_d[1] = (double)delay;
                pEnemyShot->param_d[2] = maxLen * (k + 1) / SEG_COUNT; // このセグメントの目標距離

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int    j = pShot->param_i[0];
        double maxLen = pShot->param_d[0];
        double delay = pShot->param_d[1];
        double target = pShot->param_d[2];

        double localT = pShot->count - delay;
        if (localT < 0.0) localT = 0.0;
        double growFrac = localT / GROW_DURATION;
        if (growFrac > 1.0) growFrac = 1.0;

        double dist = maxLen * growFrac;
        if (dist > target) dist = target;

        double angle = FeatherAngle(j) + SwayOffset(pShot->count);

        pShot->x = pEnemyShotSet->x + dist * cos(angle);
        pShot->y = pEnemyShotSet->y + dist * sin(angle);

        pShot = pShot->next;
    }
}

// ============================================================
// フェーズ2：目玉閃輝（羽先に現れ明滅する目玉模様）
// ============================================================
static void ShotEyeTelegraph(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int j = -FEATHER_HALF; j <= FEATHER_HALF; j++) {
            double maxLen = FeatherMaxLen(j);
            int    color = FeatherColor(j);

            // 瞳（中心の黒い大玉）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->kind = img_enemyShotLargeBall[7]; // 黒
            pEnemyShot->param_i[0] = j;
            pEnemyShot->param_i[1] = 0; // 0=瞳
            pEnemyShot->param_i[2] = color;
            pEnemyShot->param_d[0] = maxLen;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 輪（羽の色をした小玉がリング状に囲む）
            for (int r = 0; r < EYE_RING_N; r++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                pEnemyShot->param_i[0] = j;
                pEnemyShot->param_i[1] = 1; // 1=輪
                pEnemyShot->param_i[2] = color;
                pEnemyShot->param_d[0] = maxLen;
                pEnemyShot->param_d[1] = r * (2.0 * DX_PI / EYE_RING_N);
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int    j = pShot->param_i[0];
        int    type = pShot->param_i[1];
        int    color = pShot->param_i[2];
        double maxLen = pShot->param_d[0];

        int    tGlobal = PHASE1_END + 1 + pShot->count;
        double angle = FeatherAngle(j) + SwayOffset(tGlobal);
        double tipX = pEnemyShotSet->x + maxLen * cos(angle);
        double tipY = pEnemyShotSet->y + maxLen * sin(angle);

        if (type == 0) {
            // 瞳：黒と白を交互に明滅
            bool flashWhite = sin(pShot->count * 0.35) > 0.0;
            pShot->kind = flashWhite ? img_enemyShotLargeBall[6] : img_enemyShotLargeBall[7];
            pShot->x = tipX;
            pShot->y = tipY;
        }
        else {
            // 輪：羽の色と白を交互に明滅
            double ringAngle = pShot->param_d[1];
            bool flashWhite = sin(pShot->count * 0.35 + DX_PI) > 0.0;
            pShot->kind = flashWhite ? img_enemyShotSmallBall[6] : img_enemyShotSmallBall[color];
            pShot->x = tipX + EYE_RADIUS * cos(ringAngle);
            pShot->y = tipY + EYE_RADIUS * sin(ringAngle);
        }

        pShot = pShot->next;
    }
}

// ============================================================
// フェーズ3：波状放流（1つの目玉から放たれる螺旋弾）
// ============================================================
static void ShotEyeBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int color = pEnemyShotSet->kind; // 呼び出し側がkindに色を仕込んでいる
        for (int i = 0; i < BURST_COUNT; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->kind = img_enemyShotScale[color];
            pEnemyShot->param_d[0] = i * (2.0 * DX_PI / BURST_COUNT); // 初期角度

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t0 = pShot->param_d[0];
        double r = BURST_SPEED * pShot->count;
        double theta = t0 + BURST_SPIN * pShot->count;
        
        double px = pShot->x;
        double py = pShot->y;
        pShot->x = pEnemyShotSet->x + r * cos(theta);
        pShot->y = pEnemyShotSet->y + r * sin(theta);
        pShot->muki = atan2(pShot->y - py, pShot->x - px);

        pShot = pShot->next;
    }
}

// ============================================================
// フェーズ4：求愛乱舞（自機狙い3way弾）
// ============================================================
static void ShotAimed3way(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int    color = pEnemyShotSet->kind;
        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        for (int i = -1; i <= 1; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->kind = img_enemyShotBullet[color];
            pEnemyShot->muki = baseAngle + i * (AIM_SPREAD_DEG * DX_PI / 180.0);
            pEnemyShot->speed = AIM_SPEED;
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
        double x0 = pShot->param_d[0];
        double y0 = pShot->param_d[1];

        pShot->x = x0 + pShot->speed * pShot->count * cos(pShot->muki);
        pShot->y = y0 + pShot->speed * pShot->count * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// フィナーレ：全目玉同時放射（弾幕の壁）
// ============================================================
static void ShotRadialBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int color = pEnemyShotSet->kind;
        for (int i = 0; i < FINALE_RAY_COUNT; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->kind = img_enemyShotDiamond[color];
            pEnemyShot->muki = i * (2.0 * DX_PI / FINALE_RAY_COUNT);
            pEnemyShot->speed = FINALE_SPEED;
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
        double x0 = pShot->param_d[0];
        double y0 = pShot->param_d[1];

        pShot->x = x0 + pShot->speed * pShot->count * cos(pShot->muki);
        pShot->y = y0 + pShot->speed * pShot->count * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン：孔雀絢爛
// ============================================================
void EnemyPat_ThumbnailFriendly_Claude()
{
    static int aimCycle;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        aimCycle = 0;

        // フェーズ1：尾羽本体の生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFeatherGrow;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    if (count == PHASE1_END + 1) {
        // フェーズ2：目玉閃輝の生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotEyeTelegraph;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    int count_for_loop = (count - PHASE2_END) % 360 + PHASE2_END;
    if (count < PHASE2_END) count_for_loop = -1;

    // フェーズ3：波状放流（中央の羽から外側の羽へ順に発生）
    for (int j = -FEATHER_HALF; j <= FEATHER_HALF; j++) {
        if (count_for_loop == PHASE2_END + 1 + AbsI(j) * BURST_STAGGER) {
            double angle = FeatherAngle(j) + SwayOffset(count); // フェーズ3の時点ではまだ揺れていない
            double maxLen = FeatherMaxLen(j);
            int    color = FeatherColor(j);

            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotEyeBurst;
            pEnemyShotSet->x = enemy.x + maxLen * cos(angle);
            pEnemyShotSet->y = enemy.y + maxLen * sin(angle);
            pEnemyShotSet->kind = color;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }

    // フェーズ4：求愛乱舞（尾羽全体が揺れながら自機狙い弾を放つ）
    if (count_for_loop >= PHASE4_START && count_for_loop <= PHASE4_END &&
        (count_for_loop - PHASE4_START) % AIM_INTERVAL == 0) {

        int j = (aimCycle % (2 * FEATHER_HALF + 1)) - FEATHER_HALF;
        aimCycle++;

        double angle = FeatherAngle(j) + SwayOffset(count);
        double maxLen = FeatherMaxLen(j);
        int    color = FeatherColor(j);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAimed3way;
        pEnemyShotSet->x = enemy.x + maxLen * cos(angle);
        pEnemyShotSet->y = enemy.y + maxLen * sin(angle);
        pEnemyShotSet->kind = color;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // フィナーレ：全ての目玉から同時に放射状の弾幕を放つ
    if (count_for_loop == PHASE4_END + 1) {
        for (int j = -FEATHER_HALF; j <= FEATHER_HALF; j++) {
            double angle = FeatherAngle(j) + SwayOffset(count);
            double maxLen = FeatherMaxLen(j);
            int    color = FeatherColor(j);

            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotRadialBurst;
            pEnemyShotSet->x = enemy.x + maxLen * cos(angle);
            pEnemyShotSet->y = enemy.y + maxLen * sin(angle);
            pEnemyShotSet->kind = color;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}