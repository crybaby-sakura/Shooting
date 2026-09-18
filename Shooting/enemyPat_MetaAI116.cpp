// enemyPat_Tmp.cpp - Wilson霧箱モチーフ
#include "DxLib.h"
#include "gv.h"
#include <math.h>

// 外部素材 - プロジェクト側で定義済み
extern int img_enemyShotSmallBall[9];
extern int img_enemyShotMediumBall[9];
extern int img_enemyShotScale[9];
extern int sound_enemyShot_light;
extern int sound_enemyShot_medium;
extern int sound_enemyCharge;

// ------------------------------------------------------------
// 霧用：画面に低速の鱗弾をばら撒く
// ------------------------------------------------------------
static void ShotFog(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 35個の霧
        for (int i = 0; i < 35; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            while (true) {
                pShot->x = pSet->x + (double)(GetRand(480) - 240);
                pShot->y = pSet->y + (double)(GetRand(480) - 240);
                if (hypot(pShot->x - player.x, pShot->y - player.y) > 30.0) break;
            }
            pShot->muki = (double)GetRand(360) / 180.0 * DX_PI;
            pShot->speed = 0.1 + (double)GetRand(30) / 100.0; // 0.1 - 0.4

            // 白とシアンを交互に。GetRandは0-x inclusiveなので注意
            int col = (GetRand(1) == 0) ? 6 : 3; // 6:白 3:シアン
            pShot->kind = img_enemyShotScale[col];

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        // ふらつき
        p->muki += (GetRand(20) - 10) / 1000.0;
        p = p->next;
    }
}

// ------------------------------------------------------------
// 本命：Wilson Track
// param_d[0],d[1] : 不可視粒子Pの現在位置
// param_d[2] : Pの向きmuki
// param_d[3] : Pの速さ
// param_d[4] : 磁場による曲率(毎フレーム加算)
// param_i[0] : 0=飛行中 1=飛行終了
// param_i[1] : 拡散開始フラグ
// ------------------------------------------------------------
static void ShotWilsonTrack(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 初期化
        pSet->param_d[0] = pSet->x;
        pSet->param_d[1] = pSet->y;
        // 自機狙いに少しランダムを足す
        pSet->param_d[2] = pSet->muki + (GetRand(40) - 20) / 180.0 * DX_PI;
        pSet->param_d[3] = 6.0 + (double)GetRand(200) / 100.0-2; // 6.0-8.0

        // 磁場の曲がり方向をランダムで決定
        double curve = 0.005 + (double)GetRand(10) / 1000.0; // 0.005-0.015
        if (GetRand(1) == 0) curve = -curve;
        pSet->param_d[4] = curve;

        pSet->param_i[0] = 0;
        pSet->param_i[1] = 0;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ---- 不可視粒子Pの更新と航跡生成 ----
    if (pSet->param_i[0] == 0) {
        // Pを進める
        pSet->param_d[0] += pSet->param_d[3] * cos(pSet->param_d[2]);
        pSet->param_d[1] += pSet->param_d[3] * sin(pSet->param_d[2]);
        pSet->param_d[2] += pSet->param_d[4]; // 磁場でカーブ

        // 2フレームに1回、現在位置に凝結核(小玉)を残留させる
        if (pSet->count % 2 == 0) {
            sEnemyShot* pShot = new sEnemyShot;
            // 霧箱らしいザラつき
            pShot->x = pSet->param_d[0] + (double)(GetRand(4) - 2);
            pShot->y = pSet->param_d[1] + (double)(GetRand(4) - 2);
            pShot->muki = 0;
            pShot->speed = 0; // その場に残る
            pShot->kind = img_enemyShotSmallBall[6]; // 白小玉

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // δ線分岐：70Fと110Fで垂直に大玉を出す
        if (pSet->count == 70 || pSet->count == 90) {
            sEnemyShot* pBranch = new sEnemyShot;
            pBranch->x = pSet->param_d[0];
            pBranch->y = pSet->param_d[1];
            // 90度垂直 + 少し散らす
            double base = pSet->param_d[2] + DX_PI / 2.0;
            if (pSet->count == 90) base += DX_PI; // 反対側
            pBranch->muki = base + (GetRand(20) - 10) / 180.0 * DX_PI;
            pBranch->speed = 1.5 + (double)GetRand(50) / 100.0;
            int col = (pSet->count == 70) ? 4 : 0; // 青と赤
            pBranch->kind = img_enemyShotMediumBall[col];
            pBranch->margin = 240;

            pBranch->prev = pSet->pEnemyShotHead->prev;
            pBranch->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pBranch;
            pSet->pEnemyShotHead->prev = pBranch;

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 画面外に出たら生成終了
        //if (pSet->param_d[0] < -60 || pSet->param_d[0] > 540 ||
        //    pSet->param_d[1] < -60 || pSet->param_d[1] > 540) {
        //    pSet->param_i[0] = 1;
        //}
    }

    // ---- 180F後、航跡が膨張・落下する(霧の成長) ----
    if (pSet->count > 180 && pSet->param_i[1] == 0) {
        pSet->param_i[1] = 1; // 拡散は一度だけ発動
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            // 小玉の航跡だけを拡散させる。大玉(分岐)はそのまま
            if (p->speed < 0.01) {
                p->muki = (double)GetRand(360) / 180.0 * DX_PI;
                p->speed = 0.3 + (double)GetRand(50) / 100.0;
            }
            p = p->next;
        }
    }

    // 全弾の移動
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
// 敵本体 - 指定通り EnemyPat_CloudChamber_MetaAI という名前にする
// ------------------------------------------------------------
void EnemyPat_CloudChamber_MetaAI()
{
    static int muki;
    static int fogKind;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        fogKind = 0;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 1. 霧を常時敷く：40F毎
    if (count % 40 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFog;
        pSet->x = 240.0;
        pSet->y = 240.0;
        pSet->muki = 0;
        pSet->kind = fogKind++;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 2. Wilson Trackを130F毎に生成
    if (count % 130 == 20) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotWilsonTrack;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        // 自機狙いをベースに
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
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