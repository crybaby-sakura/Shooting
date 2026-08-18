// D:/workspace/Visual_Cpp/Shooting/Shooting/enemyPat_TarakoSpaghetti.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：たらこスパゲッティ・アルデンテ (改)
static void ShotTarakoSpaghetti(sEnemyShotSet* pSet)
{
    int t = pSet->count;

    // --- Phase 1: スパゲッティ (黄レーザー) ---
    // 自機方向へ波打ちながら進む
    if (t < 300) {
        if (t % 6 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y + 10.0;
            p->speed = 2.5 + GetRand(10) / 10.0;

            // 自機への角度を基準にする
            double base_angle = atan2(player.y - p->y, player.x - p->x);

            // 波打ち用パラメータ
            p->param_d[0] = p->x; // start_x
            p->param_d[1] = p->y; // start_y
            p->param_d[2] = base_angle;
            p->param_d[3] = 30.0 + GetRand(40); // amp (振幅)
            p->param_d[4] = 0.04 + GetRand(40) / 1000.0; // freq (周波数)
            p->param_d[5] = GetRand(628) / 100.0; // phase (位相)

            p->kind = img_enemyShotLaser[1]; // 黄(1)

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // --- Phase 2: たらこ (赤小玉) ---
    if (t >= 60 && t < 300) {
        if (t % 10 == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            int num = 4 + GetRand(3);
            for (int i = 0; i < num; i++) {
                sEnemyShot* p = new sEnemyShot;
                p->x = pSet->x + GetRand(60) - 30;
                p->y = pSet->y + 20.0 + GetRand(40);
                p->muki = GetRand(628) / 100.0;
                p->speed = 1.5 + GetRand(20) / 10.0;
                p->kind = img_enemyShotSmallBall[0]; // 赤(0)

                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // --- Phase 3: 海苔 (黒銃弾) ---
    // 量を増やし、範囲を広くする
    if (t >= 60 && t < 360) {
        if (t % 5 == 0) {
            sEnemyShot* p = new sEnemyShot;
            // 範囲を広げる (±150px程度)
            p->x = pSet->x + GetRand(300) - 150;
            p->y = pSet->y + GetRand(40) - 20;
            p->speed = 1.2 + GetRand(10) / 10.0;
            p->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 100.0; // ばらつき
            p->kind = img_enemyShotBullet[7]; // 黒(7)

            // 回転・揺れ用パラメータ
            p->param_d[1] = 0.05 + GetRand(50) / 1000.0;
            p->param_d[2] = GetRand(628) / 100.0;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // --- Phase 4: バター (黄中玉) ---
    // 量を増やす
    if (t == 360) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int num = 30;
        for (int i = 0; i < num; i++) {
            sEnemyShot* p = new sEnemyShot;
            double angle = 2 * DX_PI * i / num;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = angle;
            p->speed = 1.2;
            p->kind = img_enemyShotMediumBall[1]; // 黄(1)

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // --- 移動処理 ---
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->kind == img_enemyShotLaser[1]) {
            // スパゲッティ：自機方向ベース＋サインカーブ
            // pShot->count はメインルーチンでインクリメントされる
            double dist = pShot->count * pShot->speed;
            double base = pShot->param_d[2];
            double amp = pShot->param_d[3];
            double freq = pShot->param_d[4];
            double phase = pShot->param_d[5];

            // 進行方向に対して垂直な揺れ
            double offset = amp * sin(dist * freq + phase);

            double nx = cos(base + DX_PI / 2.0);
            double ny = sin(base + DX_PI / 2.0);

            double next_x = pShot->param_d[0] + cos(base) * dist + nx * offset;
            double next_y = pShot->param_d[1] + sin(base) * dist + ny * offset;

            // 進行方向に合わせて傾きを更新
            double dx = next_x - pShot->x;
            double dy = next_y - pShot->y;
            if (dx != 0 || dy != 0) pShot->muki = atan2(dy, dx);

            pShot->x = next_x;
            pShot->y = next_y;
        }
        else if (pShot->kind == img_enemyShotBullet[7]) {
            // 海苔：ヒラヒラ落下
            double swing = sin(pShot->count * pShot->param_d[1] + pShot->param_d[2]) * 0.8;
            pShot->muki = DX_PI / 2.0 + swing;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // たらこ、バター
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_TarakoSpaghetti_Qwen()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // ボスの移動
    enemy.x = 240.0 + 120.0 * sin(count * 0.01);

    // 弾幕生成 (約8秒周期)
    if (count % 480 == 60) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTarakoSpaghetti;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        pSet->muki = DX_PI / 2.0;
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