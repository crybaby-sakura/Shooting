#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 共通ヘルパー：シューティングリスト操作
// ============================================================

// 指定のショットセットの末尾に新しい弾を1つ追加して返す
static sEnemyShot* AddShotToSet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
    pEnemyShotSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 指定の弾をリストから外して破棄し、次の弾を返す（走査中の削除用）
static sEnemyShot* RemoveShotFromSet(sEnemyShot* pShot)
{
    sEnemyShot* pNext = pShot->next;
    pShot->prev->next = pShot->next;
    pShot->next->prev = pShot->prev;
    delete pShot;
    return pNext;
}

// ============================================================
// 弾幕：いがぐりの展開
// 菱形弾（緑）をリング状に配置し、回転しながら外側へ広げることで
// 焼き栗の「いが」がトゲトゲと開いていく様子を表現する。
// ============================================================
static void ShotIgaBurr(sEnemyShotSet* pEnemyShotSet)
{
    const int    IGA_NUM = 36;    // いがのトゲ（菱形弾）の数
    const double ANGLE_SPEED = 0.015; // 回転速度 [rad/frame]
    const double RADIUS_GROW = 0.55;  // 広がる速さ [px/frame]

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < IGA_NUM; i++) {
            sEnemyShot* pShot = AddShotToSet(pEnemyShotSet);
            double angle = (double)i / IGA_NUM * 2.0 * DX_PI;
            pShot->kind = img_enemyShotDiamond[2]; // 緑：いがぐりのトゲ
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = angle;          // トゲ先端が外を向くように
            pShot->speed = 0.0;           // 移動は角度・半径で直接計算するため未使用
            pShot->param_d[0] = angle;    // 現在の角度
            pShot->param_d[1] = 0.0;      // 現在の半径
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->param_d[0] += ANGLE_SPEED;
        pShot->param_d[1] += RADIUS_GROW;
        pShot->x = pEnemyShotSet->x + pShot->param_d[1] * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y + pShot->param_d[1] * sin(pShot->param_d[0]);
        pShot->muki = pShot->param_d[0]; // 回転しながら常に外向きを維持
        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕：栗の実が飛び出し、やがて爆ぜる
// 中楕円弾（橙）を全方位に飛ばし、一定時間後にその場で
// 小玉・銃弾・短レーザーへと変化させて「パンッ」と弾けさせる。
// 焼いている栗がハゼる瞬間の表現。
// ============================================================
static void ShotChestnut(sEnemyShotSet* pEnemyShotSet)
{
    const int CHESTNUT_NUM = 24; // 飛び出す栗の実の数
    const int BURST_SMALL = 10; // 爆ぜたときに散る小玉（実の欠片）の数
    const int BURST_SPARK = 2;  // 爆ぜたときに散る火花（銃弾）の数
    const int BURST_LASER = 2;  // 爆ぜた瞬間の閃光（短レーザー）の数

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < CHESTNUT_NUM; i++) {
            sEnemyShot* pShot = AddShotToSet(pEnemyShotSet);
            double angle = (double)i / CHESTNUT_NUM * 2.0 * DX_PI
                + (GetRand(200) - 100) / 1000.0; // 少しだけばらつかせる
            pShot->kind = img_enemyShotMediumOval[8]; // 橙：焼き栗の実
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = angle;
            pShot->speed = 1.6 + GetRand(150) / 100.0;
            pShot->param_i[0] = 0;               // 役割：0=飛行中の栗の実
            pShot->param_i[1] = 40 + GetRand(40); // 何フレーム後に爆ぜるか
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // --- 飛行中の栗の実 ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->param_i[1]--;

            if (pShot->param_i[1] <= 0) {
                // --- 爆ぜる！（パンッ） ---
                double bx = pShot->x, by = pShot->y;

                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                for (int j = 0; j < BURST_SMALL; j++) {
                    sEnemyShot* pFrag = AddShotToSet(pEnemyShotSet);
                    double a = (double)j / BURST_SMALL * 2.0 * DX_PI;
                    pFrag->kind = img_enemyShotSmallBall[0]; // 赤：弾けた実の欠片
                    pFrag->x = bx;
                    pFrag->y = by;
                    pFrag->muki = a;
                    pFrag->speed = 2.5 + GetRand(150) / 100.0;
                    pFrag->param_i[0] = 1; // 役割：1=あとは直進するだけの破片
                }
                for (int j = 0; j < BURST_SPARK; j++) {
                    sEnemyShot* pFrag = AddShotToSet(pEnemyShotSet);
                    pFrag->kind = img_enemyShotBullet[0]; // 赤：飛び散る火花
                    pFrag->x = bx;
                    pFrag->y = by;
                    pFrag->muki = GetRand(3600) / 3600.0 * 2.0 * DX_PI;
                    pFrag->speed = 5.0 + GetRand(200) / 100.0;
                    pFrag->param_i[0] = 1;
                }
                for (int j = 0; j < BURST_LASER; j++) {
                    sEnemyShot* pFrag = AddShotToSet(pEnemyShotSet);
                    pFrag->kind = img_enemyShotLaser[1]; // 黄：爆ぜた瞬間の閃光
                    pFrag->x = bx;
                    pFrag->y = by;
                    pFrag->muki = GetRand(3600) / 3600.0 * 2.0 * DX_PI;
                    pFrag->speed = 6.0 + GetRand(150) / 100.0;
                    pFrag->param_i[0] = 1;
                }

                pShot = RemoveShotFromSet(pShot);
                continue; // pShotは既に次を指しているのでそのまま次のループへ
            }
        }
        else {
            // --- 爆ぜた後の破片：まっすぐ飛ぶだけ ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン：焼き栗
// 「いがぐりが開く → 栗の実が飛び出す → 実が爆ぜる」を
// 一定間隔で繰り返すボスパターン。
// ============================================================
void EnemyPat_RoastChestnut_Claude()
{
    static int muki;

    const int WAVE_INTERVAL = 150; // 1波（いが→栗）の周期 [frame]
    const int CHESTNUT_DELAY = 55;  // いが展開開始から栗の実が飛び出すまでの遅延 [frame]

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (count % 200 == 100) muki *= -1;
    }

    // 第1段階：いがぐりの展開
    if (count % WAVE_INTERVAL == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotIgaBurr;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 第2・3段階：栗の実が飛び出し、やがて爆ぜる
    if (count % WAVE_INTERVAL == CHESTNUT_DELAY) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotChestnut;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}