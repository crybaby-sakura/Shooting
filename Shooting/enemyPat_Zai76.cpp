// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：双極交差（ツイン・クロス）・メビウスの帯
static void ShotTwinCross(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 連射に適した軽いショット音を採用
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 3.5;

        // 敵が動いても発射時のベクトルを維持して真っ直ぐ飛ぶよう、移動量を保存
        pEnemyShot->param_d[0] = pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->param_d[1] = pEnemyShot->speed * sin(pEnemyShot->muki);

        // 0: A->B, 1: B->A の判別フラグ
        pEnemyShot->param_i[0] = pEnemyShotSet->kind;

        // 種類と色の設定（Aは赤、Bは青の中玉）
        if (pEnemyShot->param_i[0] == 0) {
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤
        }
        else {
            pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青
        }

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本の直線移動
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];

        // 画面中央（x=240付近）を通過したか判定
        bool isPastCenter = false;
        if (pShot->param_i[0] == 0 && pShot->x > 240.0) isPastCenter = true;
        if (pShot->param_i[0] == 1 && pShot->x < 240.0) isPastCenter = true;

        if (isPastCenter) {
            // 交差後の揺らぎ（ノイズゾーンの形成）
            // 進行方向（ほぼ水平）に対して垂直方向（Y方向）へ、
            // 周期と位相をずらしてブレさせることで、不規則な波形を生み出す
            if (pShot->param_i[0] == 0) {
                // A->Bの弾（右向き）
                pShot->y += sin(pShot->count * 0.25) * 2.0;
                pShot->x += cos(pShot->count * 0.15) * 0.5;
            }
            else {
                // B->Aの弾（左向き）
                pShot->y += cos(pShot->count * 0.20 + 1.5) * 2.0;
                pShot->x += sin(pShot->count * 0.18) * 0.5;
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_TwoBoss_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 120.0 - 30;
        enemy.y = 240.0 + 30;
        enemy.x2 = 360.0;
        enemy.y2 = 240.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    else {
        // 逆位相のサインカーブで上下に動く（周期約2秒、振幅120px）
        // 動きが速いことで、発射される弾の軌道が「回転するX字」になりやすくなる
        enemy.y = 240.0 + 240.0 * sin(count * 0.05);
        enemy.y2 = 240.0 - 240.0 * sin(count * 0.05);
    }

    // 10フレームに1回、AからB、BからAへ同時に発射
    if (count % 5 == 1) {
        // --- A -> B のセット生成 ---
        sEnemyShotSet* pSetA = new sEnemyShotSet;
        pSetA->count = 0;
        pSetA->patternFunc = ShotTwinCross;
        pSetA->kind = 0; // A用の識別
        pSetA->x = enemy.x;
        pSetA->y = enemy.y;
        pSetA->muki = atan2(enemy.y2 - enemy.y, enemy.x2 - enemy.x);

        pSetA->pEnemyShotHead = new sEnemyShot;
        pSetA->pEnemyShotHead->prev = pSetA->pEnemyShotHead;
        pSetA->pEnemyShotHead->next = pSetA->pEnemyShotHead;

        pSetA->prev = enemyShotSetHead.prev;
        pSetA->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetA;
        enemyShotSetHead.prev = pSetA;


        // --- B -> A のセット生成 ---
        sEnemyShotSet* pSetB = new sEnemyShotSet;
        pSetB->count = 0;
        pSetB->patternFunc = ShotTwinCross;
        pSetB->kind = 1; // B用の識別
        pSetB->x = enemy.x2;
        pSetB->y = enemy.y2;
        pSetB->muki = atan2(enemy.y - enemy.y2, enemy.x - enemy.x2);

        pSetB->pEnemyShotHead = new sEnemyShot;
        pSetB->pEnemyShotHead->prev = pSetB->pEnemyShotHead;
        pSetB->pEnemyShotHead->next = pSetB->pEnemyShotHead;

        pSetB->prev = enemyShotSetHead.prev;
        pSetB->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetB;
        enemyShotSetHead.prev = pSetB;
    }
}