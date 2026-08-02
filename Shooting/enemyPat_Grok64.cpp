// enemyPat_Tmp.cpp
// 三色団子モチーフ弾幕「三色串刺し弾幕」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 使える効果音: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge
// 弾の種類: 小玉, 中玉, 大玉, 銃弾, 鱗弾, 菱形弾, 中楕円弾, 短レーザー
// 弾の色: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
// 三色団子用: ピンク=マゼンタ(5), 白(6), 緑(2)

static const int DANGO_COLOR[3] = { 5, 6, 2 }; // 上からピンク・白・緑

// 弾幕パターン本体
static void ShotSanshokuKushizashi(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    //--------------------------------------------------
    // 出現直後：3つの団子（大玉）を縦に配置（速度0で固定）
    //--------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + (i - 1) * 28.0;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLargeBall[DANGO_COLOR[i]];
            pEnemyShot->param_i[0] = 1;          // 種類：団子本体
            pEnemyShot->param_i[1] = i;          // 0:ピンク 1:白 2:緑
            pEnemyShot->param_d[0] = (i - 1) * 28.0; // 初期相対Y
            pEnemyShot->param_d[1] = 0.0;        // 回転角度用

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    //--------------------------------------------------
    // 串展開（左右にレーザー状の串が伸びる）
    //--------------------------------------------------
    if (pEnemyShotSet->count == 70) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int dir = -1; dir <= 1; dir += 2) {          // 左(-1)と右(+1)
            for (int k = 0; k < 3; k++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y + (k - 1) * 28.0;
                pEnemyShot->muki = (dir > 0) ? 0.0 : DX_PI;
                pEnemyShot->speed = 2.8;
                pEnemyShot->kind = img_enemyShotLaser[DANGO_COLOR[k]];
                pEnemyShot->param_i[0] = 2;               // 種類：串
                pEnemyShot->param_i[1] = k;
                pEnemyShot->margin = 40;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    //--------------------------------------------------
    // 団子回転弾幕（各団子から放射状に弾を発射）
    //--------------------------------------------------
    if (pEnemyShotSet->count >= 110 && pEnemyShotSet->count < 280) {
        if (pEnemyShotSet->count % 18 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            // 現在の団子位置を計算してそこから放射
            double baseAngle = pEnemyShotSet->count * 0.035;
            for (int i = 0; i < 3; i++) {
                double cx = pEnemyShotSet->x + 18.0 * cos(baseAngle + i * DX_PI * 2.0 / 3.0);
                double cy = pEnemyShotSet->y + 18.0 * sin(baseAngle + i * DX_PI * 2.0 / 3.0);

                int num = (i == 1) ? 12 : (i == 0 ? 8 : 6); // 白は多め、ピンク8、緑6
                for (int j = 0; j < num; j++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = cx;
                    pEnemyShot->y = cy;
                    pEnemyShot->muki = baseAngle + (DX_PI * 2.0 * j) / num + i * 0.4;
                    pEnemyShot->speed = 1.6 + (i == 1 ? 0.4 : 0.0);

                    // 色に合わせた弾種
                    if (i == 0) {          // ピンク：中玉＋弱い誘導風に少し曲げる準備
                        pEnemyShot->kind = img_enemyShotMediumBall[DANGO_COLOR[0]];
                        pEnemyShot->param_i[0] = 3; // 誘導気味弾
                        pEnemyShot->param_d[0] = 0.015; // 曲がり量
                    }
                    else if (i == 1) {     // 白：小玉で直進加速
                        pEnemyShot->kind = img_enemyShotSmallBall[DANGO_COLOR[1]];
                        pEnemyShot->param_i[0] = 4; // 加速弾
                        pEnemyShot->param_d[0] = 0.04; // 加速度
                    }
                    else {                 // 緑：鱗弾で弧を描く
                        pEnemyShot->kind = img_enemyShotScale[DANGO_COLOR[2]];
                        pEnemyShot->param_i[0] = 5; // 弧弾
                        pEnemyShot->param_d[0] = (j % 2 == 0) ? 0.007 : -0.007;
                    }

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
    }

    //--------------------------------------------------
    // 最終段階：串破壊＋三色大散開
    //--------------------------------------------------
    if (pEnemyShotSet->count == 300) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 団子を爆発させる（速度を与えて外側へ）
        // および大量の三色小玉を全方位に
        for (int n = 0; n < 48; n++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(40) - 20);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(40) - 20);
            pEnemyShot->muki = (GetRand(360) / 180.0) * DX_PI;
            pEnemyShot->speed = 1.2 + GetRand(180) / 100.0;
            int col = DANGO_COLOR[GetRand(2)];
            pEnemyShot->kind = img_enemyShotSmallBall[col];
            pEnemyShot->param_i[0] = 6; // 最終散開弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 串の破片風に菱形弾を斜めに
        for (int n = 0; n < 16; n++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (n % 2 == 0 ? 0.6 : -0.6) + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.5 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotDiamond[DANGO_COLOR[n % 3]];
            pEnemyShot->param_i[0] = 6;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    //--------------------------------------------------
    // 全弾の移動・挙動更新
    //--------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int type = pShot->param_i[0];

        if (type == 1) {
            if (pEnemyShotSet->count < 110) {
                pShot->x = pEnemyShotSet->x;
            }
            else if (pEnemyShotSet->count < 300) {
                // 団子本体：中心周りをゆっくり公転
                double ang = pEnemyShotSet->count * 0.025 + pShot->param_i[1] * (DX_PI * 2.0 / 3.0);
                pShot->x = pEnemyShotSet->x + 16.0 * cos(ang);
                pShot->y = pEnemyShotSet->y + 16.0 * sin(ang);
                // speed=0のままなので位置を直接制御
            }
            else {
                pShot->margin = -9999;
            }
        }
        else if (type == 2) {
            // 串：そのまま直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (type == 3) {
            // ピンク誘導気味：徐々にプレイヤー方向へ曲がる
            double target = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = target - pShot->muki;
            while (diff > DX_PI) diff -= DX_PI * 2.0;
            while (diff < -DX_PI) diff += DX_PI * 2.0;
            pShot->muki += diff * pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (type == 4) {
            // 白加速弾
            pShot->speed += pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (type == 5) {
            // 緑弧弾：向きを少しずつ変化
            pShot->muki += pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 通常・最終弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_TricolorDango_Grok()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 170.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 敵本体はゆっくり左右に往復
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 120.0 || enemy.x > 360.0) muki *= -1;

        // 弾幕セットの基準位置を敵に追従させる
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == ShotSanshokuKushizashi) {
                pSet->x = enemy.x;
                pSet->y = enemy.y + 20.0;
                break;
            }
            pSet = pSet->next;
        }
    }

    if (count % 360 == 1) {
        // メインの弾幕セットを1つだけ生成（以降はこのセットが全フェーズを管理）
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSanshokuKushizashi;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
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