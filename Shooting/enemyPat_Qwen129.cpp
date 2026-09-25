#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：時間停止中（移動処理を行わず、その場に弾を生成し続ける）
// ============================================================
static void ShotTimeStop(sEnemyShotSet* pEnemyShotSet)
{
    // 時間停止中は移動処理を一切行わない

    // countに応じて弾を生成し続ける（撃ちまくる演出）
    if (pEnemyShotSet->param_i[0] == 1) {
        if (pEnemyShotSet->count % 4 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 12; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                // ボスの現在位置周辺にばら撒く
                pEnemyShot->x = pEnemyShotSet->x + GetRand(400) - 200;
                pEnemyShot->y = pEnemyShotSet->y + GetRand(300) - 150;

                // 時間停止解除後に動き出すよう、速度と向きを設定する
                pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x) + (GetRand(60) - 30) / 180.0 * DX_PI;
                pEnemyShot->speed = 2.0 + GetRand(200) / 100.0;

                // 時間停止っぽい色：シアン(3) または 青(4)
                int color = 3 + GetRand(1);
                pEnemyShot->kind = img_enemyShotDiamond[color];
                pEnemyShot->margin = 120;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    if (pEnemyShotSet->param_i[0] == 2) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
    }
}

// ============================================================
// 敵本体のパターン：時間停止能力
// ============================================================
void EnemyPat_TimeStop_Qwen()
{
    static int phase = 0;          // 0:予告, 1:時間停止, 2:通常
    static int state_count = 0;    // 各フェーズ内のフレームカウント
    static double saved_player_x = 0.0;
    static double saved_player_y = 0.0;
    static sEnemyShotSet* pShotSet;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        state_count = 0;

        // 弾幕セットは1つだけ作成し、使い回す（deleteはメインルーチンに任せる）
        pShotSet = new sEnemyShotSet;
        pShotSet->count = 0;
        pShotSet->patternFunc = ShotTimeStop;
        pShotSet->x = enemy.x;
        pShotSet->y = enemy.y + 10.0;
        pShotSet->muki = 0.0;
        pShotSet->kind = 0;

        pShotSet->pEnemyShotHead = new sEnemyShot;
        pShotSet->pEnemyShotHead->prev = pShotSet->pEnemyShotHead;
        pShotSet->pEnemyShotHead->next = pShotSet->pEnemyShotHead;

        pShotSet->prev = enemyShotSetHead.prev;
        pShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pShotSet;
        enemyShotSetHead.prev = pShotSet;
    }

    // ============================================================
    // フェーズ 0: 予告 (1秒 = 60フレーム)
    // ============================================================
    if (phase == 0) {
        if (state_count == 0) {
            // 予告音の再生
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // ボスは予告中、ゆっくりと左右に揺れる
        enemy.x = 240.0 + 80.0 * sin(state_count * 0.05);
        enemy.y = 60.0;

        // 予告中は弾を撃たない
        //pShotSet->patternFunc = ShotDummy;

        state_count++;
        if (state_count >= 60) {
            // 1秒経過したので時間停止フェーズへ移行
            phase = 1;
            state_count = 0;

            // 自機の現在位置を保存
            saved_player_x = player.x;
            saved_player_y = player.y;

            // 時間停止用としてカウントをリセットし、関数を設定
            pShotSet->count = 0;
            pShotSet->param_i[0] = 1;
        }
    }
    // ============================================================
    // フェーズ 1: 時間停止 (3秒 = 180フレーム)
    // ============================================================
    else if (phase == 1) {
        // 自機の位置を直前の位置に強制移動して動けないようにする
        player.x = saved_player_x;
        player.y = saved_player_y;

        // 自機ショットのy座標を毎フレーム+15.0し、メインルーチンでの-15.0を打ち消す
        sPlayerShot* pPShot = playerShotHead.next;
        while (pPShot != &playerShotHead) {
            pPShot->y += 15.0;
            pPShot = pPShot->next;
        }

        // ボスは動き回りながら敵弾を撃ちまくる
        enemy.x = 240.0 + 160.0 * sin(state_count * 0.08);
        enemy.y = 60.0 + 50.0 * cos(state_count * 0.12);

        // 弾幕セットの位置を更新（生成位置をボスに追従させる）
        pShotSet->x = enemy.x;
        pShotSet->y = enemy.y + 10.0;

        state_count++;
        if (state_count >= 180) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 3秒経過したので時間停止を解除し、通常フェーズへ移行
            phase = 2;
            state_count = 0;

            // 通常状態では弾を撃たない
            pShotSet->param_i[0] = 2;
        }
    }
    // ============================================================
    // フェーズ 2: 通常状態 (3秒 = 180フレーム)
    // ============================================================
    else if (phase == 2) {
        // ボスは通常通り動く
        enemy.x = 240.0 + 100.0 * sin(state_count * 0.06);
        enemy.y = 60.0;

        // 通常状態では弾を撃たない（patternFunc は nullptr のまま）

        state_count++;
        if (state_count >= 180) {
            // 3秒経過したので予告フェーズへ戻り、1周期完了
            phase = 0;
            state_count = 0;
            pShotSet->param_i[0] = 2;
        }
    }
}