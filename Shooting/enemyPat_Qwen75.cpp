// enemyPat_Tmp.cpp (または既存のファイルに追記)

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕パターン：星符「五芒星牢（ペンタグラム・プリズン）」
// ============================================================
static void ShotPentagram(sEnemyShotSet* pSet)
{
    // パラメータ読み込み
    double R = pSet->param_d[0];
    double baseAngle = pSet->param_d[1];
    double spiralAngle = pSet->param_d[2];

    // 状態更新
    baseAngle += 0.0015; // 五芒星の回転速度
    spiralAngle += 0.04; // 中央螺旋の回転速度

    // 更新値を保存
    pSet->param_d[1] = baseAngle;
    pSet->param_d[2] = spiralAngle;

    // 中心座標（ボス位置に追従させる）
    double cx = enemy.x;
    double cy = enemy.y;

    // 五芒星の頂点計算
    // 72度間隔で5つの頂点を配置。-DX_PI/2 で最初の頂点を真上にする。
    double vx[5], vy[5];
    for (int i = 0; i < 5; i++) {
        double a = baseAngle + i * (DX_PI * 2.0 / 5.0) - DX_PI / 2.0;
        vx[i] = cx + R * cos(a);
        vy[i] = cy + R * sin(a);
    }

    // 辺の接続定義 (五芒星の描画順: 0->2->4->1->3->0)
    int edges[5][2] = { {0, 2}, {2, 4}, {4, 1}, {1, 3}, {3, 0} };

    // --------------------------------------------------------
    // 1. 辺弾 (星の線を描く)
    // --------------------------------------------------------
    // 4フレームに1回、各辺の始点から終点へ向けて弾を生成
    if (pSet->count % 4 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            int s = edges[i][0];
            int e = edges[i][1];

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = vx[s];
            pShot->y = vy[s];
            pShot->muki = atan2(vy[e] - vy[s], vx[e] - vx[s]);
            pShot->speed = 3.2;
            pShot->kind = img_enemyShotDiamond[4]; // 4:青 (シアン寄りの青)
            pShot->margin = 240;

            // リストへ追加
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // --------------------------------------------------------
    // 2. 頂点弾 (プレイヤーへの狙い撃ち)
    // --------------------------------------------------------
    // 80フレームに1回、各頂点から3way弾を発射
    if (pSet->count % 80 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            double angle = atan2(player.y - vy[i], player.x - vx[i]);

            // 3way (-1, 0, 1)
            for (int j = -1; j <= 1; j++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = vx[i];
                pShot->y = vy[i];
                pShot->muki = angle + j * 0.12; // 約7度間隔
                pShot->speed = 4.2;
                pShot->kind = img_enemyShotMediumBall[0]; // 0:赤

                // リストへ追加
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // --------------------------------------------------------
    // 3. 中央螺旋 (五角形の渦)
    // --------------------------------------------------------
    // 6フレームに1回、中心から5方向へ弾を発射
    if (pSet->count % 6 == 0) {
        for (int i = 0; i < 5; i++) {
            double a = spiralAngle + i * (DX_PI * 2.0 / 5.0);

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = cx;
            pShot->y = cy;
            pShot->muki = a;
            pShot->speed = 2.5;
            pShot->kind = img_enemyShotSmallBall[1]; // 1:黄

            // リストへ追加
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // --------------------------------------------------------
    // 座標更新
    // --------------------------------------------------------
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }

    // --------------------------------------------------------
    // ループ処理
    // --------------------------------------------------------
    // 600フレーム (約10秒) でサイクルをリセット
    // pSet->count はメインルーチンがインクリメントするため、
    // ここで 0 に戻すことで、次のフレームから再び 1, 2... とカウントされループする。
    if (pSet->count > 600) {
        pSet->count = 0;
        // ループ時に角度をずらし、同じパターンに見えないようにする
        pSet->param_d[1] += 0.3;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_Pentagram_Qwen()
{
    static bool spawned = false;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 弾幕確認用にHP多め
        spawned = false;
    }

    // ボスの移動 (サイン波による往復運動)
    enemy.x = 240.0 + sin(count * 0.015) * 140.0;
    enemy.y = 80.0 + sin(count * 0.025) * 20.0;

    // 弾幕セットの生成 (1回だけ行う)
    // 出現演出(60フレーム)が終わった後に生成
    if (!spawned && count > 60) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPentagram;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0;
        pSet->kind = 0;

        // パラメータ初期化
        pSet->param_d[0] = 120.0; // 五芒星の半径 R
        pSet->param_d[1] = 0.0;   // 基準角度 (baseAngle)
        pSet->param_d[2] = 0.0;   // 螺旋角度 (spiralAngle)

        // 弾リストのダミーヘッド初期化
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルリストへ接続
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        spawned = true;
    }
}