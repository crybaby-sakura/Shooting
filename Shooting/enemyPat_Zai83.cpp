// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：交差する七色のアーチ
static void ShotRainbowArc(sEnemyShotSet* pEnemyShotSet)
{
    // 虹の7色に対応する色インデックス (赤、橙、黄、緑、シアン、青、マゼンタ)
    static const int colors[7] = { 0, 8, 1, 2, 3, 4, 5 };

    // 7色分のアークを時間差で順番に発射
    for (int i = 0; i < 7; i++) {
        if (pEnemyShotSet->count == 30 + i * 10) {

            // 角度の範囲 (内側の赤が狭く、外側のマゼンタが広い)
            double angleRange = (30.0 + i * 20.0) * DX_PI / 180.0;
            // 弾の数 (外側になるほどアーチが長くなるため増加)
            int bulletNum = 10 + i * 7;
            // 目標半径 (内側が短く、外側が長い)
            double targetR = 80.0 + i * 25.0;
            // 回転角速度 (内側ほど速く、奇数番目と偶数番目で逆回転)
            double rotSpeed = (i % 2 == 0) ? (0.008 - i * 0.0005) : -(0.008 - i * 0.0005);

            for (int j = 0; j < bulletNum; j++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                // 中玉を使用
                pEnemyShot->kind = img_enemyShotMediumBall[colors[i]];
                // 自前で座標計算するため速度は0に設定（メインルーチンの移動処理と競合しないように）
                pEnemyShot->speed = 0.0;
                pEnemyShot->muki = 0.0;

                // 極座標パラメータの初期化
                pEnemyShot->param_d[0] = targetR;                                                            // [0] 目標半径
                pEnemyShot->param_d[1] = 0.0;                                                               // [1] 現在の半径
                pEnemyShot->param_d[2] = -angleRange + (2.0 * angleRange / (bulletNum - 1)) * j;            // [2] 基準角度
                pEnemyShot->param_d[3] = rotSpeed;                                                          // [3] 回転角速度
                pEnemyShot->param_d[4] = 0.0;                                                               // [4] 展開完了後の経過フレーム

                // 初期位置は敵の位置
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->margin = 480;

                // 双方向リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 全弾の移動・回転処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // 弾を目標半径に向かって展開
        pShot->param_d[1] += 2.5;

        // 目標半径に到達したら
        if (pShot->param_d[1] >= pShot->param_d[0]) {
            pShot->param_d[1] = pShot->param_d[0]; // 目標半径で張り付く
            pShot->param_d[4] += 1.0;              // 展開完了後のカウンタを加算
            pShot->param_d[2] += pShot->param_d[3]; // 角度を加算して回転させる

            // 一定時間（300フレーム）経過したら、目標半径を広げて画面外へ消しに行かせる
            if (pShot->param_d[4] > 300.0) {
                pShot->param_d[0] += 1.5;
            }
        }

        // 極座標から直交座標へ変換して座標を更新
        pShot->x = pEnemyShotSet->x + pShot->param_d[1] * cos(pEnemyShotSet->param_d[0] + pShot->param_d[2]);
        pShot->y = pEnemyShotSet->y + pShot->param_d[1] * sin(pEnemyShotSet->param_d[0] + pShot->param_d[2]);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Rainbow_Zai()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 500フレームごとに虹の弾幕を発動
    if (count % 300 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRainbowArc;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 本弾幕では使用しない
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_d[0] = GetRand(100) / 100.0 * 2.0 * DX_PI;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 予告音を再生
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}