// enemyPat_Tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾の役割
enum {
    ROLE_ORB_A = 0,  // オーブA
    ROLE_ORB_B = 1,  // オーブB
    ROLE_THREAD = 2, // 糸弾
    ROLE_NEEDLE = 3, // 針弾
    ROLE_AUX = 4,    // 補助弾
};

// 弾をセットの末尾に追加するヘルパー
static void AddBulletToSet(sEnemyShotSet* pSet, sEnemyShot* pShot)
{
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// リサジュー曲線弾幕パターン
static void LissajousPattern(sEnemyShotSet* pEnemyShotSet)
{
    // 初期化：オーブを2つ生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // オーブA（白い中玉）
        sEnemyShot* pOrbA = new sEnemyShot;
        pOrbA->x = pEnemyShotSet->x;
        pOrbA->y = pEnemyShotSet->y;
        pOrbA->muki = 0.0;
        pOrbA->speed = 0.0;
        pOrbA->kind = img_enemyShotMediumBall[6]; // 白
        pOrbA->param_i[0] = ROLE_ORB_A;
        pOrbA->margin = 200;
        AddBulletToSet(pEnemyShotSet, pOrbA);

        // オーブB（シアンの中玉）
        sEnemyShot* pOrbB = new sEnemyShot;
        pOrbB->x = pEnemyShotSet->x;
        pOrbB->y = pEnemyShotSet->y;
        pOrbB->muki = 0.0;
        pOrbB->speed = 0.0;
        pOrbB->kind = img_enemyShotMediumBall[3]; // シアン
        pOrbB->param_i[0] = ROLE_ORB_B;
        pOrbB->margin = 200;
        AddBulletToSet(pEnemyShotSet, pOrbB);
    }

    // オーブの現在位置を計算（リサジュー曲線）
    double t = pEnemyShotSet->count * 0.05;
    double orbAx = pEnemyShotSet->x + 120.0 * sin(3.0 * t) * 1.5;
    double orbAy = pEnemyShotSet->y + 80.0 * sin(2.0 * t) * 1.5;
    double orbBx = pEnemyShotSet->x + 120.0 * sin(3.0 * t + DX_PI) * 1.5;
    double orbBy = pEnemyShotSet->y + 80.0 * sin(2.0 * t + DX_PI) * 1.5;

    // 糸弾の生成（4フレームごとに各オーブから1個）
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 1 == 0) {
        // オーブAからの糸弾
        sEnemyShot* pThreadA = new sEnemyShot;
        pThreadA->x = orbAx;
        pThreadA->y = orbAy;
        pThreadA->muki = DX_PI / 2.0;          // 真下へゆっくり落下
        pThreadA->speed = 0.1;
        pThreadA->kind = img_enemyShotSmallBall[3]; // シアン
        pThreadA->param_i[0] = ROLE_THREAD;
        AddBulletToSet(pEnemyShotSet, pThreadA);

        // オーブBからの糸弾
        sEnemyShot* pThreadB = new sEnemyShot;
        pThreadB->x = orbBx;
        pThreadB->y = orbBy;
        pThreadB->muki = DX_PI / 2.0;
        pThreadB->speed = 0.1;
        pThreadB->kind = img_enemyShotSmallBall[3]; // シアン
        pThreadB->param_i[0] = ROLE_THREAD;
        AddBulletToSet(pEnemyShotSet, pThreadB);
    }

    // 補助弾の生成（2秒＝120フレームごとに各オーブから3-way自機狙い）
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 120 == 60) {
        double aimA = atan2(player.y - orbAy, player.x - orbAx);
        double aimB = atan2(player.y - orbBy, player.x - orbBx);

        for (int i = -1; i <= 1; ++i) {
            // オーブAの3-way
            sEnemyShot* pAuxA = new sEnemyShot;
            pAuxA->x = orbAx;
            pAuxA->y = orbAy;
            pAuxA->muki = aimA + i * (15.0 / 180.0 * DX_PI);
            pAuxA->speed = 2.0;
            pAuxA->kind = img_enemyShotScale[1]; // 黄
            pAuxA->param_i[0] = ROLE_AUX;
            AddBulletToSet(pEnemyShotSet, pAuxA);

            // オーブBの3-way
            sEnemyShot* pAuxB = new sEnemyShot;
            pAuxB->x = orbBx;
            pAuxB->y = orbBy;
            pAuxB->muki = aimB + i * (15.0 / 180.0 * DX_PI);
            pAuxB->speed = 2.0;
            pAuxB->kind = img_enemyShotScale[1]; // 黄
            pAuxB->param_i[0] = ROLE_AUX;
            AddBulletToSet(pEnemyShotSet, pAuxB);
        }
    }

    // 弾の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 念のため次を保持

        switch (pShot->param_i[0]) {
        case ROLE_ORB_A:
            // オーブAはリサジュー曲線上を移動（座標を直接設定）
            pShot->x = orbAx;
            pShot->y = orbAy;
            break;

        case ROLE_ORB_B:
            pShot->x = orbBx;
            pShot->y = orbBy;
            break;

        case ROLE_THREAD:
            // 糸弾はゆっくり下降
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 寿命（180フレーム）で針弾に変換
            if (pShot->count >= 180) {
                if (GetRand(3) == 0) {
                    pShot->param_i[0] = ROLE_NEEDLE;
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->speed = 3.2;
                    pShot->kind = img_enemyShotDiamond[0]; // 赤
                }
                else {
                    pShot->margin = -9999;
                }
            }
            break;

        case ROLE_NEEDLE:
        case ROLE_AUX:
            // 針弾・補助弾は直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;

        default:
            break;
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_Lissajous_DeepSeek()
{
    static int muki;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 150.0;   // 画面中央よりやや上（オーブの可動範囲が画面内に収まる）
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // 弾幕セットを生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = LissajousPattern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // ダミーヘッドを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵本体の横移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }
}