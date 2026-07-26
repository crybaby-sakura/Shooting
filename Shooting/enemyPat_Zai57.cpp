// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ルール30（非対称拡散弾幕）
static void ShotRule30(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // param_i から 64ビットのセル状態を復元
        uint64_t state = (uint64_t)(unsigned int)pEnemyShotSet->param_i[0]
            | ((uint64_t)(unsigned int)pEnemyShotSet->param_i[1] << 32);

        // 画面中央(240)を基準に、1セル20ピクセルで24セル分を配置
        // ビット位置 20 〜 43 を画面左端から右端にマッピング
        double base_x = -50.0;
        double base_y = pEnemyShotSet->y;

        // 弾の速度設定 (X: -0.4 左方向へドリフト, Y: 2.0 降下)
        //double vx = -0.4;
        //double vy = 2.0;
        //double speed = sqrt(vx * vx + vy * vy);
        //double muki = atan2(vy, vx);
        double muki = 2 * DX_PI / 3 + sin(count * 0.007) * DX_PI / 6;
        double speed = 2.5;
        double vx = speed * cos(muki);
        double vy = speed * sin(muki);

        // 色はシアン(3)で統一し、デジタルな見た目に
        int color = 3;

        for (int i = 0; i < 64; i++) {
            uint64_t mask = 1ULL << i;
            if (state & mask) {
                pEnemyShot = new sEnemyShot;

                pEnemyShot->x = base_x + i * 15.0;
                pEnemyShot->y = base_y;
                pEnemyShot->muki = muki;
                pEnemyShot->speed = speed;

                // 小玉(2.5x2.5)を使用
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                pEnemyShot->margin = 480;

                // 双方向リストに追加
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
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Rule30_Zai()
{
    static int muki;
    static uint64_t state;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // ビット32のみが立った状態（画面中央）からスタート
        state = 1ULL << 63;

        // 弾幕開始の予告音を再生
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 敵本体はゆっくり左右に移動（見た目の演出）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 5フレームに1回、1世代分の弾幕を生成
    if (count > 0 && count % 5 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRule30;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 64ビットの state を32ビットずつに分割して保存
        pEnemyShotSet->param_i[0] = (int)(state & 0xFFFFFFFF);
        pEnemyShotSet->param_i[1] = (int)((state >> 32) & 0xFFFFFFFF);

        // 循環リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 次の世代の状態を計算 (ルール30のビット演算式)
        state = state ^ (state >> 1);
    }
}