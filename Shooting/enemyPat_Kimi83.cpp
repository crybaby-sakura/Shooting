// enemyPat_tmp.cpp
// 虹をモチーフにした弾幕「七色円弧陣 — プリズムアーチ」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 素材選択メモ
// ------------------------------------------------------------
// 【効果音】
//   - sound_enemyCharge     : 虹弾発射前の予告音
//   - sound_enemyShot_extreme: 白色閃光弾展開時の重低音
//
// 【弾の種類・色】
//   - 虹のアーチ弾 : 小玉 img_enemyShotSmallBall[色]
//     色: 0赤, 8橙, 1黄, 2緑, 3シアン, 4青, 5紫
//   - 白色閃光弾   : 大玉 img_enemyShotLargeBall[6]（白）
//   - 二次弾(自機狙い): 銃弾 img_enemyShotBullet[6]（白）
// ------------------------------------------------------------

static void ShotPrismArch(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 虹色テーブル（7色）
    const int rainbowColors[7] = { 0, 8, 1, 2, 3, 4, 5 };
    // 角速度テーブル（赤は左回り急、紫は右回り急、緑は直進）
    const double omegaTable[7] = { -0.015, -0.010, -0.005, 0.0, 0.005, 0.010, 0.015 };

    // ===== 初回生成 =====
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double baseAngle = DX_PI / 2.0; // 下向きを中心に

        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;

            double spread = (i - 3) * (DX_PI / 9.0) / 4; // 左右に扇状に広げる
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + spread;
            pEnemyShot->speed = 2.2;
            pEnemyShot->kind = img_enemyShotLargeBall[rainbowColors[i]];

            pEnemyShot->param_d[0] = omegaTable[i] * 2;    // 角速度
            pEnemyShot->param_i[0] = 0;                // フェーズ: 0=円弧飛行
            pEnemyShot->param_i[1] = rainbowColors[i]; // 色情報（見た目調整用）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ===== 白色閃光弾（セットcount 100で画面中央から展開） =====
    if (pEnemyShotSet->count == 100) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double base_angle = GetRand(100) / 100.0 * 2.0 * DX_PI;
        for (int i = 0; i < 16; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = base_angle + i * (DX_PI / 8.0);
            pEnemyShot->x = 240.0; // 画面中央
            pEnemyShot->y = 240.0;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 4.5;
            pEnemyShot->kind = img_enemyShotLargeBall[6]; // 白大玉
            pEnemyShot->param_i[0] = 2; // 白色閃光弾（直進）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ===== 弾更新 =====
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int phase = pShot->param_i[0];

        if (phase == 0) {
            // --- フェーズ0: 虹弾の円弧飛行 ---
            pShot->muki += pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 減速して滞留（count 60〜100）
            if (pShot->count >= 60) {
                pShot->speed *= 0.92;
                if (pShot->speed < 0.3) pShot->speed = 0.0;
            }

            // 滞留後、下へ落下（count 130）
            if (pShot->count >= 130) {
                pShot->param_i[0] = 3; // 落下フェーズ
                pShot->speed = 2.5;
                pShot->muki = DX_PI / 2.0; // 下向き
            }
        }
        else if (phase == 2 || phase == 4) {
            // --- フェーズ2: 白色閃光弾（直進）---
            // --- フェーズ4: 二次弾（自機狙い） ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (phase == 3) {
            // --- フェーズ3: 虹弾落下 ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            if (pShot->count % 20 == 0) {
               // 自機狙い細弾を1発生成
                sEnemyShot* pSub = new sEnemyShot;
                double aim = atan2(player.y - pShot->y, player.x - pShot->x);
                pSub->x = pShot->x;
                pSub->y = pShot->y;
                pSub->muki = aim;
                pSub->speed = 3.5;
                pSub->kind = img_enemyShotBullet[pShot->param_i[1]]; // 白銃弾
                pSub->param_i[0] = 4; // 二次弾（直進）

                // pShot の前に挿入（現在の走査に影響しない）
                pSub->prev = pShot->prev;
                pSub->next = pShot;
                pShot->prev->next = pSub;
                pShot->prev = pSub;
            }
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Rainbow_Kimi()
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
        // 左右にゆっくり往復
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 約4秒間隔（240フレーム）で弾幕セットを生成
    if (count % 190 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPrismArch;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
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