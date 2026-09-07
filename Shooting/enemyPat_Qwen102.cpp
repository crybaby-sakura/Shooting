// enemyPat_Tmp.cpp
// ポップアップ広告をモチーフにした弾幕パターン「スパムウィンドウ・バースト」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：スパムウィンドウ・バースト
static void ShotPopupWindow(sEnemyShotSet* pEnemyShotSet)
{
    // count == 0: 初期発射（予告と出現）
    if (pEnemyShotSet->count == 0) {
        // 予告音（不穏なポップアップ出現の予感）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 画面4隅の座標
        double corners[4][2] = { {0.0, 0.0}, {480.0, 0.0}, {0.0, 480.0}, {480.0, 480.0} };

        for (int i = 0; i < 4; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = corners[i][0];
            pShot->y = corners[i][1];

            // プレイヤー方向へ発射
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 2.0 * 3;

            // 白・小玉（初期の小さなポップアップ通知を表現）
            pShot->kind = img_enemyShotSmallBall[6];

            // param_i[0] を分裂管理フラグとして使用 (0:未分裂, 1:分裂済み)
            pShot->param_i[0] = 0;

            // リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }
    // count == 12: 拡大フェーズ 1
    else if (pEnemyShotSet->count == 12) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                // 白・中玉へ変化（ウィンドウが開いてくる演出）
                pShot->kind = img_enemyShotMediumBall[6];
                pShot->speed = 3.5 * 3; // 加速して接近
            }
            pShot = pShot->next;
        }
    }
    // count == 22: 拡大フェーズ 2 (急接近)
    else if (pEnemyShotSet->count == 22) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                // 白・大玉へ変化（画面を覆うような巨大な広告ウィンドウ）
                pShot->kind = img_enemyShotLargeBall[6];
                pShot->speed = 5.0 * 3; // さらに加速
            }
            pShot = pShot->next;
        }
    }
    // count == 30: 分裂フェーズ
    else if (pEnemyShotSet->count == 30) {
        // 分裂時の効果音（軽快だが鬱陶しい音）
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                pShot->param_i[0] = 1; // 分裂済みフラグ
                pShot->speed = 0.5;    // 急減速（その場に残る装飾化）

                // 分裂弾の生成 (8方向 + ランダムばらつき)
                for (int i = 0; i < 8*2; i++) {
                    sEnemyShot* pNewShot = new sEnemyShot;
                    pNewShot->x = pShot->x;
                    pNewShot->y = pShot->y;

                    // GetRand(20) は 0〜20 を返すため、/100.0 で 0.0〜0.2 のばらつきを与える
                    pNewShot->muki = (DX_PI * 2.0 / 8.0) * i + GetRand(20) / 100.0;
                    pNewShot->speed = 2.5 + GetRand(15) / 10.0; // 2.5 〜 4.0

                    // 黄・菱形弾（「閉じるボタン」を連打したような派手な破片を表現）
                    pNewShot->kind = img_enemyShotDiamond[1];
                    pNewShot->param_i[0] = 2;

                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }
            }
            pShot = pShot->next;
        }
    }
    // count == 31 〜 60: 回転・装飾フェーズ
    else if (pEnemyShotSet->count > 30 && pEnemyShotSet->count <= 60) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                // 分裂元の巨大弾は回転しながらさらに減速し、フェードアウト的な挙動を模倣
                pShot->muki += 0.15;
                pShot->speed *= 0.90;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
        return; // 移動処理を行ったため、ここで早期リターン
    }
    // count > 60: 役割終了。以降は何もしない（メインルーチンが画面外消去を担当）
    else if (pEnemyShotSet->count > 60) {
        //return;
    }

    // 通常の移動処理 (上記の特殊フレーム以外)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_PopUpAds_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0; // やや上めから出現
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右へのゆっくりした往復移動
        enemy.x += 0.8 * (double)muki;
        if (count % 180 == 90) {
            muki *= -1;
        }
    }

    // 150フレーム(約2.5秒)ごとにポップアップ弾幕を発射
    if (count % 150 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPopupWindow;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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