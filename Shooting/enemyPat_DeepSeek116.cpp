// enemyPat_Tmp.cpp
//
// 弾幕：ウィルソンの霧箱 -Wilson Cloud Chamber-
//
//  ボスを霧箱に見立て、放射線の飛跡が一時的な弾の列として現れる。
//  専用素材は使わず、既存の敵弾を役割分担させて表現する。
//
//    小玉(白)     : 霧の粒／飛跡の点
//    大玉(白)     : α粒子
//    小玉(シアン) : β粒子
//    短レーザー(白): 宇宙線
//
//  count, pEnemyShotSet->count, pEnemyShot->count のインクリメント、
//  画面外の弾の消去はメインルーチンが行う。
//
//  GetRand(x) は 0 以上 x 以下の x+1 種類の整数を返す点に注意。

#include "gv.h"
#include "DxLib.h"
#include <math.h>

// ------------------------------------------------------------
// 共通ヘルパー
// ------------------------------------------------------------

// 弾をセットのリンクリスト末尾に追加
static void AddEnemyShot(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// 飛跡弾（霧の粒）を生成
static void SpawnTrail(sEnemyShotSet* pEnemyShotSet,
    double x, double y, int kind, int life)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = 0.0;
    p->speed = 0.0;
    p->kind = kind;
    p->param_i[0] = 0;    // 0 = 飛跡
    p->param_i[1] = life; // 寿命フレーム
    AddEnemyShot(pEnemyShotSet, p);
}

// 放射線の粒子本体を生成
static void SpawnParticle(sEnemyShotSet* pEnemyShotSet,
    double speed, int kind, int life)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = pEnemyShotSet->x;
    p->y = pEnemyShotSet->y;
    p->muki = pEnemyShotSet->muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = 1;    // 1 = 粒子本体
    p->param_i[1] = life; // 寿命フレーム
    AddEnemyShot(pEnemyShotSet, p);
}

// 寿命切れの弾を画面外へ飛ばしてメインルーチンに消させる
static void ExpireIfDead(sEnemyShot* pShot)
{
    if (pShot->count > pShot->param_i[1]) {
        pShot->x = -1000.0;
        pShot->y = -1000.0;
    }
}

// 弾幕セットを新規作成
static void CreateShotSet(sEnemyShotSet::PatternFunc func,
    double x, double y, double muki, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ------------------------------------------------------------
// α線トラック：太く短い直線飛跡
//   大玉(白)が低速直進し、進行方向に垂直に小玉(白)を3個並べて残す
// ------------------------------------------------------------
static void ShotAlphaTrack(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        SpawnParticle(pEnemyShotSet, 1.1+1, img_enemyShotLargeBall[6], 90);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 粒子本体：直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 一定間隔で垂直方向に飛跡を3個並べる（太い飛跡）
            if (pEnemyShotSet->count % 4 == 0) {
                double px = -sin(pShot->muki);
                double py = cos(pShot->muki);
                for (int i = -1; i <= 1; i++) {
                    SpawnTrail(pEnemyShotSet,
                        pShot->x + px * (double)i * 5.0,
                        pShot->y + py * (double)i * 5.0,
                        img_enemyShotSmallBall[6], 45);
                }
            }
        }
        ExpireIfDead(pShot);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// β線トラック：細く波打つ飛跡
//   小玉(シアン)が高速直進し、sin波でズレた位置に小玉(白)を落とす
// ------------------------------------------------------------
static void ShotBetaTrack(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        SpawnParticle(pEnemyShotSet, 5.0, img_enemyShotSmallBall[3], 60);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 垂直方向に sin 波でズレた位置に飛跡を残す
            if (pEnemyShotSet->count % 2 == 0) {
                double px = -sin(pShot->muki);
                double py = cos(pShot->muki);
                double wave = sin((double)pShot->count * 0.55) * 14.0;
                SpawnTrail(pEnemyShotSet,
                    pShot->x + px * wave,
                    pShot->y + py * wave,
                    img_enemyShotSmallBall[6], 30);
            }
        }
        ExpireIfDead(pShot);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 宇宙線トラック：画面を貫く長い直線飛跡
//   短レーザー(白)が高速直進し、まばらに小玉(白)を落とす
// ------------------------------------------------------------
static void ShotCosmicTrack(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        SpawnParticle(pEnemyShotSet, 9.0, img_enemyShotLaser[6], 80);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // まばらに飛跡を落とす
            if (pEnemyShotSet->count % 3 == 0) {
                SpawnTrail(pEnemyShotSet,
                    pShot->x, pShot->y,
                    img_enemyShotSmallBall[6], 20);
            }
        }
        ExpireIfDead(pShot);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_CloudChamber_DeepSeek()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.9 * (double)muki;
        if (count % 180 == 90) muki *= -1;
        if (enemy.x < 60.0) enemy.x = 60.0;
        if (enemy.x > 420.0) enemy.x = 420.0;
    }

    // HPに応じて放射線の発生間隔を短くする（霧箱が活発化）
    int hpRate = (enemy.maxHp > 0) ? (enemy.hp * 100 / enemy.maxHp) : 100;
    int interval = 45-10;
    if (hpRate < 75) interval = 34-10;
    if (hpRate < 50) interval = 24-10;

    // 放射線トラックを1本ずつ発生させる
    if (count % interval == 1) {
        int kind = shot_count % 3;
        double sx = enemy.x;
        double sy = enemy.y + 10.0;

        if (kind == 0) {
            // α線：プレイヤー方向へややばらして発射
            double m = atan2(player.y - sy, player.x - sx)
                + (double)(GetRand(40) - 20) / 180.0 * DX_PI;
            CreateShotSet(ShotAlphaTrack, sx, sy, m, shot_count);
        }
        else if (kind == 1) {
            // β線：ランダム方向へ
            double m = (double)GetRand(360) / 180.0 * DX_PI;
            CreateShotSet(ShotBetaTrack, sx, sy, m, shot_count);
        }
        else {
            // 宇宙線：画面を横断する方向へ
            double m = (double)GetRand(360) / 180.0 * DX_PI;
            CreateShotSet(ShotCosmicTrack, sx, sy, m, shot_count);
        }
        shot_count++;
    }

    // 最終盤：全方位β線のスターバースト＋照準α線
    if (hpRate < 50 && count % 180 == 0) {
        double sx = enemy.x;
        double sy = enemy.y + 10.0;

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // β線を全方位に一斉放射（波打つ飛跡のスターバースト）
        for (int i = 0; i < 12; i++) {
            double m = (double)i * 30.0 / 180.0 * DX_PI;
            CreateShotSet(ShotBetaTrack, sx, sy, m, shot_count++);
        }

        // 消えていく飛跡の隙間を縫うα線（プレイヤー照準）
        double m = atan2(player.y - sy, player.x - sx);
        CreateShotSet(ShotAlphaTrack, sx, sy, m, shot_count++);
    }
}