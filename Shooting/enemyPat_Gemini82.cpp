// enemyPat_futon.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：風雲！布団が吹っ飛んだ
static void ShotFuton(sEnemyShotSet* pSet)
{
    // ==========================================
    // Phase 1: 布団の形成と降下 (count: 0 ~ 134)
    // ==========================================
    if (pSet->count == 0) {
        // 重い出現音で布団の質量感を表現
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->param_i[0] = 0; // 状態フラグ: 0=降下中(原形), 1=崩壊・吹っ飛び中

        // 突風が吹く方向をランダムに決定 (1: 左から右, -1: 右から左)
        pSet->param_i[1] = (GetRand(1) == 0) ? 1 : -1;

        // 縦6行 × 横10列 の長方形「布団」を形成
        const int R = 6 + 2;
        const int C = 10 + 3;
        for (int row = 0; row < R; row++) {
            for (int col = 0; col < C; col++) {
                sEnemyShot* pShot = new sEnemyShot;

                // 四隅は大弾(赤)、それ以外は中弾(白)
                bool isCorner = (row == 0 && col == 0) || (row == 0 && col == C - 1) ||
                    (row == R - 1 && col == 0) || (row == R - 1 && col == C - 1);

                if (isCorner) {
                    pShot->param_i[0] = 1; // 1: 布団の角(大弾)
                    pShot->kind = img_enemyShotLargeBall[0]; // 赤
                }
                else {
                    pShot->param_i[0] = 0; // 0: 布団の面(中弾)
                    pShot->kind = img_enemyShotMediumBall[6]; // 白
                }

                pShot->param_i[1] = 0; // 大弾の反射状態管理用

                // 中心からの相対座標(オフセット)を記憶しておく
                pShot->param_d[0] = (col - 4.5) * 16.0; // offsetX
                pShot->param_d[1] = (row - 2.5) * 16.0; // offsetY

                pShot->x = pSet->x + pShot->param_d[0];
                pShot->y = pSet->y + pShot->param_d[1];

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    if (pSet->param_i[0] == 0) {
        // 布団の基準位置をゆっくり降下＆サイン波で揺らす
        pSet->y += 0.4;
        pSet->x = enemy.x + sin(pSet->count * 0.03) * 50.0;

        // ==========================================
        // Phase 2: 突風の到来 (count: 120)
        // ==========================================
        if (pSet->count == 120) {
            // 予告音としてチャージ音を鳴らす
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            int windDir = pSet->param_i[1];

            // 針弾(短レーザー)を画面端から水平に何条も射出
            for (int i = 0; i < 15; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->param_i[0] = 2; // 2: 風(針弾)
                pShot->kind = img_enemyShotLaser[3]; // シアン

                pShot->x = (windDir == 1) ? 0.0 : 480.0;
                pShot->y = pSet->y + GetRand(150) - 75; // 布団の高さめがけて
                pShot->muki = (windDir == 1) ? 0.0 : DX_PI;
                pShot->speed = (140 + GetRand(40)) / 10.0; // 14.0 ~ 18.0 の高速

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }

        // ==========================================
        // Phase 3: 崩壊・吹っ飛び (count: 135)
        // ==========================================
        // 風が画面中央の布団に直撃したタイミングで一気に崩す
        if (pSet->count == 135) {
            pSet->param_i[0] = 1; // 崩壊状態へ移行

            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            double windVecX = (pSet->param_i[1] == 1) ? 3.0 : -3.0; // 風圧による横方向の慣性

            sEnemyShot* pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                if (pShot->param_i[0] == 0) {
                    // 中弾: 外側に広がる速度 + 風の慣性ベクトルを合成
                    double angle = atan2(pShot->param_d[1], pShot->param_d[0]);
                    double spd = (30 + GetRand(30)) / 10.0;
                    double vx = spd * cos(angle) + windVecX;
                    double vy = spd * sin(angle);

                    pShot->speed = sqrt(vx * vx + vy * vy);
                    pShot->muki = atan2(vy, vx);

                    // 螺旋軌道を描くための角速度 (ランダムに左回り・右回り)
                    pShot->param_d[2] = ((GetRand(1) == 0) ? 1 : -1) * ((15 + GetRand(20)) / 1000.0);
                }
                else if (pShot->param_i[0] == 1) {
                    // 大弾: 四隅へ向けて高速で弾き飛ばす
                    double angle = atan2(pShot->param_d[1], pShot->param_d[0]);
                    double vx = 6.0 * cos(angle) + windVecX;
                    double vy = 6.0 * sin(angle);

                    pShot->speed = sqrt(vx * vx + vy * vy);
                    pShot->muki = atan2(vy, vx);
                }
                pShot = pShot->next;
            }
        }
    }

    // ==========================================
    // 各弾の個別軌道更新
    // ==========================================
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // [中弾 (布団の布面)]
            if (pSet->param_i[0] == 0) {
                // 原形：ふわふわと風に揺れる表現を加味して基準点に追従
                pShot->x = pSet->x + pShot->param_d[0];
                pShot->y = pSet->y + pShot->param_d[1] + sin((pSet->count + pShot->param_d[0]) * 0.05) * 5.0;
            }
            else {
                // 崩壊後：螺旋を描きながら吹き飛ぶ
                pShot->muki += pShot->param_d[2];
                pShot->speed += 0.03; // 徐々に加速して画面外へ散る

                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else if (pShot->param_i[0] == 1) {
            // [大弾 (布団の四隅の重み)]
            if (pSet->param_i[0] == 0) {
                // 原形：中弾と同様に追従
                pShot->x = pSet->x + pShot->param_d[0];
                pShot->y = pSet->y + pShot->param_d[1] + sin((pSet->count + pShot->param_d[0]) * 0.05) * 5.0;
            }
            else {
                // 崩壊後
                if (pShot->param_i[1] == 0) {
                    // 左右の壁に当たったら1度だけバウンドさせる
                    bool reflected = false;
                    if (pShot->x < 0) { pShot->muki = DX_PI - pShot->muki; pShot->x = 0; reflected = true; }
                    else if (pShot->x > 480) { pShot->muki = DX_PI - pShot->muki; pShot->x = 480; reflected = true; }

                    if (reflected) pShot->speed *= 0.7; // 壁反射で勢いを殺す

                    // 時間差(count==220)で角の重みが自機に向かって飛んでくる嫌らしい仕掛け
                    if (pSet->count == 220) {
                        pShot->param_i[1] = 1; // 変化済みフラグ
                        pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                        pShot->speed = 4.0;

                        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                    }
                }
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else if (pShot->param_i[0] == 2) {
            // [風 (針弾)]
            // 単純な等速直線運動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_FutonFlewAway_Gemini()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 敵はゆっくり左右に揺れる
        enemy.x += 0.6 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 260フレーム周期で布団攻撃を射出
    if (count % 260 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFuton;
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
}