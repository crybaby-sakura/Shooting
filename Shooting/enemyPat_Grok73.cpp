// enemyPat_Tmp.cpp
// ボス弾幕パターン「影渡り・裂空の輪」
// 瞬間移動を軸にした残像弾＋裂空弾の複合パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 残像弾パターン
// 消えた位置に円形配置 → 一定時間静止後にプレイヤー方向へ加速
// ------------------------------------------------------------
static void ShotResidual(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 16発の円形残像弾
        for (int i = 0; i < 16; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = i * (DX_PI * 2.0 / 16.0);
            // 消えた位置を中心に少し広げて配置
            pEnemyShot->x = pEnemyShotSet->x + 48.0 * cos(ang);
            pEnemyShot->y = pEnemyShotSet->y + 48.0 * sin(ang);
            pEnemyShot->muki = ang;          // 仮の向き（後で上書き）
            pEnemyShot->speed = 0.0;         // 最初は静止
            // 影らしい暗い色（黒 or マゼンタ）の中玉
            pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotMediumBall[7] : img_enemyShotMediumBall[5];
            pEnemyShot->param_i[0] = 0;      // 0:待機中  1:移動中

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 約0.5秒（30フレーム）静止後にプレイヤー方向へ加速開始
        if (pShot->count == 30 && pShot->param_i[0] == 0) {
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 3.2 + (GetRand(10) / 10.0);  // 少しバラつき
            pShot->param_i[0] = 1;
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 裂空弾パターン
// 出現位置から8方向へ高速弾を放射
// ------------------------------------------------------------
static void ShotCross(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 8方向（十字＋斜め）
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = i * (DX_PI * 2.0 / 8.0);
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 6.5;         // 高速

            // 鋭い印象の菱形弾 or 短レーザー、明るい色
            if (i % 2 == 0) {
                pEnemyShot->kind = img_enemyShotLaser[0];   // 赤
            }
            else {
                pEnemyShot->kind = img_enemyShotLaser[3];   // シアン
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の更新（等速直線運動のみ）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 準備フェーズ用の軽いばら撒き
// ------------------------------------------------------------
static void ShotPrepareScatter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = pEnemyShotSet->muki + (i - 3.5) * 0.18;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 2.0 + (GetRand(15) / 10.0);
            pEnemyShot->kind = img_enemyShotSmallBall[6];  // 白の小玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// フィニッシュ用：中央から収束残像弾
// ------------------------------------------------------------
static void ShotFinishConverge(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 24 * 5; i++) for (int j = 0; j < 3; j++) {
            pEnemyShot = new sEnemyShot;
            double ang = i * (DX_PI * 2.0 / 24.0);
            pEnemyShot->x = pEnemyShotSet->x + (50.0 + j * 50) * cos(ang);
            pEnemyShot->y = pEnemyShotSet->y + (50.0 + j * 50) * sin(ang);
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[5];  // マゼンタ
            pEnemyShot->param_i[0] = 0;
            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->count == 20 && pShot->param_i[0] == 0) {
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 4.0;
            pShot->param_i[0] = 1;
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Warp_Grok()
{
    // 状態管理用static変数
    static int state;              // 0:準備  1:瞬間移動ループ  2:フィニッシュ
    static int timer;              // 各状態内の経過フレーム
    static int teleport_count;     // 現在の瞬間移動回数
    static int wait_time;          // 次の瞬間移動までの待機時間
    static double old_x, old_y;    // 瞬間移動前の位置

    // ゲーム画面は 480x480
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        state = 0;
        timer = 0;
        teleport_count = 0;
        wait_time = 70;            // 最初の待機は長め
    }

    // ------------------------------------------------------------
    // 状態0：準備フェーズ（約0.8秒）
    // ------------------------------------------------------------
    if (state == 0) {
        timer++;

        // 予告音
        if (timer == 1) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 軽いばら撒きを数回
        if (timer == 10 || timer == 25 || timer == 40) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotPrepareScatter;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 8.0;
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

        // 準備終了 → 瞬間移動ループへ
        if (timer >= 48) {
            state = 1;
            timer = 0;
            teleport_count = 0;
        }
    }
    // ------------------------------------------------------------
    // 状態1：瞬間移動ループ（最大5回）
    // ------------------------------------------------------------
    else if (state == 1) {
        timer++;

        // 待機時間終了で瞬間移動実行
        if (timer >= wait_time) {
            // 現在位置を保存
            old_x = enemy.x;
            old_y = enemy.y;

            // --- 残像弾セット生成（消えた位置） ---
            {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotResidual;
                pSet->x = old_x;
                pSet->y = old_y;
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

            // --- 新しい位置を決定 ---
            // プレイヤーから一定距離を保ちつつ、画面内ランダム
            // GetRand(x) は 0〜x の x+1 種類
            double new_x, new_y;
            int try_count = 0;
            do {
                // 四隅寄りと中央寄りを交互に狙いやすくする
                if (teleport_count % 2 == 0) {
                    // 四隅寄り
                    new_x = 50.0 + GetRand(380);
                    new_y = 40.0 + GetRand(200);
                }
                else {
                    // 中央寄り
                    new_x = 120.0 + GetRand(240);
                    new_y = 60.0 + GetRand(160);
                }
                try_count++;
            } while (try_count < 8 &&
                (new_x - player.x) * (new_x - player.x) + (new_y - player.y) * (new_y - player.y) < 90.0 * 90.0);

            // 画面端クランプ
            if (new_x < 40.0) new_x = 40.0;
            if (new_x > 440.0) new_x = 440.0;
            if (new_y < 30.0) new_y = 30.0;
            if (new_y > 280.0) new_y = 280.0;

            enemy.x = new_x;
            enemy.y = new_y;

            // --- 裂空弾セット生成（出現位置） ---
            {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotCross;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
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

            // 次の待機時間を短縮（テンポアップ）
            teleport_count++;
            wait_time = 60 - teleport_count * 8;
            if (wait_time < 5) wait_time = 5;

            timer = 0;

            // 規定回数終了 → フィニッシュへ
            if (teleport_count >= 10) {
                state = 2;
                timer = 0;
            }
        }
    }
    // ------------------------------------------------------------
    // 状態2：フィニッシュ
    // ------------------------------------------------------------
    else if (state == 2) {
        timer++;

        // 中央付近へ移動
        if (timer == 1) {
            enemy.x = 240.0;
            enemy.y = 120.0;

            // 収束弾を生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotFinishConverge;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
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

        // 一定時間後にパターン終了（必要に応じて次のパターンへ繋ぐ）
        // ここでは特に何もしない（メイン側でHPや時間で管理することを想定）
        if (timer == 60) {
            state = 0;
            timer = 0;
        }
    }
}