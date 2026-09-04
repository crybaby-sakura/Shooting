// enemyPat_brownian.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：熱揺らぎ（ブラウン運動）
static void ShotBrownian(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 親弾（ブラウン粒子）を 8〜12 個生成
        int numParent = 8 + GetRand(4); // 8,9,10,11,12

        for (int i = 0; i < numParent; i++) {
            pEnemyShot = new sEnemyShot;

            // 敵周辺に少しばらつきを持たせて配置
            pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(40) - 20;

            // 全方位にランダムな向き（0°〜360°）
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;

            // 遅めの速度（1.0〜2.0）
            pEnemyShot->speed = (100 + GetRand(100)) / 100.0;

            // 親弾：中玉(7.0x7.0)、橙(8)色
            pEnemyShot->kind = img_enemyShotMediumBall[8];

            // param_i[0] = 1 : 親弾フラグ
            pEnemyShot->param_i[0] = 1;
            // param_i[1] = 0 : 親弾種別
            pEnemyShot->param_i[1] = 0;
            // param_d[0] : 次の角度変化までの残りフレーム数
            pEnemyShot->param_d[0] = GetRand(5) + 3; // 3〜8フレーム
            // param_d[1] : 親弾の寿命（フレーム数）
            pEnemyShot->param_d[1] = 180 + GetRand(60); // 180〜240フレーム（3〜4秒@60fps）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 1) {
            // === 親弾（ブラウン粒子）の更新 ===

            // 角度変化タイマーが切れたら方向転換
            pShot->param_d[0] -= 1.0;
            if (pShot->param_d[0] <= 0) {
                // ±20°〜±40° の範囲でランダムに角度変化
                int sign = (GetRand(1) == 0) ? 1 : -1;
                double delta = (double)(GetRand(20) + 20) * sign / 180.0 * DX_PI;
                pShot->muki += delta;

                // 速度も ±20% の範囲でわずかに変動
                double speedVar = 1.0 + (GetRand(40) - 20) / 100.0;
                pShot->speed *= speedVar;
                if (pShot->speed < 0.5) pShot->speed = 0.5;
                if (pShot->speed > 3.0) pShot->speed = 3.0;

                // 次の変化までのタイマー：3〜8フレーム
                pShot->param_d[0] = GetRand(5) + 3;
            }

            // 移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 3フレームごと（0.05秒@60fps）に痕跡弾を生成
            if (pShot->count % 3 == 0) {
                sEnemyShot* pTrace = new sEnemyShot;
                pTrace->x = pShot->x;
                pTrace->y = pShot->y;
                pTrace->muki = 0.0;
                pTrace->speed = 0.0; // 静止させて当たり判定を実質無力化

                // 痕跡弾：小玉(2.5x2.5)、白(6)色
                pTrace->kind = img_enemyShotSmallBall[6];
                pTrace->param_i[0] = 0; // 親弾ではない
                pTrace->param_i[1] = 1; // 痕跡弾種別
                pTrace->param_d[1] = 24; // 寿命 24フレーム（0.4秒@60fps）

                pTrace->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pTrace->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pTrace;
                pEnemyShotSet->pEnemyShotHead->prev = pTrace;
            }

            // 寿命チェック
            pShot->param_d[1] -= 1.0;
            if (pShot->param_d[1] <= 0) {
                // 終端拡散：最後の進行方向を中心に小弾を 6〜8 個、扇状に発射
                int numBurst = 6 + GetRand(2); // 6,7,8
                for (int i = 0; i < numBurst; i++) {
                    sEnemyShot* pBurst = new sEnemyShot;
                    pBurst->x = pShot->x;
                    pBurst->y = pShot->y;

                    // 最後の進行方向を中心に ±60° の扇状
                    double spread = (GetRand(120) - 60) / 180.0 * DX_PI;
                    pBurst->muki = pShot->muki + spread;

                    // 中速（2.0〜4.0）
                    pBurst->speed = (200 + GetRand(200)) / 100.0;

                    // 終端弾：小玉(2.5x2.5)、橙(8)色
                    pBurst->kind = img_enemyShotSmallBall[8];
                    pBurst->param_i[0] = 0;
                    pBurst->param_i[1] = 2; // 終端拡散弾種別
                    pBurst->param_d[1] = 60 * 999; // 寿命 60フレーム（1秒@60fps）

                    pBurst->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pBurst->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pBurst;
                    pEnemyShotSet->pEnemyShotHead->prev = pBurst;
                }

                // 親弾を削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;

                pShot = pNext;
                continue;
            }
        }
        else {
            // === 痕跡弾・終端拡散弾の更新 ===

            if (pShot->param_i[1] == 2) {
                // 終端拡散弾は直線移動
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            // 痕跡弾（param_i[1] == 1）は speed=0 で静止

            // 寿命チェック
            pShot->param_d[1] -= 1.0;
            if (pShot->param_d[1] <= 0) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;

                pShot = pNext;
                continue;
            }
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_BrownianMotion_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 60フレーム（1秒）ごとに弾幕セットを生成
    if (count % 30 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBrownian;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        // ブラウン運動は全方位なのでプレイヤー方向は参照しないが一応設定
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