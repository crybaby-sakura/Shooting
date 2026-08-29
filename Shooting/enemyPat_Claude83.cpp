// enemyPat_NanairoKakyou.cpp
// 「七色架橋」— 虹をモチーフにした4フェーズパターン
//
//   フェーズ1 驟雨残滴 : 斜めの雨弾が降り注ぎ、次第に弱まって虹の予兆を残す
//   フェーズ2 光の弧   : 中心(画面外下方)から7色の弧が頂点から左右へ伸びて形成され、
//                        完成後は各色の弧が周期的に外向きパルスを撃ち続ける
//   フェーズ3 双虹対話 : 一回り外側に色順を反転させた副虹が出現(アレクサンダーの暗帯を挟む)。
//                        主虹・副虹は互いに逆方向へゆっくり自転し、振幅が最大になる瞬間に
//                        自機狙い3wayを同期発射する
//   フェーズ4 分光放散 : 予告ののち、主虹・副虹の全弾がその場から放射状に加速飛散
//                        (赤は緩やか、紫は急加速)し、同時に本体から自機狙い7wayを発射して締める
//
// 弾のオブジェクト種別は sEnemyShot::param_i[2] で管理する:
//   0 = 虹の輪郭点(常駐、フェーズ4で分光飛散に切り替わる)
//   1 = 外向きパルス弾
//   2 = 自機狙い3way弾(双虹対話)
//   3 = フィナーレの自機狙い7way弾

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const double DEG2RAD = DX_PI / 180.0;

// 虹の中心(画面480x480の下方はるか外)と弧の角度範囲(頂点270°を中心に左右へ)
static const double ARC_CENTER_X = 240.0;
static const double ARC_CENTER_Y = 560.0;
static const double THETA_START_RAD = 200.0 * DEG2RAD;
static const double THETA_END_RAD = 340.0 * DEG2RAD;

// 輪郭点の密度と形成アニメーション
static const int    PRIMARY_POINTS = 35; // 主虹(奇数にして頂点を中央index に一致させる)
static const int    SECONDARY_POINTS = 17; // 副虹(主虹より疎)
static const double PRIMARY_STAGGER = 3.0;  // 頂点からの距離indexあたりの遅延フレーム数
static const double SECONDARY_STAGGER = 4.0;
static const double GROW_FRAMES = 18.0; // 出現してから伸びきるまでのフレーム数

// 自転(双虹が呼吸するように揺れる)
static const double ROT_AMP = 0.16;
static const double ROT_SPEED = 2.0 * DX_PI / 160.0; // 周期160フレーム

// フェーズ4分光飛散の加速度(色帯ごとに係数を掛ける)
static const double DISPERSE_ACCEL = 0.012;

// 色 index (0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙)
// 虹の7色帯 → 赤,橙,黄,緑,シアン(藍相当),青,マゼンタ(紫相当) の順に対応させる
static const int RAINBOW_COLOR[7] = { 0, 8, 1, 2, 3, 4, 5 };

// 主虹は外側が赤・内側が紫。副虹はさらに外側に、色順を反転(内側が赤)して配置する
// (実際の副虹の色順が主虹と逆になる現象=アレクサンダーの暗帯を再現)
static const double PRIMARY_RADIUS[7] = { 396.0, 380.0, 364.0, 348.0, 332.0, 316.0, 300.0 };
static const double SECONDARY_RADIUS[7] = { 440.0, 454.0, 468.0, 482.0, 496.0, 510.0, 524.0 };

// フェーズタイミング(グローバル count 基準)
static const int PHASE1_START = 1;
static const int PHASE1_LEN = 150;
static const int PHASE2_START = PHASE1_START + PHASE1_LEN; // 151
static const int PULSE_START = PHASE2_START + 70;         // 221 (形成が終わる目安)
static const int PHASE2_LEN = 300;
static const int PHASE3_START = PHASE2_START + PHASE2_LEN; // 451
static const int PHASE3_LEN = 300;
static const int PHASE4_START = PHASE3_START + PHASE3_LEN; // 751
static const int FINAL_BURST_DELAY = 20;

const int T = 1000;


// ============================================================
//  補助関数
// ============================================================

// 主虹は+方向、副虹は-方向にゆっくり自転する(フェーズ2形成中は0)
static double BandRotation(bool secondary)
{
    if (count % T < PULSE_START) return 0.0;
    double r = ROT_AMP * sin((count % T - PULSE_START) * ROT_SPEED);
    return secondary ? -r : r;
}

// 虹の輪郭点(常駐弾)を1個生成
static void SpawnOutlinePoint(sEnemyShotSet* pEnemyShotSet, double angle, double radius,
    double revealDelay, int band, bool secondary)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->param_d[0] = angle;       // 目標角度(固定)
    pShot->param_d[1] = radius;      // 目標半径(固定)
    pShot->param_d[2] = revealDelay; // 出現(伸び始める)までの遅延フレーム数
    pShot->param_i[0] = band;
    pShot->param_i[1] = secondary ? 1 : 0;
    pShot->param_i[2] = 0;  // objectType: 輪郭点
    pShot->param_i[3] = -1; // 分光飛散の開始時刻(未捕捉)

    pShot->x = ARC_CENTER_X;
    pShot->y = ARC_CENTER_Y;
    pShot->muki = angle;
    pShot->speed = 0.0;
    pShot->kind = secondary ? img_enemyShotSmallBall[RAINBOW_COLOR[band]]
        : img_enemyShotMediumBall[RAINBOW_COLOR[band]];
    pShot->margin = 480;

    pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
    pEnemyShotSet->pEnemyShotHead->prev = pShot;
}

// 外向きパルス弾を1個生成
static void SpawnPulseBullet(sEnemyShotSet* pEnemyShotSet, double x, double y, double angle,
    int band, bool secondary)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->param_d[0] = x;
    pShot->param_d[1] = y;
    pShot->param_i[0] = band;
    pShot->param_i[1] = secondary ? 1 : 0;
    pShot->param_i[2] = 1; // objectType: 外向きパルス

    pShot->x = x;
    pShot->y = y;
    pShot->muki = angle;
    pShot->speed = 2.3 + band * 0.1;
    pShot->kind = img_enemyShotBullet[RAINBOW_COLOR[band]];
    pShot->margin = 480;

    pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
    pEnemyShotSet->pEnemyShotHead->prev = pShot;
}

// 自機狙い3wayを生成(双虹対話フェーズ)
static void SpawnAimedTriple(sEnemyShotSet* pEnemyShotSet, double x, double y, int band, bool secondary)
{
    double baseAngle = atan2(player.y - y, player.x - x);
    double spread = 12.0 * DEG2RAD;

    for (int k = -1; k <= 1; k++) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->param_d[0] = x;
        pShot->param_d[1] = y;
        pShot->param_i[0] = band;
        pShot->param_i[1] = secondary ? 1 : 0;
        pShot->param_i[2] = 2; // objectType: 自機狙い3way

        pShot->x = x;
        pShot->y = y;
        pShot->muki = baseAngle + k * spread;
        pShot->speed = 2.6;
        pShot->kind = img_enemyShotDiamond[RAINBOW_COLOR[band]];
       
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }
}

// ============================================================
//  弾幕:フェーズ1 驟雨残滴(雨)
// ============================================================
static void ShotAmezuriShower(sEnemyShotSet* pEnemyShotSet)
{
    if (count % T <= PHASE1_START + PHASE1_LEN) {
        double densityT = (double)(count % T - PHASE1_START) / PHASE1_LEN; // 0→1
        int interval = 4 + (int)(10 * densityT); // 密度が時間とともに減衰(生成間隔が伸びる)

        if ((count % T - PHASE1_START) % interval == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 6 / 3; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->param_d[0] = GetRand(520) - 20;          // 出現x(画面外を含む)
                pShot->param_d[1] = -20.0 - GetRand(60);        // 出現y(画面上方)
                pShot->param_d[2] = 1.1 + GetRand(40) / 100.0;  // 落下速度
                pShot->param_d[3] = (GetRand(200) - 100) / 100.0; // 横揺れ位相
                pShot->param_i[1] = GetRand(1);                 // 色をランダムに2択

                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];
                pShot->muki = DX_PI / 2.0; // 見た目上、ほぼ真下向き
                pShot->kind = (pShot->param_i[1] == 0) ? img_enemyShotBullet[3] : img_enemyShotBullet[6];
                pShot->margin = 480;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }
    else if (count % T == PHASE1_START + PHASE1_LEN + 1) {
        // 雨が上がり、虹の到来を予告する
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count;
        pShot->x = pShot->param_d[0] + 14.0 * sin(t * 0.1 + pShot->param_d[3]);
        pShot->y = pShot->param_d[1] + pShot->param_d[2] * t;
        if (count % T == T - 1) pShot->margin = -9999;
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕:フェーズ2/3 光の弧・双虹対話(色帯ごとに1インスタンス)
// ============================================================
static void ShotNijoBand(sEnemyShotSet* pEnemyShotSet)
{
    int band = pEnemyShotSet->param_i[0];
    bool secondary = (pEnemyShotSet->param_i[1] == 1);
    double radius = pEnemyShotSet->param_d[0];
    double sweep = THETA_END_RAD - THETA_START_RAD;

    // --- 初回のみ:輪郭点を頂点対称の遅延つきで一斉生成 ---
    if (pEnemyShotSet->count == 0) {
        int n = secondary ? SECONDARY_POINTS : PRIMARY_POINTS;
        int mid = (n - 1) / 2;
        double stagger = secondary ? SECONDARY_STAGGER : PRIMARY_STAGGER;

        for (int i = 0; i < n; i++) {
            double angle = THETA_START_RAD + sweep * i / (n - 1);
            int dist = (i > mid) ? (i - mid) : (mid - i);
            double revealDelay = dist * stagger;
            SpawnOutlinePoint(pEnemyShotSet, angle, radius, revealDelay, band, secondary);
        }
    }

    // --- 常時継続する外向きパルス攻撃(フェーズ4開始で自然停止) ---
    if (count % T >= PULSE_START && count % T < PHASE4_START) {
        int firePeriod = (secondary ? 70 : 50) + band * 6;
        int fireOffset = (band * 7) % firePeriod;

        if ((count % T - PULSE_START) % firePeriod == fireOffset) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            int emitterCount = secondary ? 4 : 6;
            for (int k = 0; k < emitterCount; k++) {
                double baseAngle = THETA_START_RAD + sweep * k / (emitterCount - 1);
                double angle = baseAngle + BandRotation(secondary);
                double ex = ARC_CENTER_X + radius * cos(angle);
                double ey = ARC_CENTER_Y + radius * sin(angle);
                SpawnPulseBullet(pEnemyShotSet, ex, ey, angle, band, secondary);
            }
        }
    }

    // --- フェーズ3:自転振幅が最大になる瞬間に自機狙い3wayを同期発射 ---
    if (count % T >= PHASE3_START && count % T < PHASE4_START) {
        int rotPeriod = (int)(2.0 * DX_PI / ROT_SPEED); // = 160
        int localT = count - PULSE_START;
        int phase = ((localT % rotPeriod) + rotPeriod) % rotPeriod;

        if (phase == rotPeriod / 4) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            double apexAngle = (THETA_START_RAD + THETA_END_RAD) / 2.0 + BandRotation(secondary);
            double ex = ARC_CENTER_X + radius * cos(apexAngle);
            double ey = ARC_CENTER_Y + radius * sin(apexAngle);
            SpawnAimedTriple(pEnemyShotSet, ex, ey, band, secondary);
        }
    }

    // --- 各弾の位置更新(formula駆動、速度積分は行わない) ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int objectType = pShot->param_i[2];

        if (objectType == 0) {
            // 虹の輪郭点
            if (count % T < PHASE4_START) {
                double angle = pShot->param_d[0] + BandRotation(pShot->param_i[1] == 1);
                double revealElapsed = pShot->count - pShot->param_d[2];
                double revealT = 0.0;
                if (revealElapsed > 0.0) {
                    revealT = revealElapsed / GROW_FRAMES;
                    if (revealT > 1.0) revealT = 1.0;
                    revealT = 1.0 - pow(1.0 - revealT, 3.0); // ease-out cubic
                }
                double rad = pShot->param_d[1] * revealT;
                pShot->x = ARC_CENTER_X + rad * cos(angle);
                pShot->y = ARC_CENTER_Y + rad * sin(angle);
            }
            else {
                // フェーズ4:分光放散(初回だけ位置と方向を凍結捕捉)
                if (pShot->param_i[3] < 0) {
                    double angle = pShot->param_d[0] + BandRotation(pShot->param_i[1] == 1);
                    pShot->param_d[3] = ARC_CENTER_X + pShot->param_d[1] * cos(angle);
                    pShot->param_d[4] = ARC_CENTER_Y + pShot->param_d[1] * sin(angle);
                    pShot->param_d[5] = angle;
                    pShot->param_i[3] = pShot->count;
                }
                double elapsed = pShot->count - pShot->param_i[3];
                double accelFactor = 1.0 + pShot->param_i[0] * 0.15; // 赤=緩やか、紫=急加速
                double dispR = 0.5 * DISPERSE_ACCEL * accelFactor * elapsed * elapsed;
                pShot->x = pShot->param_d[3] + dispR * cos(pShot->param_d[5]);
                pShot->y = pShot->param_d[4] + dispR * sin(pShot->param_d[5]);
            }
        }
        else {
            // 外向きパルス/自機狙い3way:直線飛翔(発射時のx,y,muki,speedから毎フレーム再計算)
            pShot->x = pShot->param_d[0] + pShot->speed * cos(pShot->muki) * pShot->count;
            pShot->y = pShot->param_d[1] + pShot->speed * sin(pShot->muki) * pShot->count;
        }

        if (count % T == T - 1) pShot->margin = -9999;

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕:フェーズ4 フィナーレ(自機狙い7way、色帯ごとに1発)
// ============================================================
static void ShotFinalBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - enemy.y, player.x - enemy.x);
        for (int j = 0; j < 7; j++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->param_d[0] = enemy.x;
            pShot->param_d[1] = enemy.y;
            pShot->param_i[0] = j;
            pShot->param_i[2] = 3; // objectType: フィナーレ

            pShot->x = enemy.x;
            pShot->y = enemy.y;
            pShot->muki = baseAngle + (j - 3) * (10.0 * DEG2RAD);
            pShot->speed = 1.6;
            pShot->kind = img_enemyShotLargeBall[RAINBOW_COLOR[j]];
            
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count;
        double accel = 0.02; // 徐々に加速しながら突き抜けていく
        pShot->x = pShot->param_d[0] + pShot->speed * cos(pShot->muki) * t + 0.5 * accel * cos(pShot->muki) * t * t;
        pShot->y = pShot->param_d[1] + pShot->speed * sin(pShot->muki) * t + 0.5 * accel * sin(pShot->muki) * t * t;
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_Rainbow_Claude()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }

    if (count % T == 1) {
        // フェーズ1:驟雨残滴を開始
        sEnemyShotSet* pRainSet = new sEnemyShotSet;
        pRainSet->count = 0;
        pRainSet->patternFunc = ShotAmezuriShower;
        pRainSet->x = enemy.x;
        pRainSet->y = enemy.y;

        pRainSet->pEnemyShotHead = new sEnemyShot;
        pRainSet->pEnemyShotHead->prev = pRainSet->pEnemyShotHead;
        pRainSet->pEnemyShotHead->next = pRainSet->pEnemyShotHead;

        pRainSet->prev = enemyShotSetHead.prev;
        pRainSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pRainSet;
        enemyShotSetHead.prev = pRainSet;
    }

    // フェーズ2:光の弧(主虹、7色帯)を形成開始
    if (count % T == PHASE2_START) {
        for (int j = 0; j < 7; j++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotNijoBand;
            pSet->x = ARC_CENTER_X;
            pSet->y = ARC_CENTER_Y;
            pSet->param_i[0] = j;
            pSet->param_i[1] = 0; // 主虹
            pSet->param_d[0] = PRIMARY_RADIUS[j];

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }

    // フェーズ3:双虹対話(副虹、7色帯を反転配置で追加)
    if (count % T == PHASE3_START) {
        for (int j = 0; j < 7; j++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotNijoBand;
            pSet->x = ARC_CENTER_X;
            pSet->y = ARC_CENTER_Y;
            pSet->param_i[0] = j;
            pSet->param_i[1] = 1; // 副虹
            pSet->param_d[0] = SECONDARY_RADIUS[j];

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }

    // フェーズ4:分光放散の予告
    if (count % T == PHASE4_START) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // フェーズ4:フィナーレの自機狙い7wayを発射
    if (count % T == PHASE4_START + FINAL_BURST_DELAY) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFinalBurst;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}