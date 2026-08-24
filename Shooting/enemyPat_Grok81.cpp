// enemyPat_Tmp.cpp
// 処理落ちをモチーフにした弾幕「フレームドロップ・カスケード」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 処理落ち制御用（ファイルスコープ）
static int freeze_timer = 0;          // 残り停止フレーム
static int last_freeze_len = 0;       // 直前の停止時間（ジャンプ量に使用）
static bool was_freezing = false;     // 前フレームが停止中だったか
static double current_move_mult = 1.0;// このフレームの移動倍率（0=停止 / 1=通常 / N=キャッチアップ）
static int next_freeze_at = 90;       // 次に処理落ちを起こすフレーム

// 弾幕パターン本体
static void ShotFrameDrop(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発生フレームのみ弾を生成
    if (pEnemyShotSet->count == 0) {
        // 発射音（うるさくならないよう間引き）
        if (pEnemyShotSet->kind % 4 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        int num = 8 + GetRand(5);               // 8〜13発
        double base_muki = pEnemyShotSet->muki;
        double spread = 0.55 + GetRand(25) / 100.0;

        // 時間経過で基本速度が上がる（処理落ちが重くなっていくイメージ）
        double base_spd = 1.9 + (double)count / 900.0;
        if (base_spd > 3.8) base_spd = 3.8;

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(18) - 9;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(8) - 4;

            double offset = (i - (num - 1) / 2.0) * (spread / (num > 1 ? num - 1 : 1));
            pEnemyShot->muki = base_muki + offset + (GetRand(16) - 8) / 180.0 * DX_PI * 0.25;
            pEnemyShot->speed = base_spd + GetRand(40) / 100.0;

            // 中玉を使用（視認性と密度のバランスが良い）。色はセットごとに変化
            int color = (pEnemyShotSet->kind + i) % 9;
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            // リスト接続
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動処理（処理落ち倍率を反映）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * current_move_mult * cos(pShot->muki);
        pShot->y += pShot->speed * current_move_mult * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体
void EnemyPat_Lag_Grok()
{
    static int muki = 1;
    static int shot_count = 0;
    static int freeze_count = 0;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
        freeze_count = 0;

        freeze_timer = 0;
        last_freeze_len = 0;
        was_freezing = false;
        current_move_mult = 1.0;
        next_freeze_at = 80 + GetRand(30);
    }
    else {
        // 左右移動
        enemy.x += 0.85 * (double)muki;
        if (count % 140 == 70) muki *= -1;
    }

    //--------------------------------------------------
    // 処理落ち状態の更新
    //--------------------------------------------------
    was_freezing = (freeze_timer > 0);

    if (freeze_timer > 0) {
        freeze_timer--;
    }

    // 新しい処理落ちを開始するか判定
    if (freeze_timer == 0 && count >= next_freeze_at) {
        // 進行度に応じて停止時間を短く、発生間隔も短くする
        int progress = count / 380;
        if (progress > 7) progress = 7;

        last_freeze_len = 16 - progress + GetRand(6);   // 序盤長め → 終盤短め
        if (last_freeze_len < 6) last_freeze_len = 6;

        freeze_timer = last_freeze_len;

        // 次の発生予定時刻（間隔が徐々に短くなる）
        int interval = 95 - progress * 9 + GetRand(25);
        if (interval < 28) interval = 28;
        next_freeze_at = count + interval;

        // 予告音（処理落ち開始の合図）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // このフレームの移動倍率を決定
    if (freeze_timer > 0) {
        // 停止中
        current_move_mult = 0.0;
    }
    else if (was_freezing) {
        // 停止が解けた瞬間 → 溜まっていた移動量を一気に適用（ワープ）
        current_move_mult = (double)last_freeze_len;
        freeze_count = count;
    }
    else {
        // 通常
        current_move_mult = 1.0;
    }

    //--------------------------------------------------
    // 弾幕セット生成
    //--------------------------------------------------
    int fire_interval = 11;
    if (count > 500)  fire_interval = 10;
    if (count > 1000) fire_interval = 9;
    if (count > 1600) fire_interval = 8;

    if (count % fire_interval == 1 && count - freeze_count > 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFrameDrop;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}