// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：移動のみを行う（生成はボス側で行う）
static void ShotMove(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 弾追加ヘルパ関数
static void AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* ps = new sEnemyShot;
    ps->x = x;
    ps->y = y;
    ps->muki = muki;
    ps->speed = speed;
    ps->kind = kind;
    // count はメイン側でインクリメントされるため初期化不要

    // 循環 doubly linked list に追加
    ps->prev = pSet->pEnemyShotHead->prev;
    ps->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = ps;
    pSet->pEnemyShotHead->prev = ps;
}

// 敵本体のパターン：双環交差 ～ Twin Orbit Weave
void EnemyPat_TwoBoss_Qwen()
{
    static sEnemyShotSet* pSetA = nullptr;
    static sEnemyShotSet* pSetB = nullptr;

    // 状態管理変数
    static int rotDirA = 1;         // Aの回転方向 (1:時計回り, -1:反時計回り)
    static int rotDirB = -1;        // Bの回転方向
    static double startXA, startYA; // 補間用開始位置 A
    static double startXB, startYB; // 補間用開始位置 B
    static bool isSwapping = false; // 位置入れ替えフラグ
    static int swapTimer = 0;       // 入れ替え経過時間

    const int CYCLE = 360;          // 位置入れ替え周期 (6秒)
    const int SWAP_DUR = 60;        // 入れ替え移動時間 (1秒)
    const double CENTER_X = 240.0;
    const double CENTER_Y = 120.0;

    if (count == 1) {
        // 初期化
        enemy.maxHp = enemy.hp = 200; // 2体同時出現のためHP多め

        // ボスA用弾幕セット初期化
        pSetA = new sEnemyShotSet;
        pSetA->count = 0;
        pSetA->patternFunc = ShotMove;
        pSetA->pEnemyShotHead = new sEnemyShot;
        pSetA->pEnemyShotHead->prev = pSetA->pEnemyShotHead;
        pSetA->pEnemyShotHead->next = pSetA->pEnemyShotHead;
        pSetA->prev = enemyShotSetHead.prev;
        pSetA->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetA;
        enemyShotSetHead.prev = pSetA;

        // ボスB用弾幕セット初期化
        pSetB = new sEnemyShotSet;
        pSetB->count = 0;
        pSetB->patternFunc = ShotMove;
        pSetB->pEnemyShotHead = new sEnemyShot;
        pSetB->pEnemyShotHead->prev = pSetB->pEnemyShotHead;
        pSetB->pEnemyShotHead->next = pSetB->pEnemyShotHead;
        pSetB->prev = enemyShotSetHead.prev;
        pSetB->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetB;
        enemyShotSetHead.prev = pSetB;

        // 初期位置設定 (左A, 右B)
        enemy.x = 140.0; enemy.y = CENTER_Y;
        enemy.x2 = 340.0; enemy.y2 = CENTER_Y;

        startXA = enemy.x; startYA = enemy.y;
        startXB = enemy.x2; startYB = enemy.y2;

        rotDirA = 1;         // Aの回転方向 (1:時計回り, -1:反時計回り)
        rotDirB = -1;        // Bの回転方向
        isSwapping = false; // 位置入れ替えフラグ
        swapTimer = 0;       // 入れ替え経過時間
    }

    // --- ボス移動ロジック ---
    int phase = (count / CYCLE) % 2;    // 0: A左B右, 1: A右B左
    int t_mod = count % CYCLE;

    // 目標位置の計算
    double sideA = (phase == 0) ? -1.0 : 1.0;
    double targetXA = CENTER_X + 100.0 * sideA;
    double targetYA = CENTER_Y;
    double targetXB = CENTER_X - 100.0 * sideA;
    double targetYB = CENTER_Y;

    // 位置入れ替えトリガー
    if (t_mod == 0 && count > 1) {
        isSwapping = true;
        swapTimer = 0;
        startXA = enemy.x; startYA = enemy.y;
        startXB = enemy.x2; startYB = enemy.y2;

        // 回転方向反転
        rotDirA *= -1;
        rotDirB *= -1;

        // 予兆音再生
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (isSwapping) {
        // 入れ替え移動中 (線形補間)
        swapTimer++;
        double k = (double)swapTimer / SWAP_DUR;
        if (k >= 1.0) {
            k = 1.0;
            isSwapping = false;
        }
        enemy.x = startXA + (targetXA - startXA) * k;
        enemy.y = startYA + (targetYA - startYA) * k;
        enemy.x2 = startXB + (targetXB - startXB) * k;
        enemy.y2 = startYB + (targetYB - startYB) * k;

        // 結界（バリア）の展開：入れ替え中に中央に壁を作る
        if (swapTimer % 15 == 0) {
            AddShot(pSetA, CENTER_X, CENTER_Y, 0.0, 0.0, img_enemyShotLargeBall[5]); // マゼンタ大玉
        }
    }
    else {
        // 通常時：目標位置を中心とした円運動（振動）
        double oscX = 30.0 * cos(count * 0.04);
        double oscY = 20.0 * sin(count * 0.04);

        enemy.x = targetXA + oscX;
        enemy.y = targetYA + oscY;
        enemy.x2 = targetXB - oscX; // Xは対称、Yは同位相
        enemy.y2 = targetYB + oscY;
    }

    // --- 弾幕生成ロジック ---

    // 1. 双螺旋弾幕 (2フレームに1回)
    if (count % 2 == 0) {
        double angleBase = count * 0.08; // 回転速度

        // ボスA：赤中玉
        AddShot(pSetA, enemy.x, enemy.y + 10, angleBase * rotDirA, 2.5, img_enemyShotMediumBall[0]);
        // ボスB：青中玉 (向きをPIずらして対称に)
        AddShot(pSetB, enemy.x2, enemy.y2 + 10, angleBase * rotDirB + DX_PI, 2.5, img_enemyShotMediumBall[4]);
    }

    // 2. 自機狙い扇弾 (120フレームごと)
    if (count % 120 == 0) {
        // SE再生
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // ボスAからの5WAY
        double aimA = atan2(player.y - enemy.y, player.x - enemy.x);
        for (int i = 0; i < 5; i++) {
            double a = aimA + (i - 2) * 0.15; // 約8.5度間隔
            AddShot(pSetA, enemy.x, enemy.y + 10, a, 4.5, img_enemyShotBullet[6]); // 白銃弾
        }

        // ボスBからの5WAY
        double aimB = atan2(player.y - enemy.y2, player.x - enemy.x2);
        for (int i = 0; i < 5; i++) {
            double a = aimB + (i - 2) * 0.15;
            AddShot(pSetB, enemy.x2, enemy.y2 + 10, a, 4.5, img_enemyShotBullet[6]); // 白銃弾
        }
    }
}