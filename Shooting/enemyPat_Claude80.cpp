// enemyPat_KitakazeTaiyouSoutou.cpp
//
// 北風太陽争闘 ― 「北風と太陽」をモチーフにした4フェーズ無限ループパターン
//
// フェーズ構成:
//   1. 北風の予兆(暗雲形成) : 外周から弾が渦を巻きながら収束し、上空に暗雲のシルエットを形作る
//   2. 北風の猛攻           : 左右から横殴りの突風弾が波状に襲来。安全地帯(隙間)は時間とともに狭まる
//                             (力ずくで押すほど旅人が身を固くする様子を、隙間の縮小で表現)
//   3. 太陽の温もり         : 中心から同心リング状に暖色弾がゆったり拡散。安全地帯は逆に広がっていく
//                             (穏やかに迫るほど旅人が気を許す様子を、間引かれるリングで表現)
//   4. 決着                 : 暗雲が黒白点滅して予告した後、暗雲・新規弾すべてが中心から
//                             放射状に加速飛散して締めくくる(以後フェーズ1へループ)

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ==== 周期・フェーズ境界(フレーム、1始まりのローカルカウント基準) ====
constexpr int CYCLE_LEN = 640; // 1サイクルの長さ
constexpr int PHASE1_END = 80;  // 北風の予兆 終了
constexpr int PHASE2_END = 320; // 北風の猛攻 終了
constexpr int PHASE3_END = 560; // 太陽の温もり 終了
constexpr int TELEGRAPH_START = 561; // 決着・予告点滅 開始(ShotCloudForm内で処理)
constexpr int RELEASE_START = 601; // 決着・解放 開始

constexpr double CX = 240.0; // 画面中心X (画面は480x480)
constexpr double CY_ = 70.0;  // 暗雲・太陽の中心Y(上空寄り)

// 暗雲の輪郭(弾48発で形成)
constexpr int    CLOUD_N = 48;
constexpr double CLOUD_R0 = 70.0;
constexpr double CLOUD_A = 18.0;
constexpr double CLOUD_EASE_FRAMES = 70.0;
constexpr int    CLOUD_RELEASE_SHOTCOUNT = RELEASE_START - 1; // = 600 (生成からこの経過フレームで解放)

// 突風の縦列
constexpr int    GUST_ROWS = 16 / 2;
constexpr double GUST_SPACING = 32.0 * 2;

//------------------------------------------------------
// イージング(ease-out cubic)
//------------------------------------------------------
static double EaseOutCubic(double t)
{
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    double u = 1.0 - t;
    return 1.0 - u * u * u;
}

//------------------------------------------------------
// フェーズ1: 暗雲を形作る弾(渦を巻きながら収束→保持→決着で放射状に飛散)
// param_d[0],[1] : 開始座標(外周)
// param_d[2],[3] : 目標座標(暗雲の輪郭点)
// param_d[4],[5] : 暗雲の中心座標(飛散方向の算出に使用)
//------------------------------------------------------
static void ShotCloudForm(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double cx = pEnemyShotSet->x;
        double cy = pEnemyShotSet->y;

        for (int i = 0; i < CLOUD_N; i++) {
            double theta = DX_PI * 2.0 * i / CLOUD_N;
            double r = CLOUD_R0 + CLOUD_A * cos(8.0 * theta); // 4つの房を持つ雲の輪郭
            double endX = cx + r * cos(theta);
            double endY = cy + r * sin(theta) * 0.55; // 縦を圧縮して雲らしい扁平形状に

            double startR = 420.0;
            double startX = cx + startR * cos(theta + DX_PI * 0.5); // 角度をずらし渦状の収束に
            double startY = cy + startR * sin(theta + DX_PI * 0.5);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = startX;
            pEnemyShot->y = startY;
            pEnemyShot->kind = img_enemyShotMediumBall[7]; // 黒=暗雲
            pEnemyShot->param_d[0] = startX;
            pEnemyShot->param_d[1] = startY;
            pEnemyShot->param_d[2] = endX;
            pEnemyShot->param_d[3] = endY;
            pEnemyShot->param_d[4] = cx;
            pEnemyShot->param_d[5] = cy;
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double sx = pShot->param_d[0], sy = pShot->param_d[1];
        double ex = pShot->param_d[2], ey = pShot->param_d[3];
        double cx = pShot->param_d[4], cy = pShot->param_d[5];

        if (pShot->count < CLOUD_RELEASE_SHOTCOUNT) {
            // 収束→保持
            double t = EaseOutCubic(pShot->count / CLOUD_EASE_FRAMES);
            pShot->x = sx + (ex - sx) * t;
            pShot->y = sy + (ey - sy) * t;
            pShot->muki = atan2(ey - sy, ex - sx);

            // 決着直前は黒/白点滅で予告
            if (pShot->count >= CLOUD_RELEASE_SHOTCOUNT - 40) {
                pShot->kind = ((pShot->count / 5) % 2 == 0) ? img_enemyShotMediumBall[7] : img_enemyShotMediumBall[6];
            }
        }
        else {
            // 決着: 中心から放射状に加速飛散
            double rt = (double)(pShot->count - CLOUD_RELEASE_SHOTCOUNT);
            double angle = atan2(ey - cy, ex - cx);
            double dist = 2.2 * rt + 0.05 * rt * rt;
            pShot->x = ex + dist * cos(angle);
            pShot->y = ey + dist * sin(angle);
            pShot->muki = angle;
            pShot->kind = img_enemyShotMediumBall[7];
        }

        pShot = pShot->next;
    }
}

//------------------------------------------------------
// フェーズ2: 突風の一列(縦方向に弾を並べ、安全地帯だけ空ける)
// ShotSet param_d[0] : 速度(向き含む符号、+で右向き/-で左向き)
// ShotSet param_d[1] : 安全地帯の中心Y
// ShotSet param_d[2] : 安全地帯の半幅
//------------------------------------------------------
static void ShotWindGust(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double startX = pEnemyShotSet->x;
        double vx = pEnemyShotSet->param_d[0];
        double gapY = pEnemyShotSet->param_d[1];
        double gapHalf = pEnemyShotSet->param_d[2];

        for (int i = 0; i < GUST_ROWS; i++) {
            double y = GUST_SPACING * 0.5 + GUST_SPACING * i;
            if (fabs(y - gapY) < gapHalf) continue; // 安全地帯は空ける

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = startX;
            pEnemyShot->y = y;
            pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotBullet[4] : img_enemyShotBullet[3]; // 青/シアン=風
            pEnemyShot->param_d[0] = startX;
            pEnemyShot->param_d[1] = y;
            pEnemyShot->param_d[2] = vx;
            pEnemyShot->param_d[3] = (double)GetRand(200) / 100.0 * DX_PI; // 揺らぎの位相

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
        double vx = pShot->param_d[2];
        double wobblePhase = pShot->param_d[3];
        double t = (double)pShot->count;

        pShot->x = x0 + vx * t;
        pShot->y = y0 + 10.0 * sin(0.08 * t + wobblePhase); // 風の乱流を表す横揺れ
        pShot->muki = (vx >= 0.0) ? 0.0 : DX_PI;

        pShot = pShot->next;
    }
}

//------------------------------------------------------
// フェーズ3: 太陽の同心リング(中心から一定速度で拡散)
// ShotSet param_d[0] : 角度オフセット
// ShotSet param_i[0] : この輪の弾数(時間とともに減らし、安全地帯を広げる)
// ShotSet param_i[1] : 輪の番号(色の交互切り替えに使用)
//------------------------------------------------------
static void ShotSunRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double cx = pEnemyShotSet->x;
        double cy = pEnemyShotSet->y;
        int    n = pEnemyShotSet->param_i[0];
        double angleOffset = pEnemyShotSet->param_d[0];
        int colorIdx = (pEnemyShotSet->param_i[1] == 0) ? 1 : 8; // 黄/橙を交互に

        for (int i = 0; i < n; i++) {
            double theta = DX_PI * 2.0 * i / n + angleOffset;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + 10.0 * cos(theta);
            pEnemyShot->y = cy + 10.0 * sin(theta);
            pEnemyShot->kind = img_enemyShotMediumOval[colorIdx];
            pEnemyShot->param_d[0] = theta;
            pEnemyShot->param_d[1] = cx;
            pEnemyShot->param_d[2] = cy;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double theta = pShot->param_d[0];
        double cx = pShot->param_d[1];
        double cy = pShot->param_d[2];
        double t = (double)pShot->count;
        double r = 10.0 + 1.3 * t;

        pShot->x = cx + r * cos(theta);
        pShot->y = cy + r * sin(theta);
        pShot->muki = theta;

        pShot = pShot->next;
    }
}

//------------------------------------------------------
// 自機狙いNway(フェーズ2/3/4共通、速度・本数・色をparamで指定)
// param_d[0] : 基準角度(自機方向)
// param_d[1] : 速度
// param_i[0] : 本数
// param_i[1] : 弾色
// param_i[2] : 弾種(0:中玉 1:大玉)
//------------------------------------------------------
static void ShotAimedNway(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double ox = pEnemyShotSet->x;
        double oy = pEnemyShotSet->y;
        double baseAngle = pEnemyShotSet->param_d[0];
        double speed = pEnemyShotSet->param_d[1];
        int    n = pEnemyShotSet->param_i[0];
        int    colorIdx = pEnemyShotSet->param_i[1];
        int    kindFlag = pEnemyShotSet->param_i[2];
        double spread = 12.0 * DX_PI / 180.0;

        for (int i = 0; i < n; i++) {
            double angle = baseAngle + spread * (i - (n - 1) / 2.0);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = ox;
            pEnemyShot->y = oy;
            pEnemyShot->kind = (kindFlag == 0) ? img_enemyShotMediumBall[colorIdx] : img_enemyShotLargeBall[colorIdx];
            pEnemyShot->muki = angle;
            pEnemyShot->param_d[0] = ox;
            pEnemyShot->param_d[1] = oy;
            pEnemyShot->param_d[2] = angle;
            pEnemyShot->param_d[3] = speed;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double x0 = pShot->param_d[0], y0 = pShot->param_d[1];
        double angle = pShot->param_d[2], speed = pShot->param_d[3];
        double t = (double)pShot->count;

        pShot->x = x0 + speed * t * cos(angle);
        pShot->y = y0 + speed * t * sin(angle);

        pShot = pShot->next;
    }
}

//------------------------------------------------------
// フェーズ4: 決着の放射バースト(太陽の勝利)
//------------------------------------------------------
static void ShotFinaleBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    constexpr int N = 72 / 2;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double cx = pEnemyShotSet->x;
        double cy = pEnemyShotSet->y;

        for (int i = 0; i < N; i++) {
            double theta = DX_PI * 2.0 * i / N;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx;
            pEnemyShot->y = cy;
            pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotLargeBall[1] : img_enemyShotLargeBall[8]; // 黄/橙
            pEnemyShot->muki = theta;
            pEnemyShot->param_d[0] = theta;
            pEnemyShot->param_d[1] = cx;
            pEnemyShot->param_d[2] = cy;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double theta = pShot->param_d[0];
        double cx = pShot->param_d[1];
        double cy = pShot->param_d[2];
        double t = (double)pShot->count;
        double dist = 2.5 * t + 0.03 * t * t;

        pShot->x = cx + dist * cos(theta);
        pShot->y = cy + dist * sin(theta);

        pShot = pShot->next;
    }
}

//------------------------------------------------------
// 新しいsEnemyShotSetを生成しリストに連結するヘルパー
//------------------------------------------------------
static sEnemyShotSet* NewShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    return pEnemyShotSet;
}

// 敵本体のパターン
void EnemyPat_NorthWindAndSun_Claude()
{
    if (count == 1) {
        enemy.x = CX;
        enemy.y = CY_;
        //enemy.x2 = CX;
        //enemy.y2 = CY_;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }

    // 呼吸するようにゆったり上下移動
    enemy.y = CY_ + 6.0 * sin(count * 0.02);
    //enemy.x2 = enemy.x;
    //enemy.y2 = enemy.y;

    int localCount = ((count - 1) % CYCLE_LEN) + 1; // 1始まりのサイクル内ローカルフレーム

    //====================================================
    // フェーズ1: 北風の予兆(暗雲形成)
    //====================================================
    if (localCount == 1) {
        NewShotSet(ShotCloudForm, enemy.x, enemy.y);
    }

    //====================================================
    // フェーズ2: 北風の猛攻(左右からの突風、安全地帯が徐々に狭まる)
    //====================================================
    if (localCount > PHASE1_END && localCount <= PHASE2_END) {
        int phase2Elapsed = localCount - PHASE1_END;
        double progress = (double)phase2Elapsed / (double)(PHASE2_END - PHASE1_END); // 0→1

        if (phase2Elapsed % 8 == 1) {
            bool fromLeft = (GetRand(1) == 0);
            double speed = 2.4 + 1.6 * progress; // 時間とともに加速
            double gapCenter = 240.0 + 150.0 * cos(DX_PI * 2.0 * phase2Elapsed / 130.0);
            double gapHalf = 95.0 * (1.0 - progress) + 30.0 * progress; // 安全地帯が徐々に狭まる

            sEnemyShotSet* p = NewShotSet(ShotWindGust, fromLeft ? -20.0 : 500.0, 0.0);
            p->param_d[0] = fromLeft ? speed : -speed;
            p->param_d[1] = gapCenter;
            p->param_d[2] = gapHalf;
        }

        // 合間に北風の威圧・自機狙い3way
        if (phase2Elapsed % 55 == 30) {
            sEnemyShotSet* p = NewShotSet(ShotAimedNway, enemy.x, enemy.y);
            p->param_d[0] = atan2(player.y - enemy.y, player.x - enemy.x);
            p->param_d[1] = 3.2;
            p->param_i[0] = 3;
            p->param_i[1] = 4; // 青
            p->param_i[2] = 0; // 中玉
        }
    }

    //====================================================
    // フェーズ3: 太陽の温もり(同心リング、安全地帯が徐々に広がる)
    //====================================================
    if (localCount > PHASE2_END && localCount <= PHASE3_END) {
        int phase3Elapsed = localCount - PHASE2_END;
        double progress = (double)phase3Elapsed / (double)(PHASE3_END - PHASE2_END); // 0→1

        if (phase3Elapsed % 40 == 1) {
            int n = (int)(28.0 * (1.0 - progress) + 12.0 * progress); // 時間とともに間引き、安全地帯が広がる
            int ringIndex = (phase3Elapsed / 40) % 2;

            sEnemyShotSet* p = NewShotSet(ShotSunRing, CX, CY_);
            p->param_d[0] = (ringIndex == 0) ? 0.0 : DX_PI / n; // 輪ごとに角度を少しずらす
            p->param_i[0] = n;
            p->param_i[1] = ringIndex;
        }

        // 合間に太陽の穏やかな自機狙い3way
        if (phase3Elapsed % 60 == 30) {
            sEnemyShotSet* p = NewShotSet(ShotAimedNway, enemy.x, enemy.y);
            p->param_d[0] = atan2(player.y - enemy.y, player.x - enemy.x);
            p->param_d[1] = 1.6;
            p->param_i[0] = 3;
            p->param_i[1] = 1; // 黄
            p->param_i[2] = 0; // 中玉
        }
    }

    //====================================================
    // フェーズ4: 決着(暗雲の予告点滅・放散はShotCloudForm内で処理済み)
    //====================================================
    if (localCount == RELEASE_START) {
        NewShotSet(ShotFinaleBurst, CX, CY_);

        sEnemyShotSet* p = NewShotSet(ShotAimedNway, enemy.x, enemy.y);
        p->param_d[0] = atan2(player.y - enemy.y, player.x - enemy.x);
        p->param_d[1] = 3.6;
        p->param_i[0] = 5;
        p->param_i[1] = 8; // 橙
        p->param_i[2] = 1; // 大玉
    }
}