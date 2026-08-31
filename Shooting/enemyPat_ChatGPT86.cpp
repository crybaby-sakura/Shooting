// enemyPat_closeCombat.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 近接戦闘：ボス自身がプレイヤーへ肉薄し、包囲弾を撒いて
// 急加速で離脱する攻防を繰り返す。
// ============================================================
static void ShotCloseCombat(sEnemyShotSet* pEnemyShotSet)
{
    // 1セット目：接近時に放つ、短い扇状の弾列
    if (pEnemyShotSet->kind == 0) {
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy))
                StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            
            for (int i = 0; i < 7; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                double base = pEnemyShotSet->muki;
                double spread = (i - 3) * 0.11;

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = base + spread;
                pEnemyShot->speed = 2.3 + 0.12 * (6 - abs(i - 3));
                pEnemyShot->kind = img_enemyShotDiamond[3];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    // 2セット目：接近点でプレイヤーを囲む円弧状の弾
    else {
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_extreme))
                StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 18; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                double ang = pEnemyShotSet->muki + i * (DX_PI * 2.0 / 18.0);
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = ang;
                pEnemyShot->speed = 1.55;
                pEnemyShot->kind = img_enemyShotMediumBall[5];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾そのものの移動だけをここで行う。
    // count の加算や画面外の削除はメインルーチン側。
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_CloseCombat_ChatGPT()
{
    static int phase;
    static double dir;
    static double targetX;
    static double targetY;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;

        phase = 0;
        dir = 1.0;
        targetX = player.x;
        targetY = player.y;

        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    // 0: 上から狙う
    // 1: 肉薄
    // 2: 包囲弾
    // 3: 急速離脱
    // --------------------------------------------------------
    if (phase == 0) {
        double dx = player.x - enemy.x;
        double dy = player.y - enemy.y;
        double len = sqrt(dx * dx + dy * dy);

        if (len > 0.001) {
            enemy.x += dx / len * 1.2 * 5;
            enemy.y += dy / len * 1.2 * 5;
        }

        if (count % 3 == 0 && len < 150.0) {
            targetX = player.x;
            targetY = player.y;
            phase = 1;
        }
    }
    else if (phase == 1) {
        double dx = targetX - enemy.x;
        double dy = targetY - enemy.y;
        double len = sqrt(dx * dx + dy * dy);

        if (len > 0.001) {
            double speed = 4.8 * 2;
            enemy.x += dx / len * speed;
            enemy.y += dy / len * speed;
        }

        // プレイヤーのかなり近くまで来たら包囲弾
        if (len < 32.0) {
            if (count % 1 == 0) {
                sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                pEnemyShotSet->count = 0;
                pEnemyShotSet->patternFunc = ShotCloseCombat;
                pEnemyShotSet->kind = 1;
                pEnemyShotSet->x = enemy.x;
                pEnemyShotSet->y = enemy.y;
                pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

                pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

                pEnemyShotSet->prev = enemyShotSetHead.prev;
                pEnemyShotSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pEnemyShotSet;
                enemyShotSetHead.prev = pEnemyShotSet;
            }

            dir = (player.x >= 240.0) ? -1.0 : 1.0;
            phase = 2;
        }
    }
    else if (phase == 2) {
        // 直前までの位置を利用して横へすり抜ける
        enemy.x += dir * 6.2;

        // 離脱中は短い扇状の弾列を残す
        if (count % 4 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotCloseCombat;
            pEnemyShotSet->kind = 0;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        if ((dir < 0.0 && enemy.x < 70.0) || (dir > 0.0 && enemy.x > 410.0)) {
            phase = 3;
        }
    }
    else {
        // 上へ戻りながら次の接近対象を作る
        enemy.y -= 2.0 * 5;
        enemy.x += -dir * 0.8;

        if (enemy.y < 70.0) {
            if (CheckSoundMem(sound_enemyCharge))
                StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            enemy.y = 70.0;
            phase = 0;
        }
    }
}