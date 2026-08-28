// enemyPat_brownian.cpp
// ブラウン運動をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン：ブラウン運動する花粉弾
static void BrownianShot(sEnemyShotSet* pEnemyShotSet)
{
    // このセットが持つ全弾について更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // param_i[0] は方向転換までのカウントダウン
        pShot->param_i[0]--;

        // カウントダウンが0になったら方向をランダムに変える
        if (pShot->param_i[0] <= 0) {
            // 次の方向転換までのフレーム数を 10～40 の範囲でランダムに決める
            pShot->param_i[0] = GetRand(30) + 10;

            // 現在の向きから ±30° の範囲でランダムに変更
            double angleChange = (GetRand(60) - 30) / 180.0 * DX_PI;
            pShot->muki += angleChange;

            // 角度を正規化（オプション：0～2π に収める）
            if (pShot->muki < 0.0) pShot->muki += 2.0 * DX_PI;
            if (pShot->muki >= 2.0 * DX_PI) pShot->muki -= 2.0 * DX_PI;
        }

        // 現在の向きで移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_BrownianMotion_DeepSeek()
{
    static int muki;          // 敵の横移動方向
    static int shot_count;    // 弾セット生成用のカウンタ

    // 初期化
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵をゆっくり左右に動かす
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔（180フレームごと）で弾セットを生成
    if (count % 40 == 1) {
        // 新しい弾セットを作成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = BrownianShot; // ブラウン運動パターン
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 使用しない（各弾がランダムな初期方向を持つ）
        pEnemyShotSet->kind = shot_count++;

        // 弾リストの初期化（ダミーヘッド）
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // このセットに 80 発の弾を追加
        const int bulletNum = 80;
        for (int i = 0; i < bulletNum; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 初期位置：敵の周囲に少しばらつかせる
            pEnemyShot->x = pEnemyShotSet->x + GetRand(20) - 10;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;

            // 初期方向：完全ランダム（0～360°）
            pEnemyShot->muki = GetRand(359) / 180.0 * DX_PI;

            // 速度：1.0～2.0 px/frame（ゆっくり漂う）
            pEnemyShot->speed = (100 + GetRand(100)) / 100.0;

            // 弾の種類：小玉（花粉のように小さく）
            // 色は i % 9 で 0～8 の全色を順番に使う
            pEnemyShot->kind = img_enemyShotSmallBall[i % 9];

            // 方向転換までのカウントダウンを初期化（10～40フレーム）
            pEnemyShot->param_i[0] = GetRand(30) + 10;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // この弾セットをグローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}