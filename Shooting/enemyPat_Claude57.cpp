// enemyPat_ichiryuManbai.cpp
//
// 弾幕：一粒万倍（ルール30セル・オートマトン）
//
// 1次元セルオートマトン「ルール30」（新セル状態 = 左 XOR (中央 OR 右)）を
// 世代ごとに1行ずつ弾幕として展開するパターン。
// 中央1個の種セルから始まり、世代を重ねるごとに左右非対称の三角形へ拡大する。
// 左半分は不規則（カオス）、右半分は斜め縞状の規則的パターンとなる
// ルール30特有の性質を、弾の色分けでそのまま可視化する。
//
// ルール30は完全に決定論的なセルオートマトンなので、このパターンは
// GetRand() を一切使用しない。乱数消費ゼロでリプレイ安全性が最も高い
// パターンの一つとなっている。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ルール30 グリッド生成（プログラム起動後に一度だけ計算）
// ============================================================
static const int RULE30_GEN = 40;                   // 世代数（行数）
static const int RULE30_CENTER = RULE30_GEN;            // 中央列インデックス
static const int RULE30_WIDTH = RULE30_GEN * 2 + 1;    // 列数

static bool rule30Grid[RULE30_GEN][RULE30_WIDTH];
static bool rule30Computed = false;

static void ComputeRule30()
{
    for (int j = 0; j < RULE30_WIDTH; j++) rule30Grid[0][j] = false;
    rule30Grid[0][RULE30_CENTER] = true; // 種セル（一粒）

    for (int i = 1; i < RULE30_GEN; i++) {
        for (int j = 0; j < RULE30_WIDTH; j++) {
            bool left = (j > 0) ? rule30Grid[i - 1][j - 1] : false;
            bool center = rule30Grid[i - 1][j];
            bool right = (j < RULE30_WIDTH - 1) ? rule30Grid[i - 1][j + 1] : false;

            // ルール30本体：新セル = 左 XOR (中央 OR 右)
            rule30Grid[i][j] = left ^ (center || right);
        }
    }
    rule30Computed = true;
}

// ============================================================
//  パターン定数
// ============================================================
static const double CELL_SPACING = 10.0;  // セル間の横間隔
static const double FALL_SPEED = 1.6;   // 通常世代の落下速度
static const double VOLLEY_SPEED = 4.8;   // 最終世代・自機狙いの速度
static const int    ROW_INTERVAL = 10;    // 世代の発射間隔（フレーム）

static const int PHASE1_LEN = 50;                          // Phase1：種の一粒（テレグラフ）
static const int PHASE2_LEN = RULE30_GEN * ROW_INTERVAL;   // Phase2+3：生成の連鎖～最終世代の一斉射
static const int PHASE4_WAIT = 80;                          // Phase4：再種までの間
static const int TOTAL_CYCLE = PHASE1_LEN + PHASE2_LEN + PHASE4_WAIT; // 1周期の長さ（以後、種から再生）

// ============================================================
//  通常世代：まっすぐ落下する弾
//  位置は pShot->count のみから決まる純粋な数式（速度積分しない）
// ============================================================
static void ShotRule30Fall(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int row = pEnemyShotSet->param_i[0];

        // 使用効果音: sound_enemyShot_light（世代ごとの軽い発射音）
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int j = 0; j < RULE30_WIDTH; j++) {
            if (!rule30Grid[row][j]) continue;

            sEnemyShot* pEnemyShot = new sEnemyShot;

            int colOffset = j - RULE30_CENTER;

            // 弾の種類：小玉（セルの粒立ちを表現）
            // 色分け：中央列は白（種の系譜）、左半分は赤（カオス側）、右半分はシアン（秩序側）
            int colorIndex;
            if (colOffset == 0)     colorIndex = 6; // 白
            else if (colOffset < 0) colorIndex = 0; // 赤：不規則な左半分
            else                    colorIndex = 3; // シアン：縞模様の右半分
            pEnemyShot->kind = img_enemyShotSmallBall[colorIndex];

            pEnemyShot->param_d[0] = pEnemyShotSet->x + colOffset * CELL_SPACING; // 基準x（固定）
            pEnemyShot->param_d[1] = pEnemyShotSet->y;                            // 基準y

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 横方向は固定、縦方向は count のみから求まる等速下降
        pShot->x = pShot->param_d[0];
        pShot->y = pShot->param_d[1] + pShot->count * FALL_SPEED;

        pShot = pShot->next;
    }
}

// ============================================================
//  最終世代：自機狙いの一斉射
//  発射時点の自機座標へ向かう角度を param_d に保持し、count のみで直進する
// ============================================================
static void ShotRule30Volley(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int row = pEnemyShotSet->param_i[0];

        // 使用効果音: sound_enemyShot_heavy（最終世代の合図）
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int j = 0; j < RULE30_WIDTH; j++) {
            if (!rule30Grid[row][j]) continue;

            sEnemyShot* pEnemyShot = new sEnemyShot;

            int colOffset = j - RULE30_CENTER;
            double startX = pEnemyShotSet->x + colOffset * CELL_SPACING;
            double startY = pEnemyShotSet->y;

            // 弾の種類：中玉・橙（最終世代であることを示す警戒色）
            pEnemyShot->kind = img_enemyShotMediumBall[8];

            pEnemyShot->param_d[0] = startX;
            pEnemyShot->param_d[1] = startY;
            pEnemyShot->param_d[2] = atan2(player.y - startY, player.x - startX); // 発射時点の自機狙い角
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double dist = pShot->count * VOLLEY_SPEED;
        pShot->x = pShot->param_d[0] + dist * cos(pShot->param_d[2]);
        pShot->y = pShot->param_d[1] + dist * sin(pShot->param_d[2]);

        pShot = pShot->next;
    }
}

// ============================================================
//  弾セット生成の共通処理
// ============================================================
static void SpawnRule30Row(int row, sEnemyShotSet::PatternFunc patternFunc)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = -20.0; // 画面上端の少し外側
    pEnemyShotSet->param_i[0] = row;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ============================================================
//  敵本体のパターン：一粒万倍（ルール30）
// ============================================================
void EnemyPat_Rule30_Claude()
{
    // 周期内カウント（Phase4：再種のループに使用。count 自体はリセットしない）
    int cycleCount = (count - 1) % TOTAL_CYCLE + 1;

    if (count == 1) {
        if (!rule30Computed) ComputeRule30();

        // ゲーム画面は480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // Phase 1：種の一粒（テレグラフ、弾は撃たず予告音のみ）
    if (cycleCount == 1) {
        // 使用効果音: sound_enemyCharge（予告音）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // Phase 2〜3：生成の連鎖（世代を順に発射／左右非対称の色分けは発射時に決定）
    //            → 最終世代のみ自機狙いの一斉射に切り替え
    if (cycleCount > PHASE1_LEN && cycleCount <= PHASE1_LEN + PHASE2_LEN) {
        int phase2Count = cycleCount - PHASE1_LEN;
        if ((phase2Count - 1) % ROW_INTERVAL == 0) {
            int row = (phase2Count - 1) / ROW_INTERVAL;
            if (row < RULE30_GEN - 1) {
                SpawnRule30Row(row, ShotRule30Fall);
            }
            else {
                SpawnRule30Row(row, ShotRule30Volley); // 最終世代
            }
        }
    }

    // Phase 4：再種
    // cycleCount が TOTAL_CYCLE を超えると自動的に 1 へ戻り、Phase1 から繰り返される。
    // なお古い世代の弾が画面上に残っている間に次の周期が始まることがあるが、
    // これは複数世代が重なり合う演出として意図的なもの（画面外の弾は自動で消去される）。
}