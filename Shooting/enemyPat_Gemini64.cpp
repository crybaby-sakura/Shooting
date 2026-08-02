// enemyPat_SanshokuDango.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：春風・三色串崩し（さんしょくくしくずし）
static void ShotSanshokuDango(sEnemyShotSet* pSet)
{
    // 発射時（串団子＋串の生成）
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 1. 串の生成（橙色の短レーザーを使用）
        // 短レーザー(64.0x4.0)を中心(白玉と同じ位置)に配置すると、両端がはみ出て串に見える
        sEnemyShot* pStick = new sEnemyShot;
        pStick->muki = pSet->muki;
        pStick->speed = 3.0;
        pStick->param_i[0] = 4; // 串として識別
        pStick->kind = img_enemyShotLaser[8]; // 橙色の短レーザー
        pStick->x = pSet->x;
        pStick->y = pSet->y;

        pStick->prev = pSet->pEnemyShotHead->prev;
        pStick->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pStick;
        pSet->pEnemyShotHead->prev = pStick;

        // 2. 3色の玉を生成（0:ピンク, 1:白, 2:緑）
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->muki = pSet->muki;
            pShot->speed = 3.0;
            pShot->param_i[0] = i;

            // 前から ピンク・白・緑 の順になるように20ピクセル間隔で配置
            // i=0(前: +20), i=1(中: 0), i=2(後: -20)
            double offset = (1 - i) * 20.0;
            pShot->x = pSet->x + offset * cos(pSet->muki);
            pShot->y = pSet->y + offset * sin(pSet->muki);

            // マゼンタをピンクの代用とする
            if (i == 0) pShot->kind = img_enemyShotLargeBall[5];
            else if (i == 1) pShot->kind = img_enemyShotLargeBall[6];
            else pShot->kind = img_enemyShotLargeBall[2];

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 分離時の効果音
    if (pSet->count == 60) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // --- ピンク（マゼンタ）：桜花散開 ---
        if (pShot->param_i[0] == 0) {
            if (pSet->count < 60) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (pSet->count == 60) {
                // 16方向に小玉をばら撒く
                int way = 16;
                for (int i = 0; i < way; i++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    pNew->muki = pShot->muki + (double)i * DX_PI * 2.0 / way;
                    pNew->speed = 2.0;
                    pNew->kind = img_enemyShotSmallBall[5];
                    pNew->param_i[0] = 3; // 破片として識別

                    pNew->prev = pSet->pEnemyShotHead->prev;
                    pNew->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pNew;
                    pSet->pEnemyShotHead->prev = pNew;
                }
                // 大玉本体は画面外に飛ばして消滅（自動消去に任せる）
                pShot->y = -9999.0;
            }
        }
        // --- 白：白雪一閃 ---
        else if (pShot->param_i[0] == 1) {
            if (pSet->count < 60) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (pSet->count == 60) {
                // 自機狙いを再計算し、一気に加速
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->speed = 8.0;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        // --- 緑：新緑誘導 ---
        else if (pShot->param_i[0] == 2) {
            if (pSet->count < 60) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (pSet->count == 60) {
                // 一旦減速
                pShot->speed = 1.0;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                // 自機に向かってゆっくり誘導
                double target_muki = atan2(player.y - pShot->y, player.x - pShot->x);
                double diff = target_muki - pShot->muki;

                // 角度の正規化 (-PI ～ PI)
                while (diff < -DX_PI) diff += 2 * DX_PI;
                while (diff > DX_PI) diff -= 2 * DX_PI;

                double rot_max = 0.015; // 旋回力
                if (diff > rot_max) pShot->muki += rot_max;
                else if (diff < -rot_max) pShot->muki -= rot_max;
                else pShot->muki = target_muki;

                // 少しずつ加速
                if (pShot->speed < 3.5) pShot->speed += 0.01;

                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        // --- 破片（ピンク小玉） ---
        else if (pShot->param_i[0] == 3) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        // --- 串（橙短レーザー） ---
        else if (pShot->param_i[0] == 4) {
            if (pSet->count < 60) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (pSet->count == 60) {
                // 串が抜ける演出（画面外へ飛ばしてメインルーチンの消去処理に任せる）
                pShot->y = -9999.0;
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_TricolorDango_Gemini()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右にゆらゆら移動
        enemy.x += 1.0 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 150フレーム周期で攻撃（予告音 -> 発射）
    int cycle = count % 150;
    if (cycle == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else if (cycle == 40) {
        for (int i = -1; i <= 1; i++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSanshokuDango;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            // 発射角度を自機狙いに設定
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x) + i * DX_PI / 3;

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