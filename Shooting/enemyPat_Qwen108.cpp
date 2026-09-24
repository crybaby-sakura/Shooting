// enemyPat_Tmp.cpp
// 格子状の迷路を実装した敵パターン

#include "gv.h"

// 迷路用の弾処理（速度0のため移動処理は不要）
static void ShotMaze(sEnemyShotSet* pEnemyShotSet)
{
    // 静止している壁のため、特に毎フレームの移動処理は行わない
    // 必要に応じてここで count に応じた演出（点滅など）を追加可能
}

void EnemyPat_FixedMaze_Qwen()
{
    if (count == 1) {
        // 自機の初期位置を左上に設定
        player.x = 40.0;
        player.y = 40.0;

        // 敵機の位置を右下に固定
        enemy.x = 440.0;
        enemy.y = 440.0;
        enemy.maxHp = enemy.hp = 10;

        // 効果音再生（重厚な音で迷路の生成を演出）
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 迷路用の弾セットを作成
        sEnemyShotSet* pMazeSet = new sEnemyShotSet;
        pMazeSet->count = 0;
        pMazeSet->patternFunc = ShotMaze;
        pMazeSet->x = 0.0;
        pMazeSet->y = 0.0;
        pMazeSet->muki = 0.0;
        pMazeSet->kind = 0;

        pMazeSet->pEnemyShotHead = new sEnemyShot;
        pMazeSet->pEnemyShotHead->prev = pMazeSet->pEnemyShotHead;
        pMazeSet->pEnemyShotHead->next = pMazeSet->pEnemyShotHead;

        pMazeSet->prev = enemyShotSetHead.prev;
        pMazeSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMazeSet;
        enemyShotSetHead.prev = pMazeSet;

        // ==========================================
        // 格子状の迷路生成
        // 格子の間隔は 80
        // 縦壁: x = 80, 160, 240, 320, 400
        // 横壁: y = 80, 160, 240, 320, 400
        // 自機は x=40 の列を下り、y=440 の行を右に進む経路を確保する
        // ==========================================

        // 1. 交差点に大玉を配置して隙間を埋める (x=80..400, y=80..400)
        // 弾の色: 4:青
        for (int gx = 80; gx <= 400; gx += 80) {
            for (int gy = 80; gy <= 400; gy += 80) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = (double)gx;
                pShot->y = (double)gy;
                pShot->muki = 0.0;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotLargeBall[4];

                pShot->prev = pMazeSet->pEnemyShotHead->prev;
                pShot->next = pMazeSet->pEnemyShotHead;
                pMazeSet->pEnemyShotHead->prev->next = pShot;
                pMazeSet->pEnemyShotHead->prev = pShot;
            }
        }

        // 2. 縦壁の短レーザーを配置 (交差点を除く)
        // 弾の色: 6:白
        for (int gx = 80; gx <= 400; gx += 80) {
            for (int gy = 40; gy <= 360; gy += 80) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = (double)gx;
                pShot->y = (double)gy;
                pShot->muki = DX_PI / 2; // 縦向き
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotLaser[6];

                pShot->prev = pMazeSet->pEnemyShotHead->prev;
                pShot->next = pMazeSet->pEnemyShotHead;
                pMazeSet->pEnemyShotHead->prev->next = pShot;
                pMazeSet->pEnemyShotHead->prev = pShot;
            }
        }

        // 3. 横壁の短レーザーを配置 (交差点を除く)
        // 弾の色: 6:白
        // ※ gx=40 は自機の経路なので配置しない
        for (int gy = 80; gy <= 400; gy += 80) {
            for (int gx = 120; gx <= 440; gx += 80) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = (double)gx;
                pShot->y = (double)gy;
                pShot->muki = 0.0; // 横向き
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotLaser[6];

                pShot->prev = pMazeSet->pEnemyShotHead->prev;
                pShot->next = pMazeSet->pEnemyShotHead;
                pMazeSet->pEnemyShotHead->prev->next = pShot;
                pMazeSet->pEnemyShotHead->prev = pShot;
            }
        }
    }
}