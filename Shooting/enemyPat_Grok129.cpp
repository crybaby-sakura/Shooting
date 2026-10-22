#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 時間停止フラグ（このファイル内の弾パターンから参照）
static int isTimeStopped = 0;

// 時間停止中に撃つ弾（移動処理をスキップする）
static void ShotTimeStop(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 激しい発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 12発をランダム方向にばら撒き
        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(60) - 30);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(40) - 20);
            // 自機狙い + ランダム角度
            double base = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShot->muki = base + (GetRand(160) - 80) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.5 + GetRand(150) / 100.0;   // 1.5〜3.0

            // 小玉〜中玉・菱形・鱗弾をランダムに使用
            int col = GetRand(8);   // 0〜8の色
            switch (GetRand(3)) {
            case 0: pEnemyShot->kind = img_enemyShotSmallBall[col];   break;
            case 1: pEnemyShot->kind = img_enemyShotMediumBall[col];  break;
            case 2: pEnemyShot->kind = img_enemyShotDiamond[col];     break;
            default:pEnemyShot->kind = img_enemyShotScale[col];       break;
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 時間停止中は移動処理を一切行わない
    if (isTimeStopped) return;

    // 通常時のみ移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 通常時の弾（軽め）
static void ShotNormal(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            double base = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShot->muki = base + (i - 2) * 0.18;
            pEnemyShot->speed = 3.0 + GetRand(50) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumBall[GetRand(8)];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 時間停止中は移動しない（既存弾も停止）
    if (isTimeStopped) return;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_TimeStop_Grok()
{
    // フェーズ管理
    // 0 : 予告音再生 + 1秒待機
    // 1 : 時間停止（3秒）
    // 2 : 通常状態（3秒）
    static int phase = 0;
    static int phaseTimer = 0;
    static int muki = 1;
    static double lockedPlayerX = 0.0;
    static double lockedPlayerY = 0.0;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
        phaseTimer = 0;
        isTimeStopped = 0;

        // 最初の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ----- フェーズ遷移 -----
    phaseTimer++;
    switch (phase) {
    case 0: // 予告後1秒待機
        if (phaseTimer >= 60) {
            phase = 1;
            phaseTimer = 0;
            isTimeStopped = 1;
            // 時間停止開始瞬間の自機位置をロック
            lockedPlayerX = player.x;
            lockedPlayerY = player.y;
        }
        break;
    case 1: // 時間停止 3秒
        if (phaseTimer >= 180) {
            phase = 2;
            phaseTimer = 0;
            isTimeStopped = 0;
        }
        break;
    case 2: // 通常 3秒
        if (phaseTimer >= 180) {
            phase = 0;
            phaseTimer = 0;
            // 次周期の予告音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        break;
    }

    // ----- 時間停止中の特殊処理 -----
    if (isTimeStopped) {
        // 自機をロック位置に強制固定（動けなくする）
        player.x = lockedPlayerX;
        player.y = lockedPlayerY;

        // 自機ショットのY移動を打ち消す（メインの -15.0 を相殺）
        sPlayerShot* pPs = playerShotHead.next;
        while (pPs != &playerShotHead) {
            pPs->y += 15.0;
            pPs = pPs->next;
        }
    }

    // ----- ボス移動（左右往復） -----
    // 時間停止中は少し速く動く
    double moveSpd = isTimeStopped ? 1.8 : 0.98;
    enemy.x += moveSpd * (double)muki;
    if (enemy.x < 60.0) { enemy.x = 60.0;  muki = 1; }
    if (enemy.x > 420.0) { enemy.x = 420.0; muki = -1; }
    // 端で反転タイミングも変える
    if (count % (isTimeStopped ? 90 : 120) == 0) muki *= -1;

    // ----- 弾幕発射 -----
    if (isTimeStopped) {
        // 時間停止中は激しく撃ちまくる（4フレームに1回）
        if (phaseTimer % 5 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotTimeStop;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 12.0;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    else {
        // 通常時は控えめ（20フレームに1回）
        if (phaseTimer % 40 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotNormal;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}