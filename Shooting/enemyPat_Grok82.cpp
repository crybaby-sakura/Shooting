// enemyPat_Tmp.cpp
// 「布団が吹っ飛んだ」モチーフ弾幕
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：布団形成 → 一気に吹き飛ばし
static void ShotFuton(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 形成フェーズ（count == 0 で布団を展開）
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 布団のグリッド設定（横長の長方形）
        const int cols = 12 * 2;          // 横方向の弾数
        const int rows = 4 * 3;           // 縦方向の弾数
        const double spacingX = 13.0; // 横間隔
        const double spacingY = 9.0;  // 縦間隔

        double startX = pEnemyShotSet->x - (cols - 1) * spacingX / 2.0;
        double startY = pEnemyShotSet->y - (rows - 1) * spacingY / 2.0;

        for (int j = 0; j < rows; j++) {
            for (int i = 0; i < cols; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = startX + i * spacingX;
                pEnemyShot->y = startY + j * spacingY;
                pEnemyShot->muki = -DX_PI / 2.0; // 上向き（待機）
                pEnemyShot->speed = 0.0;         // 形成中は静止

                // 白の中玉で布団を表現（色6:白）
                // 中玉(7.0x7.0)を使用。密度を出して一枚の布団に見せる
                pEnemyShot->kind = img_enemyShotMediumBall[6];

                // 後で散開計算用に元位置を保存
                pEnemyShot->param_d[0] = pEnemyShot->x; // 元のx
                pEnemyShot->param_d[1] = pEnemyShot->y; // 元のy
                pEnemyShot->param_i[0] = i;             // 列インデックス
                pEnemyShot->param_i[1] = j;             // 行インデックス

                // リスト接続
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 吹き飛ばし瞬間（約1秒後）
    if (pEnemyShotSet->count == 55) {
        // 強烈な発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            double dx = pShot->param_d[0] - pEnemyShotSet->x; // 中心からの横オフセット

            // 基本は上方向へ。外側ほど大きく左右に広がる
            double baseAngle = -DX_PI / 2.0;
            double spread = dx / 80.0; // オフセットに比例した広がり
            // ランダムゆらぎを追加（GetRandは 0〜x の x+1 種類）
            double randAngle = (GetRand(50) - 25) / 180.0 * DX_PI;

            pShot->muki = baseAngle + spread + randAngle;

            // 速度：中央ほど速く、外側もそれなりに高速
            // 3.0〜6.5 程度
            pShot->speed = 3.2 + GetRand(33) / 10.0;

            // たまに端の弾がゆっくり下方向へ落ちる（布団の端がめくれるイメージ）
            if (GetRand(7) == 0) {
                pShot->muki = DX_PI / 2.0 + (GetRand(80) - 40) / 180.0 * DX_PI;
                pShot->speed = 0.8 + GetRand(20) / 10.0;
            }

            pShot = pShot->next;
        }
    }

    // 毎フレーム移動（形成中はspeed=0なので静止）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_FutonFlewAway_Grok()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.55 * (double)muki;
        if (count % 160 == 80) muki *= -1;

        // 画面端で反転（安全策）
        if (enemy.x < 60.0) {
            enemy.x = 60.0;
            muki = 1;
        }
        if (enemy.x > 420.0) {
            enemy.x = 420.0;
            muki = -1;
        }
    }

    // 一定間隔で布団弾幕を発生
    if (count % 100 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFuton;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 25.0;
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
}