#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：拡散噴水（スプレッド・ファウンテン）
static void ShotFountain(sEnemyShotSet* pEnemyShotSet)
{
    // 生成フェーズ: count が 60 未満の間だけ新規弾を生成し続ける
    if (pEnemyShotSet->count < 60) {

        // 1. センターコア（立ち上がる水柱）の生成
        // 毎フレーム1発、真上に高速で発射
        sEnemyShot* pCoreShot = new sEnemyShot;
        pCoreShot->x = pEnemyShotSet->x;
        pCoreShot->y = pEnemyShotSet->y;
        pCoreShot->muki = -DX_PI / 2.0;          // 真上 (-90度)
        pCoreShot->speed = 9.0;                  // 高速
        pCoreShot->param_d[0] = 0.0;             // accel_x (なし)
        pCoreShot->param_d[1] = 0.08;            // accel_y (わずかな重力で、高く上がりすぎないようにする)
        pCoreShot->kind = img_enemyShotSmallBall[3]; // 色: 3=シアン (水の芯)

        // 双方向リストに追加
        pCoreShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pCoreShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pCoreShot;
        pEnemyShotSet->pEnemyShotHead->prev = pCoreShot;

        // 2. サイド・アーチ（弧を描く飛沫）の生成
        // 1フレームあたり3発ばら撒き、密度を出す
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pArchShot = new sEnemyShot;
            pArchShot->x = pEnemyShotSet->x;
            pArchShot->y = pEnemyShotSet->y;

            // 真上(-90度)を中心に、左右60度の範囲(-150度 〜 -30度)でランダムにばら撒く
            // GetRand(120) は 0〜120 を返す。-30.0 - GetRand(120) で -30.0 〜 -150.0 になる。
            double angle = (-30.0 - GetRand(120)) / 180.0 * DX_PI;
            pArchShot->muki = angle;

            // 速度は中速でばらつきを持たせる (4.0 〜 6.0)
            pArchShot->speed = 4.0 + GetRand(20) / 10.0;

            pArchShot->param_d[0] = 0.0;         // accel_x (なし)
            pArchShot->param_d[1] = 0.15;        // accel_y (強めの重力で、早く放物線を描いて落ちさせる)
            pArchShot->kind = img_enemyShotMediumBall[6]; // 色: 6=白 (飛び散る水滴)

            // 双方向リストに追加
            pArchShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pArchShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pArchShot;
            pEnemyShotSet->pEnemyShotHead->prev = pArchShot;
        }
    }

    // 弾の移動処理 (加速度を考慮したベクトル計算)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 現在の速度ベクトルを取得
        double vx = pShot->speed * cos(pShot->muki);
        double vy = pShot->speed * sin(pShot->muki);

        // 加速度を加算 (param_d[0]: x軸加速度, param_d[1]: y軸加速度)
        vx += pShot->param_d[0];
        vy += pShot->param_d[1];

        // 新しい速度の大きさと向きを再計算
        pShot->speed = sqrt(vx * vx + vy * vy);
        pShot->muki = atan2(vy, vx);

        // 位置を更新
        pShot->x += vx;
        pShot->y += vy;

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Fountain_Qwen()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 260.0; // 少し下げて噴水の高さを確保
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        // 左右にゆっくり往復移動
        enemy.x += 0.8 * (double)muki;
        if (count % 150 == 75) {
            muki *= -1;
        }
    }

    // 噴射の予告 (180フレーム周期の 120フレーム前に鳴らす)
    if (count % 180 == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 噴水の発射 (180フレーム周期)
    if (count % 180 == 61) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFountain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0; // 敵の少し下から噴き出すように見せる
        pEnemyShotSet->muki = -DX_PI / 2.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 双方向リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 発射音 (水の勢いのある音として light を使用)
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
}