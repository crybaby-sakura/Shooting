// enemyPat_Tmp.cpp
// 虹をモチーフにした弾幕「虹のアーチ弾幕」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 虹色の並び（利用可能な色インデックスにマッピング）
// 0:赤, 8:橙, 1:黄, 2:緑, 4:青, 3:シアン(藍相当), 5:マゼンタ(紫相当)
static const int rainbow_color[7] = { 0, 8, 1, 2, 4, 3, 5 };

// ------------------------------------------------------------
// 弾幕パターン：虹のアーチ
// ・count==0 でアーチ形成＋二次反射弾を生成
// ・各弾は param_i[0] の遅延後に動き出す（赤から順）
// ・param_d[0] に最終速度を保持
// ・画面外消去・countインクリメントはメインルーチン側で行う
// ------------------------------------------------------------
static void ShotRainbowArch(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // アーチの中心・半径（敵位置を基準に少し下げる）
        const double cx = pEnemyShotSet->x;
        const double cy = pEnemyShotSet->y + 40.0;
        const double radius = 155.0;

        // 各色帯あたりの弾数（密度調整用）
        const int bullets_per_band = 7;
        const int total = 7 * bullets_per_band;

        // ----- メイン：虹色アーチ形成 -----
        for (int c = 0; c < 7; c++) {
            for (int i = 0; i < bullets_per_band; i++) {
                pEnemyShot = new sEnemyShot;

                // 左(赤) → 右(紫) に均等配置
                double t = (double)(c * bullets_per_band + i) / (total - 1);
                // 上に凸のアーチになる角度範囲（約 25°〜155°）
                double theta = DX_PI * 0.14 + t * (DX_PI * 0.72);

                // y が小さい方が画面上なので -sin で上側に配置
                pEnemyShot->x = cx + radius * cos(theta);
                pEnemyShot->y = cy - radius * sin(theta);

                // 初期は停止。向きはプレイヤー方向＋微小なばらつき
                double base_muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
                pEnemyShot->muki = base_muki + (GetRand(40) - 20) / 180.0 * DX_PI;
                pEnemyShot->speed = 0.0;

                // 中玉で色をはっきり見せる
                pEnemyShot->kind = img_enemyShotMediumBall[rainbow_color[c]];

                // 遅延：赤から順に動き出す（20F + 色ごとに8F）
                pEnemyShot->param_i[0] = 20 + c * 8;
                // 最終速度（色でわずかに変化）
                pEnemyShot->param_d[0] = 1.7 + c * 0.06;
                pEnemyShot->margin = 240;

                // リンクリスト接続
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // ----- 二次反射弾：アーチ頂点付近から数本 -----
        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = cx + (GetRand(80) - 40);
            pEnemyShot->y = cy - radius + 5.0;

            double base_muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->muki = base_muki + (GetRand(50) - 25) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.0;

            // 小玉で軽く
            int col = rainbow_color[GetRand(6)];
            pEnemyShot->kind = img_enemyShotSmallBall[col];

            // アーチ本体より遅れて発射
            pEnemyShot->param_i[0] = 45 + GetRand(25);
            pEnemyShot->param_d[0] = 2.4 + GetRand(12) / 10.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ----- 毎フレーム更新 -----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 遅延時間を過ぎたら速度を与えて動き出す
        if (pShot->count >= pShot->param_i[0] && pShot->speed <= 0.0) {
            pShot->speed = pShot->param_d[0];
        }

        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// 関数名は指定通り EnemyPat_Rainbow_Grok
// ------------------------------------------------------------
void EnemyPat_Rainbow_Grok()
{
    static int muki;
    static int shot_phase;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 155.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_phase = 0;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.85 * (double)muki;
        if (count % 150 == 75) {
            muki *= -1;
        }
        // 画面端を超えないよう簡易クランプ
        if (enemy.x < 70.0) {
            enemy.x = 70.0;
            muki = 1;
        }
        if (enemy.x > 410.0) {
            enemy.x = 410.0;
            muki = -1;
        }
    }

    // 一定間隔で虹のアーチを発生
    // 90フレームごとに1回（難易度に応じて変更可）
    if (count % 70 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRainbowArch;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_phase++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // ショットセットをリストに接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}