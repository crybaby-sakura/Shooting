#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ピッチドロップ実験
static void ShotPitchDrop(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 1つの集合が作られた直後に、静止した「液滴群」を配置する。
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 液体の塊を思わせるように、上下で少し幅の違う配置にする。
        const double centerX = pEnemyShotSet->x;
        const double baseY = pEnemyShotSet->y + 100.0;

        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;

            const double row = i / 4;
            const double col = i % 4;
            const double rowWidth = 1.0 + row * 0.12;

            pEnemyShot->x = centerX + (col - 1.5) * 52.0 * rowWidth;
            pEnemyShot->y = baseY + row * 42.0;
            pEnemyShot->muki = DX_PI / 2.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumOval[3]; // シアンの中楕円弾：液滴
            pEnemyShot->param_i[0] = 0;                    // 0:静止、1:落下、2:波紋
            pEnemyShot->param_i[1] = i;                    // 液滴ID
            pEnemyShot->param_i[2] = 0;                    // 波紋を出したか
            pEnemyShot->param_d[0] = 0.0;                  // 落下速度

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 一定間隔で「次の一滴が落ちる」予告を入れ、その後に落下させる。
    // 予告から落下開始までの間は、ほかの液滴は静止したまま。
    if (pEnemyShotSet->count >= 45 && pEnemyShotSet->count % 90 == 45) {
        //if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        //PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count >= 65 && pEnemyShotSet->count % 90 == 65) {
        const int dropId = ((pEnemyShotSet->count - 65) / 90) % 12;
        bool activated = false;

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0 && pShot->param_i[1] == dropId) {
                pShot->param_i[0] = 1;
                pShot->param_i[2] = 1;
                pShot->param_d[0] = 0.32;
                pShot->speed = pShot->param_d[0];
                activated = true;
                break;
            }

            pShot = pShot->next;
        }

        if (activated) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 落下開始直後に、周囲へ小型弾を広げて「波紋」を表現する。
            if (pShot->param_i[2] == 1) {
                for (int i = 0; i < 10*9; i++) {
                    sEnemyShot* pRipple = new sEnemyShot;
                    const double angle = 2.0 * DX_PI * (double)i / 10.0/9;

                    pRipple->x = pShot->x;
                    pRipple->y = pShot->y;
                    pRipple->muki = angle;
                    pRipple->speed = 1.3;
                    pRipple->kind = img_enemyShotSmallBall[3]; // シアンの小玉：波紋
                    pRipple->param_i[0] = 2;

                    pRipple->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pRipple->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pRipple;
                    pEnemyShotSet->pEnemyShotHead->prev = pRipple;
                }

                pShot->param_i[2] = 0;
            }

            // 「非常にゆっくり落ちる一滴」から、徐々に落下が進む。
            pShot->param_d[0] += 0.0035;
            if (pShot->param_d[0] > 1.6) {
                pShot->param_d[0] = 1.6;
            }
            pShot->speed = pShot->param_d[0];
            pShot->x += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed;
        }
        else if (pShot->param_i[0] == 2) {
            // 波紋は直線的に外へ広がる。
            pShot->x += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed;
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_PitchDrop_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 48.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.72 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 静止した液滴群を一定間隔で追加し、複数の実験容器が並ぶような密度感を作る。
    if (count % 420 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPitchDrop;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = DX_PI / 2.0;
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
