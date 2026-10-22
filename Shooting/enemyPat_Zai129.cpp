#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ボス弾幕：時間停止
//
//  1周期 = 420フレーム
//    予告(60F) → 時間停止(180F) → 通常(180F) を繰り返す
//
//  時間停止中は
//    ・自機を直前の位置に強制移動させて動けなくする
//    ・自機ショットの y に +15.0 してメインルーチンの -15.0 を相殺
//    ・ボスが動き回りながら撃ちまくる
//    ・敵弾の移動処理は一切行わない（その場に静止し、解除後に一斉に飛ぶ）
//
//  ※ 自機の移動処理は敵パターンチンの前に処理される前提
// ============================================================

static const int CYCLE = 420; // 1周期のフレーム数
static const int WARN_FRAMES = 60;  // 予告時間（1秒）
static const int STOP_FRAMES = 180; // 時間停止時間（3秒）

// 現在フレームが時間停止中かどうか（count から純粋に決まるので
// 敵弾パターン関数側でも同じ判定を使えば呼び出し順序に依存しない）
static bool IsTimeStop()
{
    if (count < 1) return false;
    int c = (count - 1) % CYCLE;
    return (c >= WARN_FRAMES && c < WARN_FRAMES + STOP_FRAMES);
}

// 弾セット生成ヘルパー
static sEnemyShotSet* CreateEnemyShotSet(double x, double y, double muki, int kind,
    sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;
    pEnemyShotSet->patternFunc = func;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    return pEnemyShotSet;
}

// ------------------------------------------------------------
// 弾幕パターン：時間停止中に撃つ全方位弾
// 停止中は弾がその場に静止し、解除された瞬間に一斉に飛び出す
// ------------------------------------------------------------
static void ShotStopRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int n = pEnemyShotSet->param_i[0]; // 弾数

        for (int i = 0; i < n; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + 2.0 * DX_PI * i / n;
            pEnemyShot->speed = pEnemyShotSet->param_d[0] + GetRand(200) / 100.0; // 基準速度 + ランダム

            // 4発に1発は中玉、それ以外は小玉。色はリングごとにずらす
            if (i % 4 == 0) pEnemyShot->kind = img_enemyShotMediumBall[(i + pEnemyShotSet->kind) % 9];
            else            pEnemyShot->kind = img_enemyShotSmallBall[(i + pEnemyShotSet->kind) % 9];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 時間停止中は敵弾の移動処理を一切行わない
    if (IsTimeStop()) return;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 弾幕パターン：通常時の自機狙い3-way
// ------------------------------------------------------------
static void ShotAimedSpread(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * 0.22;
            pEnemyShot->speed = 3.2;
            pEnemyShot->kind = img_enemyShotSmallBall[(pEnemyShotSet->kind * 3 + 2) % 9];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 停止が発生した場合はこちらも凍結させる
    if (IsTimeStop()) return;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_TimeStop_Zai()
{
    static int muki;
    static int shotCount;
    static double prevX, prevY; // 直前の自機位置

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shotCount = 0;
        prevX = player.x;
        prevY = player.y;
    }

    int c = (count - 1) % CYCLE; // 周期内の経過フレーム
    bool timeStop = IsTimeStop();

    // ---- 効果音 ----
    if (c == 0) {
        // 予告音（1秒後に時間停止へ）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (c == WARN_FRAMES) {
        // 時間停止開始
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    if (c == WARN_FRAMES + STOP_FRAMES) {
        // 時間停止解除
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // ---- 自機の拘束と自機ショットの相殺 ----
    if (timeStop) {
        // 自機を直前の位置に強制移動して動けないようにする
        player.x = prevX;
        player.y = prevY;

        // メインルーチンでの y -= 15.0 を打ち消す
        sPlayerShot* pPlayerShot = playerShotHead.next;
        while (pPlayerShot != &playerShotHead) {
            pPlayerShot->y += 15.0;
            pPlayerShot = pPlayerShot->next;
        }
    }
    else {
        // 停止していない間は直前の位置を記録し続ける
        prevX = player.x;
        prevY = player.y;
    }

    // ---- ボスの移動 ----
    if (timeStop) {
        // 停止中はリサージュ曲線で画面上部を高速に動き回る
        int t = c - WARN_FRAMES;
        enemy.x = 240.0 + 170.0 * sin(t * 0.055);
        enemy.y = 110.0 + 55.0 * sin(t * 0.13 + 1.3);
    }
    else {
        // 通常時は左右への往復
        enemy.x += 0.98 * (double)muki;
        if (enemy.x < 40.0) { enemy.x = 40.0;  muki = 1; }
        if (enemy.x > 440.0) { enemy.x = 440.0; muki = -1; }
        enemy.y += (60.0 - enemy.y) * 0.02; // 少しずつ基準の高さへ戻る
    }

    // ---- 攻撃 ----
    if (timeStop) {
        // 時間停止中は10フレームごとに撃ちまくる（弾はその場に静止する）
        if (c % 10 == 0) {
            sEnemyShotSet* pEnemyShotSet = CreateEnemyShotSet(
                enemy.x, enemy.y,
                atan2(player.y - enemy.y, player.x - enemy.x)
                + (GetRand(120) - 60) / 180.0 * DX_PI, // 自機方向 ± ランダム回転
                shotCount++,
                ShotStopRing);

            pEnemyShotSet->param_i[0] = 16 + (shotCount % 3) * 4; // 弾数 16/20/24
            pEnemyShotSet->param_d[0] = 1.2;                       // 基準速度
        }
    }
    else if (c % 40 == 20) {
        // 通常時は自機狙い3-way
        CreateEnemyShotSet(
            enemy.x, enemy.y + 10.0,
            atan2(player.y - (enemy.y + 10.0), player.x - enemy.x),
            shotCount++,
            ShotAimedSpread);
    }
}