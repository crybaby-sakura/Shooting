// enemyPat_yoyo.cpp
// ヨーヨーをモチーフにした弾幕「ヨーヨー・リターン」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：ヨーヨー・リターン
// 弾が放射状に伸びて一定距離で止まり、その後戻ってくる。
// 戻り時にわずかな揺れを付け、原点付近で画面外に飛ばして消去。
// ============================================================
static void ShotYoYoReturn(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // count == 0 のフレームで弾を生成（count のインクリメントはメイン側）
    if (pEnemyShotSet->count == 0) {
        // 効果音：中程度の発射音を使用
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 同時に出す弾の数（奇数で対称に）
        const int NUM = 7;
        // 広がり角度（ラジアン）
        const double SPREAD = 0.22;

        for (int i = 0; i < NUM; i++) {
            pEnemyShot = new sEnemyShot;

            // 発射位置はショットセットの位置
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 基準方向（プレイヤー方向）＋左右に扇状に広げる
            double offset = (i - (NUM / 2)) * SPREAD;
            pEnemyShot->muki = pEnemyShotSet->muki + offset;

            // 伸びる速度
            pEnemyShot->speed = 2.8;
            pEnemyShot->margin = 240;

            // 弾種：中玉を使用（視認性とヨーヨーらしい丸み）
            // 色はショットセットの kind を基準に少しずつ変化させる
            int color = (pEnemyShotSet->kind + i) % 9;
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            // 自由パラメータの設定
            // param_d[0] : 原点 x
            // param_d[1] : 原点 y
            // param_d[2] : 元の向き（戻り時の基準）
            // param_i[0] : フェーズ（0=伸びる, 1=戻る）
            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = pEnemyShot->muki;
            pEnemyShot->param_i[0] = 0;

            // リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // ===== 伸びるフェーズ =====
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 原点からの距離を計算
            double dx = pShot->x - pShot->param_d[0];
            double dy = pShot->y - pShot->param_d[1];
            double distSq = dx * dx + dy * dy;

            // 最大到達距離（約 170px）で反転
            if (distSq > 370.0 * 370.0) {
                pShot->param_i[0] = 1;                     // 戻りフェーズへ
                pShot->muki = pShot->param_d[2] + DX_PI;   // 向きを反転
                pShot->speed = 3.6;                        // 戻りは少し速く
            }
        }
        else {
            // ===== 戻るフェーズ =====
            // ヨーヨーらしいわずかな揺れを付加
            double sway = 0.04 * sin(pShot->count * 0.18);
            pShot->x += pShot->speed * cos(pShot->muki + sway);
            pShot->y += pShot->speed * sin(pShot->muki + sway);

            // 原点付近に戻ったら画面外に飛ばしてメイン側の消去に任せる
            double dx = pShot->x - pShot->param_d[0];
            double dy = pShot->y - pShot->param_d[1];
            if (dx * dx + dy * dy < 12.0 * 12.0) {
                pShot->x = -200.0;
                pShot->y = -200.0;
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// 関数名は指定通り EnemyPat_Yoyo_Grok
// ============================================================
void EnemyPat_Yoyo_Grok()
{
    static int muki;          // 敵の移動方向
    static int shot_count;    // ショットセットの種類カウンタ（色変化用）

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 150.0;
        enemy.maxHp = enemy.hp = 200;   // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.85 * (double)muki;
        if (count % 150 == 75) {
            muki *= -1;
        }
        // 画面端で強制反転
        if (enemy.x < 60.0)  muki = 1;
        if (enemy.x > 420.0) muki = -1;
    }

    // 一定間隔でヨーヨー弾幕を生成
    if (count % 55 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotYoYoReturn;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        // プレイヤー方向を基準にする
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