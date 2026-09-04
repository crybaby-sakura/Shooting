// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：スリンガー.ioモチーフ『滑蛇の鱗道（スリザー・コイル）』
static void ShotSlitherSnake(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    // フェーズ1 & 2: うねりながら移動し、体を落とす (0 ~ 239フレーム)
    if (pEnemyShotSet->count < 240) {
        // 頭の移動処理
        double targetMuki;
        double dx = player.x - pEnemyShotSet->x;
        double dy = player.y - pEnemyShotSet->y;
        double dist = sqrt(dx * dx + dy * dy);

        if (pEnemyShotSet->count < 120) {
            // 接近フェーズ：プレイヤーに向かう
            targetMuki = atan2(dy, dx);
        }
        else {
            // 包囲フェーズ：プレイヤーの周囲を旋回
            if (dist > 100.0) {
                targetMuki = atan2(dy, dx); // 遠ければ近づく
            }
            else {
                // 旋回方向を決める (1 or -1)
                if (pEnemyShotSet->param_i[0] == 0) {
                    pEnemyShotSet->param_i[0] = (GetRand(1) ? 1 : -1);
                }
                targetMuki = atan2(dy, dx) + (DX_PI / 2) * pEnemyShotSet->param_i[0];
            }
        }

        // 画面外に逃がさないように中心へ誘導
        if (pEnemyShotSet->x < 40.0 || pEnemyShotSet->x > 440.0 ||
            pEnemyShotSet->y < 40.0 || pEnemyShotSet->y > 440.0) {
            targetMuki = atan2(240.0 - pEnemyShotSet->y, 240.0 - pEnemyShotSet->x);
        }

        // 滑らかに方向を変える
        double diff = targetMuki - pEnemyShotSet->muki;
        while (diff > DX_PI) diff -= DX_PI * 2;
        while (diff < -DX_PI) diff += DX_PI * 2;
        pEnemyShotSet->muki += diff * 0.05;

        // うねり成分を加えて移動
        double moveMuki = pEnemyShotSet->muki + 0.4 * sin(pEnemyShotSet->count * 0.12);
        pEnemyShotSet->x += 2.2 * cos(moveMuki);
        pEnemyShotSet->y += 2.2 * sin(moveMuki);

        // 5フレームに1回、体（小弾）をドロップする
        if (pEnemyShotSet->count % 5 == 0) {
            pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = 0;
            pShot->speed = 0;
            pShot->kind = img_enemyShotSmallBall[4]; // 4:青色の小玉
            pShot->param_i[0] = 0; // 状態: 0(静止中の体)

            // リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // フェーズ3: 散開 (240フレーム目)
    if (pEnemyShotSet->count == 240) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 全ての体を散開させる
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                pShot->param_i[0] = 1; // 状態: 1(散開済み)
                pShot->kind = img_enemyShotSmallBall[6]; // 6:白色の小玉（餌っぽく）
                // 0 ~ 2PI のランダムな方向
                pShot->muki = (GetRand(3600)) / 1800.0 * DX_PI;
                // 2.0 ~ 4.0 のランダムな速度
                pShot->speed = (200 + GetRand(200)) / 100.0;
            }
            pShot = pShot->next;
        }
    }

    // 全ての体弾の移動処理 (散開後のみ)
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Slitherio_Zai()
{
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
    }
    else {
        // 敵本体は上下左右にゆっくりうねうね動く
        enemy.x = 240.0 + 100.0 * sin(count * 0.02);
        enemy.y = 40.0 + 20.0 * sin(count * 0.05);
    }

    // 200フレームごとに新しい蛇を生成
    if (count % 200 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSlitherSnake;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->param_i[0] = 0; // 旋回方向未定

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}