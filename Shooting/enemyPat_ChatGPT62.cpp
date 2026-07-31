// enemyPat_spiralScramble.cpp
// 乱流回廊「スパイラル・スクランブル」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕パターン
// 高速回転放射 + 一度だけ屈折 + 加減速
// ------------------------------------------------------------
static void ShotSpiralScramble(sEnemyShotSet* pEnemyShotSet)
{
    // --------------------------------------------------------
    // 発射処理
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 毎フレーム少数弾を放射
    if (pEnemyShotSet->count < 5) {
        for (int i = 0; i < 2; ++i) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 少しだけ回転速度をずらした渦を重ねる
            double angle = pEnemyShotSet->muki;
            angle += (i == 0 ? -0.09 : 0.09);

            pEnemyShot->muki = angle;

            // 高速銃弾
            pEnemyShot->speed = 4.6 + i * 0.25;
            pEnemyShot->kind = img_enemyShotBullet[6]; // 白銃弾

            // ----------------------------------------------------
            // 自由パラメータ
            // param_i[0] : 屈折済みフラグ
            // param_i[1] : 屈折方向 (-1 or +1)
            // param_i[2] : 速度モード (0=減速, 1=加速)
            // param_i[3] : 屈折フレーム (40-60)
            // ----------------------------------------------------
            pEnemyShot->param_i[0] = 0;
            pEnemyShot->param_i[1] = (GetRand(1) == 0) ? -1 : 1;
            pEnemyShot->param_i[2] = GetRand(1);
            pEnemyShot->param_i[3] = 30 + GetRand(20); // 40-60

            // 軌跡が長く残るよう少し広め
            pEnemyShot->margin = 40.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --------------------------------------------------------
    // 弾更新処理
    // --------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // 一度だけ屈折
        if (!pShot->param_i[0] &&
            pShot->count >= pShot->param_i[3]) {

            pShot->muki += pShot->param_i[1] * (DX_PI / 4.0); // ±45°
            pShot->param_i[0] = 1;

            // 屈折後に色を変えて視認性を上げる
            if (pShot->param_i[2])
                pShot->kind = img_enemyShotScale[1]; // 黄鱗弾 = 加速
            else
                pShot->kind = img_enemyShotScale[3]; // シアン鱗弾 = 減速
        }

        // 屈折後のみ速度変化
        if (pShot->param_i[0]) {
            if (pShot->param_i[2]) {
                // 加速組
                pShot->speed += 0.015;
                if (pShot->speed > 5.0) pShot->speed = 5.0;
            }
            else {
                // 減速組
                pShot->speed *= 0.985;
                if (pShot->speed < 1.2) pShot->speed = 1.2;
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_TooChaotic_ChatGPT()
{
    static double baseAngle;
    static int rotateDir;
    static int phaseCount;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 20.0;
        enemy.maxHp = enemy.hp = 200;

        baseAngle = 0.0;
        rotateDir = 1;
        phaseCount = 0;
    }

    // --------------------------------------------------------
    // ボス移動
    // 上部を左右へゆっくり往復
    // --------------------------------------------------------
    enemy.x = 240.0 + sin(count * 0.018) * 120.0;
    enemy.y = 20.0 + sin(count * 0.011) * 6.0;

    // --------------------------------------------------------
    // 約2秒ごとに回転方向反転
    // --------------------------------------------------------
    phaseCount++;
    if (phaseCount >= 180) {
        phaseCount = 0;
        rotateDir *= -1;
    }

    // --------------------------------------------------------
    // 高速回転放射
    // 奇数・偶数で回転速度を微妙に変える
    // --------------------------------------------------------
    if (count % 4 == 1) {
        double rotSpeed;

        if ((count / 4) % 2 == 0)
            rotSpeed = 0.075; // 約4.3°/frame
        else
            rotSpeed = 0.063; // 約3.6°/frame

        baseAngle += rotSpeed * rotateDir;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiralScramble;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = baseAngle;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}