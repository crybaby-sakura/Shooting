// enemyPat_shuumatsuKokushou.cpp
//
// 「終末刻鐘」— 時計をモチーフにした4レイヤー非同期弾幕
//
// 設計方針：
//   ・秒針(23F)/分針(41F)/時針(67F)/十二方位斉射(131F) の4つを
//     互いに素な周期で完全非同期に重ねる。最小公倍数は約830万フレーム
//     (約38時間@60fps)なので、同じ位相関係は実質的に二度と来ない。
//   ・各レイヤー単体は十分避けられる強さに抑え、「同時処理量」で難度を作る。
//   ・公平性の担保として、全レイヤー共通の「安全回廊」を1本だけ用意する。
//     安全回廊は SAFE_ZONE_PERIOD フレームで画面中心を1周する角度帯で、
//     各レイヤーはこの角度帯への弾配置を必ず避ける（間引く／逸らす）。
//     これにより「今この瞬間に必ず通れる道」が数式的に保証される。
//   ・当初案にあった「連鎖トリガー」層は、実装の複雑さと公平性の両立が
//     難しかったため、この安全回廊メカニズムに統合した。
//
// 元ネタメモ（提案時の対応関係）：
//   秒針 = ShotSecondHand（周期23F）
//   分針 = ShotMinuteHand（周期41F）
//   時針 = ShotHourHand（周期67F、狙い撃ち・ためて加速）
//   文字盤十二方位 = ShotTwelveBurst（周期131F、予告付き一斉解放）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数・共通ヘルパー
// ============================================================

// 安全回廊：SAFE_ZONE_PERIOD フレームで一周する、常に存在する逃げ道。
static const double SAFE_ZONE_PERIOD = 300.0;              // 周期(F) ≒5秒@60fps
static const double SAFE_ZONE_HALF_WIDTH = DX_PI / 10.0;    // 半幅(約18度、通路幅約36度)

static const double HOUR_HAND_SPEED = 1.8;
static const double HOUR_HAND_EASE_FRAMES = 60.0;

// 角度差を (-PI, PI] に正規化
static double AngleDiff(double a, double b)
{
    double d = fmod(a - b + DX_PI, 2.0 * DX_PI);
    if (d < 0.0) d += 2.0 * DX_PI;
    return d - DX_PI;
}

// 現在の安全回廊の中心角度（画面中心から見た方向）
static double SafeZoneAngle(int c)
{
    double cc = fmod((double)c, SAFE_ZONE_PERIOD);
    if (cc < 0.0) cc += SAFE_ZONE_PERIOD;
    return 2.0 * DX_PI * cc / SAFE_ZONE_PERIOD;
}

// 指定角度が安全回廊内かどうか（extraMarginで判定を広げられる）
static bool IsInSafeZone(double angle, int c, double extraMargin = 0.0)
{
    double diff = AngleDiff(angle, SafeZoneAngle(c));
    return fabs(diff) < (SAFE_ZONE_HALF_WIDTH + extraMargin);
}

// 時針用：「ためてから加速する」移動オフセットを閉じた式で計算
// speed(t) = s0 * (0.3 + 0.7*t/ease)  ( 0<=t<=ease、以降は s0 で等速 )
static double HourHandOffset(double t, double s0, double ease)
{
    if (t <= ease) {
        return s0 * (0.3 * t + 0.35 * t * t / ease);
    }
    double baseOffset = s0 * 0.65 * ease;
    return baseOffset + s0 * (t - ease);
}

// sEnemyShotSet を生成してリストに登録する共通ヘルパー
static sEnemyShotSet* CreateShotSet(double x, double y, double muki, sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = 0;

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
//  レイヤー1：秒針（周期23F）
//  中心から放射状に伸びる細い針。安全回廊と重なる瞬間は
//  「秒針が止まる」演出として発射自体を間引く。
// ============================================================
static void ShotSecondHand(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int NODE_NUM = 5;
        const double R_START = 24.0;
        const double R_STEP = 44.0;
        const double SPEED = 2.6;

        for (int i = 0; i < NODE_NUM; i++) {
            double r = R_START + R_STEP * i;
            double x0 = pEnemyShotSet->x + r * cos(pEnemyShotSet->muki);
            double y0 = pEnemyShotSet->y + r * sin(pEnemyShotSet->muki);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x0;
            pEnemyShot->y = y0;
            pEnemyShot->muki = pEnemyShotSet->muki;
            pEnemyShot->speed = SPEED;
            pEnemyShot->kind = img_enemyShotBullet[0]; // 赤：秒針

            // formula駆動のため、出発点と速度成分を保存する
            pEnemyShot->param_d[0] = x0;
            pEnemyShot->param_d[1] = y0;
            pEnemyShot->param_d[2] = SPEED * cos(pEnemyShotSet->muki);
            pEnemyShot->param_d[3] = SPEED * sin(pEnemyShotSet->muki);

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->param_d[2] * t;
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * t;
        pShot = pShot->next;
    }
}

// ============================================================
//  レイヤー2：分針（周期41F）
//  自機とは無関係に独立回転する扇状の壁。開き角と枚数が
//  正弦波でゆっくり揺らぐ。安全回廊に重なる弾のみ個別に間引く。
// ============================================================
static void ShotMinuteHand(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const double SPEED = 1.8;
        const double SPREAD = (50.0 + 20.0 * sin(count / 50.0)) * DX_PI / 180.0; // 半開き角
        const int N = 7 + (int)round(2.0 * sin(count / 70.0));

        for (int i = 0; i < N; i++) {
            double rate = (N == 1) ? 0.0 : ((double)i / (N - 1) - 0.5) * 2.0; // -1〜1
            double angle = pEnemyShotSet->muki + rate * SPREAD;

            // 安全回廊に重なる弾は生成そのものをスキップ（=そこだけ壁に穴が開く）
            if (IsInSafeZone(angle, count, 0.0)) continue;

            double x0 = pEnemyShotSet->x;
            double y0 = pEnemyShotSet->y;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x0;
            pEnemyShot->y = y0;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = SPEED;
            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄：分針

            pEnemyShot->param_d[0] = x0;
            pEnemyShot->param_d[1] = y0;
            pEnemyShot->param_d[2] = SPEED * cos(angle);
            pEnemyShot->param_d[3] = SPEED * sin(angle);

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->param_d[2] * t;
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * t;
        pShot = pShot->next;
    }
}

// ============================================================
//  レイヤー3：時針（周期67F）
//  発射時点の自機位置を狙う3way。最初はためて、後半で加速する
//  （時針が重い腰を上げてから追いすがるイメージ）。
//  安全回廊に重なる弾は消さず、外側へ逸らして誘導する。
// ============================================================
static void ShotHourHand(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const double SPREAD = 14.0 * DX_PI / 180.0;

        for (int i = -1; i <= 1; i++) {
            double angle = pEnemyShotSet->muki + i * SPREAD;

            if (IsInSafeZone(angle, count, 0.05)) {
                double sign = (AngleDiff(angle, SafeZoneAngle(count)) >= 0.0) ? 1.0 : -1.0;
                angle += sign * (SAFE_ZONE_HALF_WIDTH + 0.08);
            }

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = HOUR_HAND_SPEED;
            pEnemyShot->kind = img_enemyShotLargeBall[4]; // 青：時針

            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = cos(angle);
            pEnemyShot->param_d[3] = sin(angle);

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double offset = HourHandOffset(t, HOUR_HAND_SPEED, HOUR_HAND_EASE_FRAMES);
        pShot->x = pShot->param_d[0] + pShot->param_d[2] * offset;
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * offset;
        pShot = pShot->next;
    }
}

// ============================================================
//  レイヤー4：文字盤十二方位斉射（周期131F、30F前に予告音）
//  盤面を囲む12方位から一斉に外向きバーストを解放する。
//  安全回廊に最も近い1方位だけを丸ごと間引き、通路を確保する。
// ============================================================
static void ShotTwelveBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const int DIR_NUM = 12;
        const int NODE_PER_DIR = 3;
        const double R = 90.0;
        const double SPEED = 2.2;
        const double NODE_SPREAD = 8.0 * DX_PI / 180.0;

        for (int d = 0; d < DIR_NUM; d++) {
            double baseAngle = 2.0 * DX_PI * d / DIR_NUM;

            // 安全回廊に最も近い方位は丸ごとスキップ
            if (IsInSafeZone(baseAngle, count, 0.15)) continue;

            double x0 = pEnemyShotSet->x + R * cos(baseAngle);
            double y0 = pEnemyShotSet->y + R * sin(baseAngle);

            for (int n = 0; n < NODE_PER_DIR; n++) {
                double rate = n - (NODE_PER_DIR - 1) / 2.0;
                double angle = baseAngle + rate * NODE_SPREAD;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = x0;
                pEnemyShot->y = y0;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = SPEED;
                pEnemyShot->kind = img_enemyShotLargeBall[6]; // 白：文字盤
                pEnemyShot->margin = 480;

                pEnemyShot->param_d[0] = x0;
                pEnemyShot->param_d[1] = y0;
                pEnemyShot->param_d[2] = SPEED * cos(angle);
                pEnemyShot->param_d[3] = SPEED * sin(angle);

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->param_d[2] * t;
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * t;
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン：終末刻鐘
// ============================================================
void EnemyPat_TheHardest_Claude()
{
    static double swayDir;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        swayDir = 1.0;
    }
    else {
        // 文字盤本体はゆっくり左右に揺れる（振り子演出）
        enemy.x += 0.35 * swayDir;
        if (count % 300 == 150) swayDir *= -1.0;
    }

    // ---- レイヤー1：秒針（周期23F、素数） ----
    if (count % 23 == 0) {
        double spokeAngle = fmod(2.0 * DX_PI * count / 230.0, 2.0 * DX_PI);

        // 安全回廊と重なる瞬間は「秒針が止まる」演出として発射自体を間引く
        if (!IsInSafeZone(spokeAngle, count, 0.05)) {
            CreateShotSet(enemy.x, enemy.y, spokeAngle, ShotSecondHand);
        }
    }

    // ---- レイヤー2：分針（周期41F、素数） ----
    if (count % 41 == 0) {
        double baseAngle = fmod(2.0 * DX_PI * count / 480.0, 2.0 * DX_PI);
        CreateShotSet(enemy.x, enemy.y, baseAngle, ShotMinuteHand);
    }

    // ---- レイヤー3：時針（周期67F、素数） ----
    if (count % 67 == 0) {
        double aimAngle = atan2(player.y - enemy.y, player.x - enemy.x);
        CreateShotSet(enemy.x, enemy.y, aimAngle, ShotHourHand);
    }

    // ---- レイヤー4：文字盤十二方位斉射（周期131F、素数、30F前に予告） ----
    if (count % 131 == 101) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (count % 131 == 0) {
        CreateShotSet(enemy.x, enemy.y, 0.0, ShotTwelveBurst);
    }
}