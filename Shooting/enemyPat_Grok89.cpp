// enemyPat_Tmp.cpp
// ジャンプスケア弾幕「凝視からの咆哮」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾の種類（param_i[0]）
// 0: 目の輪郭（小玉）
// 1: 瞳（中玉）
// 2: 口の輪郭（菱形弾）
// 3: 咆哮弾（小玉・中玉混合）

// ---------------------------------------------------------------
// 弾幕パターン本体
// ---------------------------------------------------------------
static void ShotGazeRoar(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int c = pEnemyShotSet->count;

    // ---- 初期化（1回だけ） ----
    if (c == 0) {
        // 予告音（チャージ）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 目と口の基準位置を param に保存
        // param_d[0],[1] : 左目中心
        // param_d[2],[3] : 右目中心
        // param_d[4],[5] : 口中心
        pEnemyShotSet->param_d[0] = 180.0;
        pEnemyShotSet->param_d[1] = 110.0;
        pEnemyShotSet->param_d[2] = 300.0;
        pEnemyShotSet->param_d[3] = 110.0;
        pEnemyShotSet->param_d[4] = 240.0;
        pEnemyShotSet->param_d[5] = 175.0;
    }

    // ============================================================
    // フェーズ1: 目の輪郭をゆっくり形成（0〜90F）
    // ============================================================
    if (c >= 0 && c < 90) {
        // 2フレームに1発ずつ左右の目に弾を追加
        if (c % 2 == 0) {
            int idx = c / 2;               // 0〜44
            int eye = (idx % 2);           // 0:左 1:右
            int num = idx / 2;             // 輪郭上の何番目か
            if (num < 20) {                // 1目あたり20発
                double cx = pEnemyShotSet->param_d[eye * 2];
                double cy = pEnemyShotSet->param_d[eye * 2 + 1];
                double rad = 38.0;
                double ang = num * (DX_TWO_PI / 20.0) - DX_PI * 0.5;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx + rad * cos(ang);
                pEnemyShot->y = cy + rad * sin(ang);
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青
                pEnemyShot->param_i[0] = 0;                   // 目輪郭
                pEnemyShot->param_d[0] = cx;                  // 所属する目の中心を記憶
                pEnemyShot->param_d[1] = cy;
                pEnemyShot->param_d[2] = ang;                 // 初期角度

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ============================================================
    // フェーズ2: 瞳と口を形成（90〜150F）
    // ============================================================
    if (c == 90) {
        // 左瞳
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->param_d[0];
        pEnemyShot->y = pEnemyShotSet->param_d[1];
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白
        pEnemyShot->param_i[0] = 1;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 右瞳
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->param_d[2];
        pEnemyShot->y = pEnemyShotSet->param_d[3];
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白
        pEnemyShot->param_i[0] = 1;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 口の輪郭を徐々に追加（100〜140F）
    if (c >= 100 && c < 140 && (c % 2 == 0)) {
        int num = (c - 100) / 2; // 0〜19
        if (num < 18) {
            double cx = pEnemyShotSet->param_d[4];
            double cy = pEnemyShotSet->param_d[5];
            // 下に開いた弧
            double ang = -DX_PI / 2.0 - 0.9 + num * (1.8 / 17.0);
            double rad = 55.0;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + rad * cos(ang);
            pEnemyShot->y = cy + rad * sin(ang) * 0.55; // 少し潰した弧
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotDiamond[3]; // シアン
            pEnemyShot->param_i[0] = 2;                 // 口
            pEnemyShot->param_d[0] = ang;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ============================================================
    // フェーズ3: 凝視（150〜240F）— わずかに揺れる
    // ============================================================
    // 移動処理の中で行う

    // ============================================================
    // フェーズ4: ジャンプスケア瞬間（240F）
    // ============================================================
    if (c == 240) {
        // 大音量の咆哮音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 既存の目・口弾を外側へ爆発させる
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
                // 目の中心から外側へ
                double dx = pShot->x - pShot->param_d[0];
                double dy = pShot->y - pShot->param_d[1];
                pShot->muki = atan2(dy, dx);
                pShot->speed = 4.5 + GetRand(20) / 10.0;
                // 色を赤系に変える
                if (pShot->param_i[0] == 0)
                    pShot->kind = img_enemyShotSmallBall[0]; // 赤
                else
                    pShot->kind = img_enemyShotMediumBall[0];
            }
            else if (pShot->param_i[0] == 2) {
                // 口は下方向へ散らす
                pShot->muki = DX_PI * 0.5 + (GetRand(40) - 20) / 180.0 * DX_PI;
                pShot->speed = 3.0 + GetRand(15) / 10.0;
                pShot->kind = img_enemyShotDiamond[0]; // 赤
            }
            pShot = pShot->next;
        }

        // 口の中心から高密度扇状弾幕を一斉発射
        double mouthX = pEnemyShotSet->param_d[4];
        double mouthY = pEnemyShotSet->param_d[5];
        // 基準角度：真下
        double baseMuki = DX_PI * 0.5;

        // 扇の弾数（中央ほど密に）
        for (int i = 0; i < 72; i++) {
            pEnemyShot = new sEnemyShot;
            // -0.85〜+0.85 rad 程度の扇
            double offset = (i - 35.5) * 0.024;
            pEnemyShot->x = mouthX + (GetRand(12) - 6);
            pEnemyShot->y = mouthY + (GetRand(8) - 4);
            pEnemyShot->muki = baseMuki + offset;
            // 速度にばらつきを持たせて厚みを出す
            pEnemyShot->speed = 5.2 + GetRand(35) / 10.0;
            pEnemyShot->param_i[0] = 3; // 咆哮弾

            // 種類と色を混ぜる（赤・橙・マゼンタ）
            int col = (i % 3 == 0) ? 0 : ((i % 3 == 1) ? 8 : 5);
            if (i % 5 == 0)
                pEnemyShot->kind = img_enemyShotMediumBall[col];
            else if (i % 7 == 0)
                pEnemyShot->kind = img_enemyShotScale[col];
            else
                pEnemyShot->kind = img_enemyShotSmallBall[col];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 追加でプレイヤー狙いの弾を少し混ぜてプレッシャーを高める
        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = mouthX + (GetRand(30) - 15);
            pEnemyShot->y = mouthY;
            double aim = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->muki = aim + (GetRand(30) - 15) / 180.0 * DX_PI;
            pEnemyShot->speed = 6.0 + GetRand(20) / 10.0;
            pEnemyShot->kind = img_enemyShotMediumOval[0]; // 赤の中楕円
            pEnemyShot->param_i[0] = 3;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ============================================================
    // 全弾の移動・微調整
    // ============================================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 凝視フェーズ中の目の揺れ（150〜239F）
        if (c >= 150 && c < 240) {
            if (pShot->param_i[0] == 0) {
                // 輪郭弾をわずかに回転させる
                double cx = pShot->param_d[0];
                double cy = pShot->param_d[1];
                double ang = pShot->param_d[2] + sin(c * 0.07) * 0.08;
                double rad = 38.0 + sin(c * 0.11) * 1.5;
                pShot->x = cx + rad * cos(ang);
                pShot->y = cy + rad * sin(ang);
            }
            else if (pShot->param_i[0] == 1) {
                // 瞳をわずかに左右に揺らす
                double baseX = (pShot->x < 240.0) ? pEnemyShotSet->param_d[0] : pEnemyShotSet->param_d[2];
                pShot->x = baseX + sin(c * 0.09) * 3.0;
            }
            else if (pShot->param_i[0] == 2) {
                // 口をゆっくり開閉
                double open = 0.55 + sin(c * 0.06) * 0.12;
                double cx = pEnemyShotSet->param_d[4];
                double cy = pEnemyShotSet->param_d[5];
                double ang = pShot->param_d[0];
                double rad = 55.0;
                pShot->x = cx + rad * cos(ang);
                pShot->y = cy + rad * sin(ang) * open;
            }
        }

        // 通常移動（speedが0なら動かない）
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------
// 敵本体パターン
// ---------------------------------------------------------------
void EnemyPat_JumpScare_Grok()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    else {
        // わずかに左右に揺れる（目立たない程度）
        enemy.x = 240.0 + sin(count * 0.025) * 18.0;
    }

    // 最初の1回だけ弾幕セットを生成
    if (count % 300 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGazeRoar;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI * 0.5; // 下向き
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