// enemyPat_tmp.cpp
// 弾幕：血の代償（Blood Tithe）
// 被弾時に敵中心から8方向に弾を噴出し、一定距離進むと停止・脈動した後、
// 扇状に弾を破裂させる撃ち返し弾幕。被弾回数に応じて弾の色が変化する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕パターン：血の代償
// ------------------------------------------------------------
static void ShotBloodTithe(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // count のインクリメントはメインルーチンが行う
    if (pEnemyShotSet->count == 0) {
        // 効果音：被弾時の重い衝撃音
        // 使える効果音: sound_enemyShot_light, medium, heavy, extreme, sound_enemyCharge
        if (pEnemyShotSet->param_i[0] % 2 == 1) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        // 第1段：「傷口からの出血」
        // 敵中心から8方向に赤い小玉を噴出
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;

            // 基本方向は pEnemyShotSet->muki（プレイヤー方向）から45°ずつ
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * DX_PI / 4.0;
            pEnemyShot->speed = 3.0;

            // 弾の種類: 小玉(2.5x2.5) ※最初は赤固定
            // 弾の色: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            pEnemyShot->kind = img_enemyShotSmallBall[0];

            // param_i[0]: 状態管理 (0=移動中, 1=停止中, 2=破裂処理済み, 3=破裂後の弾)
            // param_i[1]: 停止開始時の count 値（30フレーム計測用）
            // param_d[0,1]: 起点座標（移動距離計測用）
            pEnemyShot->param_i[0] = 0;
            pEnemyShot->param_i[1] = 0;
            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_i[2] = enemy.hp;

            // 連結リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 各弾の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 削除対応用

        if (pShot->param_i[0] == 0) {
            // 移動中：わずかにプレイヤー方向へ誘導（最大10°以内に収まるよう緩やかに曲がる）
            double targetMuki = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = targetMuki - pShot->muki;
            while (diff > DX_PI)  diff -= DX_PI * 2.0;
            while (diff < -DX_PI) diff += DX_PI * 2.0;
            // 1フレームあたり最大約2°（0.035rad）だけ方向修正
            if (diff > 0.035)  diff = 0.035;
            if (diff < -0.035) diff = -0.035;
            pShot->muki += diff;

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 起点から60ピクセル進んだら「凝固」して停止
            double dx = pShot->x - pShot->param_d[0];
            double dy = pShot->y - pShot->param_d[1];
            if (sqrt(dx * dx + dy * dy) >= 60.0) {
                pShot->param_i[0] = 1;
                pShot->param_i[1] = pShot->count; // 停止開始時の count を記録
                pShot->speed = 0.0;

                // 停止中は中玉に見た目を変更。色は被弾回数に応じて変化
                int hitCount = pEnemyShotSet->param_i[0];
                int color = (hitCount >= 10) ? 7 : (hitCount - 1) % 9;
                pShot->kind = img_enemyShotMediumBall[color];
            }
        }
        else if (pShot->param_i[0] == 1) {
            // 停止中：30フレーム（0.5秒）経過で「代償の開花」
            // pEnemyShot->count のインクリメントはメインルーチンが行う
            if (pShot->count - pShot->param_i[1] >= 30) {
                pShot->param_i[0] = 2;

                // 破裂音
                if (pEnemyShotSet->param_i[0] % 2 == 1) {
                    if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
                }

                // 第3段：扇状の弾幕を放射（各光球から12発、計96発）
                int hitCount = pEnemyShotSet->param_i[0];
                int color = (hitCount >= 10) ? 7 : (hitCount - 1) % 9;

                for (int j = 0; j < 12; j++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    // 30°間隔で360°全方向に放射
                    pNew->muki = j * DX_PI / 6.0;
                    // 弾速は遅め（プレイヤーよりやや遅い）
                    pNew->speed = 2.0;
                    // 破裂後は小玉で統一
                    pNew->kind = img_enemyShotSmallBall[color];
                    pNew->param_i[0] = 3; // 破裂後の弾は直進のみ

                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }

                // 元の停止中の光球を削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        else if (pShot->param_i[0] == 3) {
            // 破裂後の弾：単純直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Counter_Kimi()
{
    static int prevHp;
    static int hitCount;
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        prevHp = enemy.hp;
        hitCount = 0;
        muki = 1;
    }
    else {
        // ゆっくりと左右に揺動
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 被弾検知：前フレームよりHPが減っていたら被弾とみなす
    // 正確な被弾地点は構造体にないため、敵中心(enemy.x, enemy.y)を起点とする
    if (prevHp > enemy.hp) {
        hitCount++;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBloodTithe;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        // プレイヤー方向を基準角度とする
        pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        // param_i[0] に被弾回数を渡し、弾の色変化に使用
        pEnemyShotSet->param_i[0] = hitCount;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    prevHp = enemy.hp;
}