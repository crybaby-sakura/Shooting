#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：噴水「天上からの水冠」
// 小玉・中楕円弾を水滴に見立て、重力付きの放物線で噴水を表現する。
static void ShotFountain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 中央の水柱。中央ほど高く、外側ほど横へ大きく広がる。
        for (int i = -8; i <= 8; i++) {
            const double t = i / 8.0;
            const double vx = t * 1.75;
            const double vy = -4.20 + fabs(t) * 0.55;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->param_d[0] = vx;
            pEnemyShot->param_d[1] = vy;
            pEnemyShot->param_d[2] = 0.070;
            pEnemyShot->muki = atan2(vy, vx);
            pEnemyShot->speed = sqrt(vx * vx + vy * vy);

            // 水滴はシアンの小玉。
            if (abs(i) <= 2) {
                pEnemyShot->kind = img_enemyShotMediumBall[3];
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[3];
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 外側へ飛び散る水滴。白い小玉を混ぜて、噴水の飛沫感を出す。
        for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < 4; i++) {
                const double k = (double)i;
                const double vx = side * (2.25 + k * 0.42);
                const double vy = -3.30 + k * 0.28;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + side * (2.0 + k * 2.0);
                pEnemyShot->y = pEnemyShotSet->y + 2.0;
                pEnemyShot->param_d[0] = vx;
                pEnemyShot->param_d[1] = vy;
                pEnemyShot->param_d[2] = 0.070;
                pEnemyShot->muki = atan2(vy, vx);
                pEnemyShot->speed = sqrt(vx * vx + vy * vy);
                pEnemyShot->kind = img_enemyShotSmallBall[6];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 大きな水滴を少量混ぜ、頂点から落ちてくる部分を見やすくする。
        for (int i = -2; i <= 2; i++) {
            const double vx = i * 0.72;
            const double vy = -4.75 + fabs((double)i) * 0.16;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + i * 3.0;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->param_d[0] = vx;
            pEnemyShot->param_d[1] = vy;
            pEnemyShot->param_d[2] = 0.070;
            pEnemyShot->muki = atan2(vy, vx);
            pEnemyShot->speed = sqrt(vx * vx + vy * vy);
            pEnemyShot->kind = img_enemyShotMediumOval[3];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全ての水滴を重力で動かす。上昇後に頂点を作って、左右へ広がりながら落下する。
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->param_d[1] += pShot->param_d[2];
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];
        pShot->speed = sqrt(
            pShot->param_d[0] * pShot->param_d[0] +
            pShot->param_d[1] * pShot->param_d[1]);
        pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Fountain_ChatGPT()
{
    static int muki;
    static int fountain_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 150.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        fountain_count = 0;
    }
    else {
        // 噴水の発射位置が左右に動き、落下弾の着地点も少しずつ変化する。
        enemy.x += 0.85 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 噴水の立ち上がりを時間差で連続発生させる。
    if (count % 84 >= 1 && count % 84 <= 20 && count % 2 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFountain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;
        pEnemyShotSet->muki = -DX_PI / 2.0;
        pEnemyShotSet->kind = fountain_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
