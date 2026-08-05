// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

#define PI 3.14159265358979323846

// ---------------------------------------------------------
// 第1フェーズ：偽りの安息地（幾何学模様）
// ---------------------------------------------------------
static void Phase1_Geometry(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 24方向にゆっくりとした白い中玉を発射（隙間が空いているように見える）
        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (PI * 2.0 / 24.0) * i;
            pEnemyShot->speed = 1.5;
            pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 第1フェーズ：絶対死亡領域（隙間を塞ぐ超高速罠弾）
// ---------------------------------------------------------
static void Phase1_Trap(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 幾何学模様の「綺麗に空いた隙間」をピンポイントで狙う超高速弾
        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 弾と弾のちょうど中間を狙う角度（+PI/24）
            pEnemyShot->muki = (PI * 2.0 / 24.0) * i + (PI / 24.0);
            pEnemyShot->speed = 12.0; // 視認できないほどの超高速
            pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒（背景に紛れさせる）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 第2フェーズ：視界の奪還と超高速微積分
// ---------------------------------------------------------
static void Phase2_InvisibleAndCross(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 30フレームごとに、画面外からボス位置めがけて見えない超高速弾を発射
    if (pEnemyShotSet->count % 30 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;

            // ボスを中心とした半径300の円周上から発射
            // GetRand(628) は 0〜628 を返すので、100.0で割ると 0.00〜6.28 になる
            double angle = GetRand(628) / 100.0;
            double radius = 300.0;
            pEnemyShot->x = enemy.x + cos(angle) * radius;
            pEnemyShot->y = enemy.y + sin(angle) * radius;

            // 現在のボス位置に向かう（移動に追従するため直接enemyを参照）
            pEnemyShot->muki = atan2(enemy.y - pEnemyShot->y, enemy.x - pEnemyShot->x) + (GetRand(20) - 10) / 100.0;
            pEnemyShot->speed = 15.0; // 猛スピード
            pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 5フレームごとに、目立つ色で交差弾幕を展開（視界のカオス化）
    if (pEnemyShotSet->count % 5 == 0) {
        // 時計回り
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = enemy.x;
        pEnemyShot->y = enemy.y;
        pEnemyShot->muki = (pEnemyShotSet->count / 5.0) * 0.3;
        pEnemyShot->speed = 4.0;
        pEnemyShot->kind = img_enemyShotDiamond[5]; // マゼンタの菱形弾

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 反時計回り
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = enemy.x;
        pEnemyShot->y = enemy.y;
        pEnemyShot->muki = (pEnemyShotSet->count / 5.0) * 0.3 + PI;
        pEnemyShot->speed = 4.0;
        pEnemyShot->kind = img_enemyShotScale[3]; // シアンの鱗弾

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 第3フェーズ：次元の圧縮（短レーザーによる壁の四方からの迫り）
// ---------------------------------------------------------
static void Phase3_LaserWall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 短レーザー(64.0x4.0)を隙間なく敷き詰めて壁にする
        // 40ピクセル間隔で重ねて配置すれば、64ピクセル幅のレーザーが隙間なく覆う
        for (int pos = -32; pos <= 512; pos += 40) {
            // 下から上への壁
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pos;
            pEnemyShot->y = 520.0;
            pEnemyShot->muki = -PI / 2.0;
            pEnemyShot->speed = 1.2; // ゆっくり中央へ迫る
            pEnemyShot->kind = img_enemyShotLaser[0]; // 赤い短レーザー
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 上から下への壁
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pos;
            pEnemyShot->y = -40.0;
            pEnemyShot->muki = PI / 2.0;
            pEnemyShot->speed = 1.2;
            pEnemyShot->kind = img_enemyShotLaser[0];
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 左から右への壁
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = -40.0;
            pEnemyShot->y = pos;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 1.2;
            pEnemyShot->kind = img_enemyShotLaser[0];
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 右から左への壁
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 520.0;
            pEnemyShot->y = pos;
            pEnemyShot->muki = PI;
            pEnemyShot->speed = 1.2;
            pEnemyShot->kind = img_enemyShotLaser[0];
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 第3フェーズ：完全ランダム砲火（運ゲーの極み）
// ---------------------------------------------------------
static void Phase3_RandomBarrage(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 毎フレーム大量発射
    if (pEnemyShotSet->count % 10 == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    for (int i = 0; i < 5; i++) {
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = enemy.x;
        pEnemyShot->y = enemy.y;

        // 完全にランダムな角度と速度
        pEnemyShot->muki = GetRand(628) / 100.0;
        pEnemyShot->speed = (GetRand(500) + 100) / 100.0; // 1.0 〜 6.0

        // 視認性を最悪にするため、大きさの違うランダムな色の弾を混ぜる
        int rndKind = GetRand(2);
        if (rndKind == 0)      pEnemyShot->kind = img_enemyShotSmallBall[0];  // 赤小玉
        else if (rndKind == 1) pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄中玉
        else                   pEnemyShot->kind = img_enemyShotLargeBall[2];  // 緑大玉

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// =========================================================
// 敵本体のパターン
// =========================================================
void EnemyPat_Tmp()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // 開始直後に予告音を鳴らす
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // ボスはゆっくり左右に移動
        enemy.x += 0.5 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // -------------------------------------------------
    // 第1フェーズ (0秒〜4秒 : count 1〜239)
    // -------------------------------------------------
    if (count == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Phase1_Geometry;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    if (count == 90) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Phase1_Trap;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // -------------------------------------------------
    // 第2フェーズ (4秒〜9秒 : count 240〜539)
    // -------------------------------------------------
    if (count == 240) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Phase2_InvisibleAndCross;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // -------------------------------------------------
    // 第3フェーズ (9秒〜 : count 540〜)
    // -------------------------------------------------
    if (count == 540) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Phase3_LaserWall;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    if (count == 600) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Phase3_RandomBarrage;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
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