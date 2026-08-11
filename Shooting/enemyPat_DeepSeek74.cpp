// EnemyPat_TarakoSpaghetti_DeepSeek.cpp
// たらこスパゲッティモチーフ弾幕「明太子の大渦巻き」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 螺旋麺弾パターン
static void ShotSpiralNoodle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 軽い発射音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int NUM = 240;                     // 麺の本数
        const double BASE_DIST = 200.0;           // 初期距離の基準
        const double SPEED = 1.5;                // 速度
        const double ANGULAR_VEL = 0.025;        // 角速度（ラジアン/フレーム）

        double baseAngle = pEnemyShotSet->muki;  // セット生成時の向き

        for (int i = 0; i < NUM; ++i) {
            sEnemyShot* pShot = new sEnemyShot;

            double angle = baseAngle + i * (2.0 * DX_PI / NUM);
            double dist = BASE_DIST + (i % 15) * 2.5;   // 少しばらつかせる
            pShot->x = pEnemyShotSet->x + dist * cos(angle);
            pShot->y = pEnemyShotSet->y + dist * sin(angle);

            pShot->muki = angle + DX_PI / 2.0;          // 接線方向に飛ばす
            pShot->speed = SPEED;
            pShot->kind = img_enemyShotSmallBall[1];    // 小玉／黄（クリーム色）
            pShot->param_d[0] = ANGULAR_VEL;            // 角速度を保存
            pShot->margin = 480;

            // 双方向リンクリストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 毎フレーム移動（角度を加算して渦を描く）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->muki += pShot->param_d[0];
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 粒弾（たらこ）パターン
static void ShotTarakoGrains(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 中程度の発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int NUM = 12;                      // 12方向に飛び散る
        double baseAngle = pEnemyShotSet->muki;  // 敵から自機への角度

        for (int i = 0; i < NUM; ++i) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = baseAngle + i * (2.0 * DX_PI / NUM);

            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = angle;
            pShot->speed = 3.0;                   // 拡散速度
            pShot->kind = img_enemyShotSmallBall[0]; // 小玉／赤（たらこ色）

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 状態遷移しながら移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int c = pShot->count;   // メインルーチンでインクリメント済み
        if (c < 30) {
            // 拡散中：何も変えない
        }
        else if (c == 30) {
            // ぴたりと停止
            pShot->speed = 0.0;
        }
        else if (c == 60) {
            // 自機めがけて再加速
            double angle = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->muki = angle;
            pShot->speed = 4.5;
        }
        // 常に現在の速度と向きで移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_TarakoSpaghetti_DeepSeek()
{
    if (count == 1) {
        // 初期化：ボスは画面中央、お皿のイメージ
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;

        // 常時展開する螺旋麺弾のセットを生成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSpiralNoodle;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;   // 渦の開始角度
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        // 敵本体は中央に留まり、ゆっくりとした動き（ここでは固定）
        // 必要に応じて微動を入れることも可能
        enemy.x = 240.0;
        enemy.y = 240.0;
    }

    // 一定間隔で粒弾（たらこ）をばら撒く
    if (count % 50 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTarakoGrains;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x); // 自機方向基準
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}