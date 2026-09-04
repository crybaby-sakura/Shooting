// enemyPat_YamiNoGyoushi.cpp
// 「闇夜の凝視」— ジャンプスケアをモチーフにした弾幕パターン
// 通常パターンが持つ「予告してから撃つ」というフェアな作法をあえて崩し、
// 静寂 → 無予告の至近距離出現 → フラッシュ → 偽りの安堵 → 二段目の不意打ち
// というホラー演出のリズムを、既存の弾種の組み合わせと出現タイミングだけで表現する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  周期・タイミング定数
// ============================================================
static const int CYCLE_LEN = 600; // 1サイクルの長さ（フレーム）
static const int EYES_HOLD = 18;  // 「目」が出現してから静止し続けるフレーム数
static const int RELIEF_LEN = 220; // 安堵フェーズ（心音）の長さ

// サイクル内での発火フレーム。count==1 の時に一度だけ乱数で決定し、
// 以後は毎サイクル同じフレームで再現される（リプレイ安全性を保つため）。
static int g_eyesTrigger = 0;
static int g_secondScareTrigger = 0;

// ============================================================
//  共通ユーティリティ：弾を1発生成してリストに繋ぐ
// ============================================================
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->kind = kind;
    pEnemyShot->margin = 480;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

    return pEnemyShot;
}

// ============================================================
//  フェーズ1: 静寂（助走）
//  画面端からごくまばらに、遅い黒玉が漂う。予告音は一切鳴らさない。
//  「目」が出現するフレーム（g_eyesTrigger）で静寂を打ち切る。
// ============================================================
static void ShotAmbientSparse(sEnemyShotSet* pEnemyShotSet)
{
    int c = count % CYCLE_LEN;

    if (c >= 1 && c <= g_eyesTrigger && pEnemyShotSet->count % 55 == 1) {
        sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet, img_enemyShotSmallBall[7]); // 黒・小玉

        double edgeX, edgeY;
        switch (GetRand(2)) {
        case 0:  edgeX = GetRand(480); edgeY = -10.0;         break; // 上端
        case 1:  edgeX = -10.0;        edgeY = GetRand(480);  break; // 左端
        default: edgeX = 490.0;        edgeY = GetRand(480);  break; // 右端
        }

        double muki = atan2(240.0 - edgeY, 240.0 - edgeX) + (GetRand(60) - 30) / 180.0 * DX_PI;

        pEnemyShot->param_d[0] = edgeX; // 起点x
        pEnemyShot->param_d[1] = edgeY; // 起点y
        pEnemyShot->param_d[2] = muki;  // 向き
        pEnemyShot->param_d[3] = 0.35 + GetRand(20) / 100.0; // 速さ（かなり遅い）
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0] + pShot->param_d[3] * cos(pShot->param_d[2]) * pShot->count;
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * sin(pShot->param_d[2]) * pShot->count;
        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ2: 「目」の無予告出現
//  自機のごく至近距離（すぐ脇・すぐ上）に、黒い大玉2発（目）が
//  予告なしで実体化し、EYES_HOLD フレームだけ完全静止する。
//  静止後、自機から逃げるように左右斜め上へ一気に弾け飛ぶ。
// ============================================================
static void ShotEyesAppear(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 予告音（sound_enemyCharge）は意図的に鳴らさない。無予告であることが恐怖演出の要。
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double cx = pEnemyShotSet->x; // 出現時点で捕捉した自機x
        double cy = pEnemyShotSet->y; // 出現時点で捕捉した自機y

        for (int i = 0; i < 2; i++) {
            sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet, img_enemyShotLargeBall[7]); // 黒・大玉
            double side = (i == 0) ? -1.0 : 1.0;

            pEnemyShot->param_d[0] = cx + side * 17.0; // 目の位置x（自機のすぐ脇）
            pEnemyShot->param_d[1] = cy - 26.0;         // 目の位置y（自機のすぐ上）
            pEnemyShot->param_d[2] = side * (DX_PI * 0.72); // 弾け飛ぶ向き（斜め上外側）
            pEnemyShot->param_d[3] = 6.5; // 弾け飛ぶ速さ（急に、かなり速く）
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->count < EYES_HOLD) {
            // 静止フェーズ：一切動かず、ただそこに「在る」ことの違和感を出す
            pShot->x = pShot->param_d[0];
            pShot->y = pShot->param_d[1];
        }
        else {
            // 弾け飛ぶフェーズ：静止していた反動でそのまま加速なしの高速直線離脱
            double t = pShot->count - EYES_HOLD;
            pShot->x = pShot->param_d[0] + pShot->param_d[3] * cos(pShot->param_d[2]) * t;
            pShot->y = pShot->param_d[1] + pShot->param_d[3] * sin(pShot->param_d[2]) * t;
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ2': フラッシュリング
//  「目」が静止し終わる瞬間に同期して、自機を至近距離で囲む
//  白玉のリングが1フレームで丸ごと実体化し、外側へ一気に弾け飛ぶ。
//  「中心から徐々に成長する」通常の予告的出現を排除しているのが肝。
// ============================================================
static void ShotFlashRing(sEnemyShotSet* pEnemyShotSet)
{
    const int RING_N = 40;

    if (pEnemyShotSet->count == EYES_HOLD) {
        double cx = pEnemyShotSet->x; // 出現時点で捕捉した自機x
        double cy = pEnemyShotSet->y; // 出現時点で捕捉した自機y

        for (int i = 0; i < RING_N; i++) {
            // 白玉を主体に、7発に1発だけ赤の大玉を混ぜて視覚的なインパクトを強める
            bool accent = (i % 7 == 0);
            int  kind = accent ? img_enemyShotLargeBall[0] : img_enemyShotSmallBall[6];
            sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet, kind);

            double angle = DX_PI * 2.0 * i / RING_N;
            double r0 = 16.0; // 自機のすぐ傍という至近距離半径

            pEnemyShot->param_d[0] = cx + r0 * cos(angle); // リング上の起点x
            pEnemyShot->param_d[1] = cy + r0 * sin(angle); // リング上の起点y
            pEnemyShot->param_d[2] = angle;                 // そのまま放射方向へ
            pEnemyShot->param_d[3] = accent ? 5.0 : 7.5;    // 弾け飛ぶ速さ
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count; // このリングは出現後すぐに動き出す
        pShot->x = pShot->param_d[0] + pShot->param_d[3] * cos(pShot->param_d[2]) * t;
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * sin(pShot->param_d[2]) * t;
        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ3: 偽りの安堵（心音）
//  静寂に戻るが、一定周期で「ドッ、ドッ」と自機狙いの単発弾が
//  2連続で飛んでくる。何かがまだ近くにいる違和感だけを残す。
// ============================================================
static void ShotHeartbeat(sEnemyShotSet* pEnemyShotSet)
{
    int c = count % CYCLE_LEN;
    int reliefStart = g_eyesTrigger + 40; // 一段目のフラッシュが収まった直後から開始
    int local = c - reliefStart;

    if (local >= 0 && local <= RELIEF_LEN) {
        int beatPhase = local % 70;
        if (beatPhase == 0 || beatPhase == 10) {
            sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet, img_enemyShotSmallBall[0]); // 赤・小玉

            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            double muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = muki;
            pEnemyShot->param_d[3] = 1.4; // ゆったりとした速さ
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0] + pShot->param_d[3] * cos(pShot->param_d[2]) * pShot->count;
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * sin(pShot->param_d[2]) * pShot->count;
        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ4: 二段目の不意打ち
//  「もう終わった」と油断させた直後、視界の隅（自機と逆サイド寄り）に
//  黒い大玉が1発だけ無予告で出現し、短く静止した後、小規模なリングを
//  弾いてコーナーの外へ抜ける。規模を一段目よりだいぶ小さくすることで
//  フェイントの効果を高める。
// ============================================================
static void ShotSecondScare(sEnemyShotSet* pEnemyShotSet)
{
    const int HOLD2 = 10;
    const int RING2_N = 14 * 3;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 自機の視界の隅（逆サイド寄り）に出現させる
        double side = (player.x < 240.0) ? 1.0 : -1.0;
        double ex = 240.0 + side * 190.0;
        double eyRaw = player.y - 60.0;
        double ey = (eyRaw > 30.0) ? eyRaw : 30.0;

        pEnemyShotSet->param_d[0] = ex; // ring 生成時に参照するための保存
        pEnemyShotSet->param_d[1] = ey;

        sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet, img_enemyShotLargeBall[7]); // 黒・大玉
        pEnemyShot->param_i[0] = 1; // 1発目＝「目」役の目印
        pEnemyShot->param_d[0] = ex;
        pEnemyShot->param_d[1] = ey;
    }

    if (pEnemyShotSet->count == HOLD2) {
        double ex = pEnemyShotSet->param_d[0];
        double ey = pEnemyShotSet->param_d[1];

        for (int i = 0; i < RING2_N; i++) {
            sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet, img_enemyShotSmallBall[6]); // 白・小玉
            double angle = DX_PI * 2.0 * i / RING2_N;

            pEnemyShot->param_d[0] = ex;
            pEnemyShot->param_d[1] = ey;
            pEnemyShot->param_d[2] = angle;
            pEnemyShot->param_d[3] = 6.0;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            if (pShot->count < HOLD2) {
                // 静止フェーズ
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];
            }
            else {
                // 静止後、画面外のコーナーへ抜けて自然消滅させる
                double t = pShot->count - HOLD2;
                double dirX = (pShot->param_d[0] < 240.0) ? -1.0 : 1.0;
                pShot->x = pShot->param_d[0] + dirX * 4.0 * t;
                pShot->y = pShot->param_d[1] - 4.0 * t;
            }
        }
        else {
            pShot->x = pShot->param_d[0] + pShot->param_d[3] * cos(pShot->param_d[2]) * pShot->count;
            pShot->y = pShot->param_d[1] + pShot->param_d[3] * sin(pShot->param_d[2]) * pShot->count;
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体
// ============================================================
void EnemyPat_JumpScare_Claude()
{
    if (count == 1) {
        // 画面内に留まり続けるボススプライトを持たない「不可視の気配」として、
        // 画面上端の外へ位置を固定する
        enemy.x = 240.0;
        enemy.y = -60.0;
        enemy.maxHp = enemy.hp = 200;

        // サイクル内での発火フレームを一度だけ乱数決定
        // （以後は毎サイクル同じフレームで再現される＝リプレイ安全）
        g_eyesTrigger = 130 + GetRand(110);                    // 130～240 の間で無予告発火
        g_secondScareTrigger = g_eyesTrigger + 100 + GetRand(140);    // 一段目よりだいぶ後、安堵フェーズの最中

        // 常駐する環境弾（静寂フェーズ）
        sEnemyShotSet* pAmbient = new sEnemyShotSet;
        pAmbient->count = 0;
        pAmbient->patternFunc = ShotAmbientSparse;
        pAmbient->x = 240.0;
        pAmbient->y = 240.0;
        pAmbient->pEnemyShotHead = new sEnemyShot;
        pAmbient->pEnemyShotHead->prev = pAmbient->pEnemyShotHead;
        pAmbient->pEnemyShotHead->next = pAmbient->pEnemyShotHead;
        pAmbient->prev = enemyShotSetHead.prev;
        pAmbient->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pAmbient;
        enemyShotSetHead.prev = pAmbient;

        // 常駐する心音弾（安堵フェーズ）
        sEnemyShotSet* pHeartbeat = new sEnemyShotSet;
        pHeartbeat->count = 0;
        pHeartbeat->patternFunc = ShotHeartbeat;
        pHeartbeat->x = 240.0;
        pHeartbeat->y = 0.0;
        pHeartbeat->pEnemyShotHead = new sEnemyShot;
        pHeartbeat->pEnemyShotHead->prev = pHeartbeat->pEnemyShotHead;
        pHeartbeat->pEnemyShotHead->next = pHeartbeat->pEnemyShotHead;
        pHeartbeat->prev = enemyShotSetHead.prev;
        pHeartbeat->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pHeartbeat;
        enemyShotSetHead.prev = pHeartbeat;
    }

    int c = count % CYCLE_LEN;

    // 「目」の無予告出現（＋直後に同期するフラッシュリング）
    if (c == g_eyesTrigger) {
        sEnemyShotSet* pEyes = new sEnemyShotSet;
        pEyes->count = 0;
        pEyes->patternFunc = ShotEyesAppear;
        pEyes->x = player.x; // 出現時点の自機位置を捕捉して固定
        pEyes->y = player.y;
        pEyes->pEnemyShotHead = new sEnemyShot;
        pEyes->pEnemyShotHead->prev = pEyes->pEnemyShotHead;
        pEyes->pEnemyShotHead->next = pEyes->pEnemyShotHead;
        pEyes->prev = enemyShotSetHead.prev;
        pEyes->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEyes;
        enemyShotSetHead.prev = pEyes;

        sEnemyShotSet* pRing = new sEnemyShotSet;
        pRing->count = 0;
        pRing->patternFunc = ShotFlashRing;
        pRing->x = player.x; // 同じく出現時点の自機位置を共有
        pRing->y = player.y;
        pRing->pEnemyShotHead = new sEnemyShot;
        pRing->pEnemyShotHead->prev = pRing->pEnemyShotHead;
        pRing->pEnemyShotHead->next = pRing->pEnemyShotHead;
        pRing->prev = enemyShotSetHead.prev;
        pRing->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pRing;
        enemyShotSetHead.prev = pRing;
    }

    // 二段目の不意打ち
    if (c == g_secondScareTrigger) {
        sEnemyShotSet* pSecond = new sEnemyShotSet;
        pSecond->count = 0;
        pSecond->patternFunc = ShotSecondScare;
        pSecond->x = player.x;
        pSecond->y = player.y;
        pSecond->pEnemyShotHead = new sEnemyShot;
        pSecond->pEnemyShotHead->prev = pSecond->pEnemyShotHead;
        pSecond->pEnemyShotHead->next = pSecond->pEnemyShotHead;
        pSecond->prev = enemyShotSetHead.prev;
        pSecond->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSecond;
        enemyShotSetHead.prev = pSecond;
    }
}