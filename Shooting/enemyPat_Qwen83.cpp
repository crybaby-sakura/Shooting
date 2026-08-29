// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：七彩分光アーク
static void ShotRainbowArc(sEnemyShotSet* pEnemyShotSet)
{
    // 初回のみ弾を生成
    if (pEnemyShotSet->count == 0) {
        // 軽やかで美しい発射音を選択
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 虹の7色定義 (赤, 橙, 黄, 緑, 青, 藍(シアンで代用), 紫(マゼンタで代用))
        int rainbowColors[7] = { 0, 8, 1, 2, 4, 3, 5 };

        // 速度設定 (外側の色ほど速く、広い軌道を描く)
        double speeds[7] = { 4.0, 3.8, 3.6, 3.4, 3.2, 3.0, 2.8 };

        // 角速度設定 (内側の色ほど急激に曲がる)
        // 回転半径 r = speed / angularVel となり、赤は約400、紫は約93の円弧を描く
        double angularVels[7] = { 0.010, 0.012, 0.015, 0.018, 0.022, 0.026, 0.030 };

        // 発射角度範囲 (右斜め下 から 左斜め下)
        double angle_start = DX_PI * 0.2;
        double angle_end = DX_PI * 0.8;
        int shotsPerColor = 13; // 各色あたりの発射数（隙間なく虹を埋める数）

        for (int c = 0; c < 7; c++) {
            double angle_step = (angle_end - angle_start) / (shotsPerColor - 1);

            for (int i = 0; i < shotsPerColor; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                double muki = angle_start + i * angle_step;

                // 左側(0〜PI/2)は右回り(+)、右側(PI/2〜PI)は左回り(-)にして、
                // 画面中央上で頂点を持つ対称なアーチを描かせる
                double sign = (muki < DX_PI / 2.0) ? 1.0 : -1.0;

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = muki;
                pEnemyShot->speed = speeds[c];

                // param_d[0] に角速度を保存（メイン側でインクリメントされない自由パラメータとして使用）
                pEnemyShot->param_d[0] = angularVels[c] * sign;

                // 短レーザーを使用し、進行方向を向かせることで「帯状」の虹を表現
                pEnemyShot->kind = img_enemyShotLaser[rainbowColors[c]];
                pEnemyShot->margin = 480;

                // 双方向リンクリストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 向きを回転させて弧を描かせる（曲射弾ギミック）
        pShot->muki += pShot->param_d[0];

        // 位置を更新
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        if (pShot->count >= 180 && abs(pShot->x - pEnemyShotSet->x) <= 30) pShot->margin = -9999;

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Rainbow_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0; // やや上めで固定し、虹が画面全体に広がるようにする
        enemy.maxHp = enemy.hp = 200; // 耐久値

        // パターン開始時の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 敵は画面中央上部で左右にゆっくりと優雅に往復する
        enemy.x = 240.0 + 100.0 * sin(count * 0.015);
        enemy.y = 80.0 + 20.0 * cos(count * 0.03); // 上下にもわずかに揺らす
    }

    // 180フレーム(約3秒)ごとに虹のアーチを発射
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRainbowArc;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0; // 敵の少し下から発射
        pEnemyShotSet->muki = DX_PI / 2.0; // 基準向きは真下
        pEnemyShotSet->kind = 0;

        // 弾リストのヘッド初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}