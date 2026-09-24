// enemyPat_Maze.cpp
// 迷路ステージ：速度0の敵弾で格子状の迷路を構築し、ボスへの到達を阻む
// 自機は左上、敵機は右下に固定。

#include "DxLib.h"
#include "gv.h"
#include <cmath>

// ============================================================
// 迷路の横壁を配置するヘルパー
//   x1〜x2 の区間に step 間隔で静止弾を並べる
// ============================================================
static void AddMazeWallH(sEnemyShotSet* pEnemyShotSet, double x1, double x2, double y, int imgKind)
{
    const double step = 6.0;
    for (double x = x1; x <= x2; x += step) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = x;
        pEnemyShot->y = y;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;   // 速度0で静止
        pEnemyShot->kind = imgKind;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }
}

// ============================================================
// 迷路の壁を生成するパターン関数
//   初回フレーム(count==0)のみ全壁を生成する
// ============================================================
static void ShotMazeWall(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count != 0) return;

    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

    const int wallKind = img_enemyShotMediumBall[3]; // シアンの中玉を壁に使う

    // 6x6グリッド（セル80px）
    // 横壁に左右交互のギャップを設け、ジグザグに誘導する
    AddMazeWallH(pEnemyShotSet, 0.0, 400.0, 80.0, wallKind); // ギャップ: 右 (x 400-480)
    AddMazeWallH(pEnemyShotSet, 80.0, 480.0, 160.0, wallKind); // ギャップ: 左 (x 0-80)
    AddMazeWallH(pEnemyShotSet, 0.0, 400.0, 240.0, wallKind); // ギャップ: 右 (x 400-480)
    AddMazeWallH(pEnemyShotSet, 80.0, 480.0, 320.0, wallKind); // ギャップ: 左 (x 0-80)
    AddMazeWallH(pEnemyShotSet, 0.0, 400.0, 400.0, wallKind); // ギャップ: 右 (x 400-480)
}

// ============================================================
// ボスの扇状弾幕
// ============================================================
static void ShotBossFan(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int num = 7;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            double angle = (i - (num - 1) / 2.0) * 12.0 / 180.0 * DX_PI;
            pEnemyShot->muki = pEnemyShotSet->muki + angle;
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤の小玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動（speed=0の壁弾はここには含まれない）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_FixedMaze_DeepSeek()
{
    static int shot_count;

    if (count == 1) {
        // 敵機は右下に固定
        enemy.x = 440.0;
        enemy.y = 440.0;
        enemy.maxHp = enemy.hp = 100; // 200で固定
        shot_count = 0;
        player.x = 30;
        player.y = 30;

        // 迷路の壁を配置するセットを生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMazeWall;
        pEnemyShotSet->x = 0.0;
        pEnemyShotSet->y = 0.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ボスの攻撃（90フレームごとに扇状弾幕）
    if (count > 1 && count % 90 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBossFan;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
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