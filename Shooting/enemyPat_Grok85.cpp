// enemyPat_Tmp.cpp
// 無限螺旋崩壊（Infinite Spiral Collapse） - TAS前提超高難易度弾幕
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン関数群
// ============================================================

// 1. 多重加速螺旋（メイン弾幕）
//    8方向螺旋が加速しながら広がり、回転方向が周期的に反転する
static void ShotInfiniteSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射タイミング（密度を非常に高くする）
    if (pEnemyShotSet->count % 3 == 0) {
        // 効果音（極端な弾幕なので extreme）
        if (pEnemyShotSet->count % 15 == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        // 基本角度（時間とともにゆっくり回転）
        double baseAngle = pEnemyShotSet->param_d[0];
        // 回転方向フラグ（param_i[0] = 1 or -1）
        int rotDir = pEnemyShotSet->param_i[0];

        // 8方向螺旋
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 初期向き
            pEnemyShot->muki = baseAngle + (DX_PI * 2.0 / 8.0) * i;
            // 初期速度（後で加速させる）
            pEnemyShot->speed = 0.9;
            // 小玉（高密度用）白寄り
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
            // 加速用パラメータ
            pEnemyShot->param_d[0] = 0.012;          // 加速度
            pEnemyShot->param_d[1] = (double)rotDir; // 回転方向記憶
            pEnemyShot->param_i[0] = 0;              // 寿命カウント用

            // リスト接続
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 基本角度を少し進める（螺旋のねじれ）
        pEnemyShotSet->param_d[0] += 0.07 * rotDir;
    }

    // 回転方向の反転（約0.5秒ごと = 30フレーム）
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 30 == 0) {
        pEnemyShotSet->param_i[0] *= -1;
    }

    // 弾の移動と加速処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 加速
        pShot->speed += pShot->param_d[0];
        if (pShot->speed > 3.8) pShot->speed = 3.8; // 上限

        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // わずかに外側へ曲がる（螺旋感を強調）
        pShot->muki += 0.004 * pShot->param_d[1];

        pShot = pShot->next;
    }
}

// 2. 位相ずらし二重リング（収縮・膨張）
static void ShotPhaseRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 一定間隔でリングを生成
    if (pEnemyShotSet->count % 45 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double radius = 40.0 + (pEnemyShotSet->count % 90) * 1.2; // 半径変化
        int num = 36; // 密度高め

        for (int i = 0; i < num; i++) {
            double ang = (DX_PI * 2.0 / num) * i + pEnemyShotSet->param_d[0];

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + cos(ang) * radius;
            pEnemyShot->y = pEnemyShotSet->y + sin(ang) * radius;
            // 外側へ放射 or 内側へ収束を切り替える
            if (pEnemyShotSet->param_i[0] == 0) {
                // 外側へ
                pEnemyShot->muki = ang;
                pEnemyShot->speed = 1.4;
            }
            else {
                // 内側へ（収束）
                pEnemyShot->muki = ang + DX_PI;
                pEnemyShot->speed = 1.8;
            }
            // 菱形弾（視認性と密度のバランス）
            pEnemyShot->kind = img_enemyShotDiamond[5]; // マゼンタ
            pEnemyShot->param_d[0] = 0.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 位相をずらす
        pEnemyShotSet->param_d[0] += 0.15;
        // 収縮/膨張モード切替
        if (pEnemyShotSet->count % 90 == 0) {
            pEnemyShotSet->param_i[0] = 1 - pEnemyShotSet->param_i[0];
        }
    }

    // 移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 3. 超高速レーザー掃引（終盤用）
//    画面端から細いレーザーが掃引し、残像弾を残す
static void ShotLaserSweep(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射間隔を短くして圧迫感を出す
    if (pEnemyShotSet->count % 8 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 掃引角度を時間で変化
        double sweepAngle = pEnemyShotSet->param_d[0];

        // レーザー本体（短レーザーを連続配置して擬似的に長く見せる）
        for (int k = 0; k < 7; k++) {
            pEnemyShot = new sEnemyShot;
            // 画面端寄りから開始
            double dist = 30.0 + k * 55.0;
            pEnemyShot->x = pEnemyShotSet->x + cos(sweepAngle) * dist;
            pEnemyShot->y = pEnemyShotSet->y + sin(sweepAngle) * dist;
            pEnemyShot->muki = sweepAngle;
            pEnemyShot->speed = 0.0; // その場に残す（残像風）
            pEnemyShot->kind = img_enemyShotLaser[0]; // 赤レーザー
            pEnemyShot->param_i[0] = 12; // 残存フレーム数
            pEnemyShot->margin = 40.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 掃引角度を進める（斜めも含む）
        pEnemyShotSet->param_d[0] += 0.22;
    }

    // 残存時間管理と移動（ほぼ固定だが微動）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] > 0) {
            pShot->param_i[0]--;
            // わずかに外側へ押し出す
            pShot->x += 0.4 * cos(pShot->muki);
            pShot->y += 0.4 * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 4. 崩壊拡散弾（最終フェーズ）
//    一度内側に収束した後、一気に外側へ爆発的に拡散
static void ShotCollapseBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 予告音
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 収束弾を生成（序盤）
    if (pEnemyShotSet->count < 40 && pEnemyShotSet->count % 2 == 0) {
        for (int i = 0; i < 12; i++) {
            double ang = (DX_PI * 2.0 / 12.0) * i + pEnemyShotSet->count * 0.05;
            pEnemyShot = new sEnemyShot;
            // 外側から生成して内側へ
            pEnemyShot->x = pEnemyShotSet->x + cos(ang) * 220.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(ang) * 220.0;
            pEnemyShot->muki = ang + DX_PI; // 中心向き
            pEnemyShot->speed = 4.5;
            pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤
            pEnemyShot->param_i[0] = 1; // 収束モード

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 崩壊拡散（中盤以降）
    if (pEnemyShotSet->count == 50) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 72; i++) { // 高密度拡散
            double ang = (DX_PI * 2.0 / 72.0) * i;
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 1.2 + (i % 5) * 0.35; // 速度差で複雑に
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
            pEnemyShot->param_d[0] = 0.025; // わずかな加速

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 収束モード：中心に近づいたら消える想定（画面外処理に任せる）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 拡散モード
            pShot->speed += pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_TAS_Grok()
{
    // 敵の初期化と固定配置（中央寄りでパターンを最大限活かす）
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // わずかな左右揺れ（完全固定だと単調になるため）
    enemy.x = 240.0 + sin(count * 0.02) * 18.0;

    // -------------------------------------------------------
    // フェーズ管理（全体の流れ）
    // -------------------------------------------------------

    // Phase 1: 多重加速螺旋（常時）
    if (count == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotInfiniteSpiral;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->param_d[0] = 0.0;   // baseAngle
        pSet->param_i[0] = 1;     // 回転方向
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // Phase 2: 位相リング追加（中盤から）
    if (count == 180) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPhaseRing;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->param_d[0] = 0.0;
        pSet->param_i[0] = 0; // 膨張モードから開始
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // Phase 3: レーザー掃引開始（さらに圧迫）
    if (count == 360) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotLaserSweep;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->param_d[0] = -DX_PI * 0.25; // 初期掃引角
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // Phase 4: 崩壊拡散（終盤の決め手）
    if (count == 520) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotCollapseBurst;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 螺旋の発射位置を敵に追従させる（簡易的に最新の螺旋セットを探すのは省略し、
    // 必要なら追加のセット生成で対応。ここでは初期位置固定で十分圧がかかる）
}