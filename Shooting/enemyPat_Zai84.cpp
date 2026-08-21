// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 外側の歯車（固定枠）
// ============================================================
static void ShotOuterGear(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;

    if (pSet->count == 0) {
        // 60個の弾で外側の歯車を構成する (30度ごとに大玉で「歯」を表現)
        for (int i = 0; i < 60; i++) {
            pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->speed = 0.0;
            pShot->muki = 0.0;

            // 弾自身の相対角度と半径を記憶
            pShot->param_d[0] = (i / 60.0) * DX_PI * 2.0;
            pShot->param_d[1] = 120.0 * 2; // 外歯車の半径 R

            // 5個に1個を「歯」として大玉にする
            if (i % 5 == 0) {
                pShot->kind = img_enemyShotLargeBall[6]; // 白の大玉
                pShot->param_i[0] = 1;
            }
            else {
                pShot->kind = img_enemyShotSmallBall[3]; // シアンの小玉
                pShot->param_i[0] = 0;
            }

            // 双方向リストへの追加
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        pSet->param_d[0] = 0.0; // 歯車全体の回転角度
    }
    if (pSet->count < 180) {
        // 歯車をゆっくり回転させる
        pSet->param_d[0] += 0.01;

        // 全弾の座標を再計算して歯車の形を維持
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            double angle = pSet->param_d[0] + pShot->param_d[0];
            double r = pShot->param_d[1];
            pShot->x = pSet->x + r * cos(angle);
            pShot->y = pSet->y + r * sin(angle);
            pShot = pShot->next;
        }
    }
    else if (pSet->count == 180) {
        // 一定時間経過後、歯車を構成していた弾を外側へ解放（放射状に飛ばす）
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->speed = 2.0;
            pShot->muki = atan2(pShot->y - pSet->y, pShot->x - pSet->x) + DX_PI;
            pShot = pShot->next;
        }
    }
    else {
        // 解放後の弾の移動（画面外に出たらメインルーチンで消去される）
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
}

// ============================================================
// 内側の歯車（公転・自転しながらスピログラフ弾を発射）
// ============================================================
static void ShotInnerGear(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;

    if (pSet->count == 0) {
        // 45個の弾で内側の歯車を構成する (40度ごとに大玉で「歯」を表現)
        for (int i = 0; i < 45; i++) {
            pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->speed = 0.0;
            pShot->muki = 0.0;

            pShot->param_d[0] = (i / 45.0) * DX_PI * 2.0; // 相対角度
            pShot->param_d[1] = 45.0 * 2; // 内歯車の半径 r

            if (i % 5 == 0) {
                pShot->kind = img_enemyShotLargeBall[8]; // 橙の大玉
                pShot->param_i[0] = 1; // 歯フラグ
            }
            else {
                pShot->kind = img_enemyShotSmallBall[5]; // マゼンタの小玉
                pShot->param_i[0] = 0;
            }

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // スピログラフ計算用パラメータ
        pSet->param_d[2] = GetRand(1000) * 0.025; // 時間 t

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (pSet->count < 180) {
        double R = 120.0 * 2; // 外歯車の半径（外側関数と共通）
        double r = 45.0 * 2;  // 内歯車の半径

        // 時間を進める
        pSet->param_d[2] += 0.025;
        double t = pSet->param_d[2];

        // 内歯車の中心座標（公転軌道）
        double cx = pSet->x + (R - r) * cos(t);
        double cy = pSet->y + (R - r) * sin(t);

        // 内歯車の自転角度（噛み合うように逆回転）
        double rot = -(R - r) / r * t;

        // 全弾の座標を再計算
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 2) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                double angle = rot + pShot->param_d[0];
                double lr = pShot->param_d[1];
                pShot->x = cx + lr * cos(angle);
                pShot->y = cy + lr * sin(angle);
            }
            pShot = pShot->next;
        }

        // 3フレームに1回、歯の先端からスピログラフ弾を発射
        if (pSet->count % 3 == 0) {
            double dt = 0.05; // 接線の傾きを求めるための微小時間

            pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                if (pShot->param_i[0] == 1) { // 歯の先端でのみ発射
                    double baseAngle = pShot->param_d[0];

                    // 現在の座標
                    double px = cx + r * cos(rot + baseAngle);
                    double py = cy + r * sin(rot + baseAngle);

                    // 微小時間後の座標を計算して接線方向を取得
                    double t2 = t + dt;
                    double cx2 = pSet->x + (R - r) * cos(t2);
                    double cy2 = pSet->y + (R - r) * sin(t2);
                    double rot2 = -(R - r) / r * t2;
                    double px2 = cx2 + r * cos(rot2 + baseAngle);
                    double py2 = cy2 + r * sin(rot2 + baseAngle);

                    // 接線方向に弾を発射（これがスピログラフの曲線になる）
                    sEnemyShot* pNewShot = new sEnemyShot;
                    pNewShot->x = px;
                    pNewShot->y = py;
                    pNewShot->muki = atan2(py2 - py, px2 - px);
                    pNewShot->speed = 2.5;
                    pNewShot->kind = img_enemyShotMediumBall[1]; // 黄色の中玉
                    pNewShot->param_i[0] = 2;

                    pNewShot->prev = pSet->pEnemyShotHead->prev;
                    pNewShot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pNewShot;
                    pSet->pEnemyShotHead->prev = pNewShot;
                }
                pShot = pShot->next;
            }
        }
    }
    else if (pSet->count == 180) {
        double R = 120.0 * 2; // 外歯車の半径（外側関数と共通）
        double r = 45.0 * 2;  // 内歯車の半径

        // 時間を進める
        pSet->param_d[2] += 0.025;
        double t = pSet->param_d[2];

        // 内歯車の中心座標（公転軌道）
        double cx = pSet->x + (R - r) * cos(t);
        double cy = pSet->y + (R - r) * sin(t);

        // 外歯車に合わせて内歯車も解放
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 2) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                pShot->speed = 2.0;
                pShot->muki = atan2(pShot->y - cy, pShot->x - cx);
            }
            pShot = pShot->next;
        }
    }
    else {
        // 解放後の弾の移動
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Spirograph_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 敵本体はゆっくり左右に移動
        enemy.x += 0.98 * (count % 240 < 120 ? 1.0 : -1.0);
    }

    // 300フレームごとにスピログラフ歯車を生成
    if ((count - 1) % 420 == 0) {
        double gearCenterX = 240.0;
        double gearCenterY = 240.0; // 画面中央よりやや上

        // --- 外側の歯車セットを生成 ---
        sEnemyShotSet* pSetOuter = new sEnemyShotSet;
        pSetOuter->count = 0;
        pSetOuter->patternFunc = ShotOuterGear;
        pSetOuter->x = gearCenterX;
        pSetOuter->y = gearCenterY;

        pSetOuter->pEnemyShotHead = new sEnemyShot;
        pSetOuter->pEnemyShotHead->prev = pSetOuter->pEnemyShotHead;
        pSetOuter->pEnemyShotHead->next = pSetOuter->pEnemyShotHead;

        pSetOuter->prev = enemyShotSetHead.prev;
        pSetOuter->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetOuter;
        enemyShotSetHead.prev = pSetOuter;

        // --- 内側の歯車セットを生成 ---
        sEnemyShotSet* pSetInner = new sEnemyShotSet;
        pSetInner->count = 0;
        pSetInner->patternFunc = ShotInnerGear;
        pSetInner->x = gearCenterX;
        pSetInner->y = gearCenterY;

        pSetInner->pEnemyShotHead = new sEnemyShot;
        pSetInner->pEnemyShotHead->prev = pSetInner->pEnemyShotHead;
        pSetInner->pEnemyShotHead->next = pSetInner->pEnemyShotHead;

        pSetInner->prev = enemyShotSetHead.prev;
        pSetInner->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetInner;
        enemyShotSetHead.prev = pSetInner;
    }
}