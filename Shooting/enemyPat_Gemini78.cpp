// enemyPat_TidalRondo.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：潮汐の輪舞（タイダル・ロンド）
static void ShotTidalRondo(sEnemyShotSet* pSet)
{
    // 青弾の発射周期
    const int BLUE_SHOT_INTERVAL = 90;

    // 予告音：青弾発射の30フレーム前
    if (pSet->count % BLUE_SHOT_INTERVAL == BLUE_SHOT_INTERVAL - 30) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 静寂の檻（紫色の低速中玉）の生成
    if (pSet->count % 4 == 0) {
        // 発射音はうるさくならないよう適度に間引く
        if (pSet->count % 16 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 万華鏡のように回転しながら広がる
        double baseAngle = pSet->count * 0.03;
        int way = 16;
        for (int i = 0; i < way; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = baseAngle + i * (DX_PI * 2.0 / way);
            pShot->speed = 1.0;
            pShot->kind = img_enemyShotMediumBall[5]; // マゼンタ(紫)の中玉

            // パラメータで状態を管理
            pShot->param_i[0] = 0; // 0: 紫弾
            pShot->param_d[0] = pShot->speed * cos(pShot->muki); // vx
            pShot->param_d[1] = pShot->speed * sin(pShot->muki); // vy

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 潮汐の波動（青色の高速大玉）の生成
    if (pSet->count > 0 && pSet->count % BLUE_SHOT_INTERVAL == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pShot->speed = 6.0;
        pShot->kind = img_enemyShotLargeBall[4]; // 青色の大玉

        pShot->param_i[0] = 1; // 1: 青弾
        pShot->param_d[0] = pShot->speed * cos(pShot->muki); // vx
        pShot->param_d[1] = pShot->speed * sin(pShot->muki); // vy

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    // 青弾のポインタ収集
    sEnemyShot* blueShots[64];
    int blueCount = 0;

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 1 && blueCount < 64) {
            blueShots[blueCount++] = p;
        }
        p = p->next;
    }

    // 弾の移動と干渉処理
    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 0) { // 紫弾
            // 青弾からの反発力を計算
            for (int i = 0; i < blueCount; i++) {
                double dx = p->x - blueShots[i]->x;
                double dy = p->y - blueShots[i]->y;
                double distSq = dx * dx + dy * dy;
                double effectRadius = 140.0; // 影響範囲（自機が通り抜けられるトンネルの太さ）

                if (distSq < effectRadius * effectRadius) {
                    double dist = sqrt(distSq);
                    if (dist < 1.0) dist = 1.0; // ゼロ除算防止

                    // 距離が近いほど反発力が強い
                    double force = (effectRadius - dist) * 0.02;
                    p->param_d[0] += force * (dx / dist);
                    p->param_d[1] += force * (dy / dist);
                }
            }

            // 速度上限の設定
            double currentSpeedSq = p->param_d[0] * p->param_d[0] + p->param_d[1] * p->param_d[1];
            double maxSpeed = 12.0;
            if (currentSpeedSq > maxSpeed * maxSpeed) {
                double ratio = maxSpeed / sqrt(currentSpeedSq);
                p->param_d[0] *= ratio;
                p->param_d[1] *= ratio;
            }

            // 速度の減衰（摩擦抵抗）
            // これにより、弾き飛ばされた後ゆっくりと停止し、流体のような動きになる
            p->param_d[0] *= 0.95;
            p->param_d[1] *= 0.95;

            // 常に原点(ボス)から外側へ広がる本来の力を微小に加える
            // トンネルが形成された後、ジワジワと元に戻ろうとする圧力になる
            double angleFromCenter = atan2(p->y - pSet->y, p->x - pSet->x);
            p->param_d[0] += 0.05 * cos(angleFromCenter);
            p->param_d[1] += 0.05 * sin(angleFromCenter);
        }
        else if (p->param_i[0] == 1) { // 青弾
            // 青弾は直進するだけ。減衰や反発は受けない
        }

        // 座標更新
        p->x += p->param_d[0];
        p->y += p->param_d[1];

        // 画像の向き(muki)を進行方向に合わせる
        p->muki = atan2(p->param_d[1], p->param_d[0]);

        p = p->next;
    }
}

// 敵本体のパターン（潮汐の輪舞）
void EnemyPat_TheMostFun_Gemini()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 150.0;
        enemy.maxHp = enemy.hp = 200;

        // 弾幕セットの登録
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTidalRondo;
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

    // ボス自身は優雅に８の字を描くように漂う
    enemy.x = 240.0 + 80.0 * sin(count * 0.015);
    enemy.y = 150.0 + 30.0 * sin(count * 0.03);

    // 発射源となる Set の座標もボスの動きに追従させる
    if (enemyShotSetHead.next != &enemyShotSetHead) {
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
    }
}