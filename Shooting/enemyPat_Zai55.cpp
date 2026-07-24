// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

// 弾幕：彩華「極彩の万華鏡」
static void ShotKaleidoscope(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 弾幕開始時の効果音
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // --- 第1レイヤー（赤）・第2レイヤー（青）の螺旋発射 ---
    // 2フレームに1回、計12発の小玉を発射
    if (pEnemyShotSet->count % 2 == 0) {
        // 1フレームあたりの回転角度（2度）
        double rotSpeed = PI / 90.0;

        for (int i = 0; i < 6; i++) {
            // 第1レイヤー（赤・時計回り）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * (PI / 3.0) + pEnemyShotSet->count * rotSpeed;
            pEnemyShot->speed = 4.0;
            pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤い小玉
            pEnemyShot->param_i[0] = 0; // 螺旋弾であることを示すフラグ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 第2レイヤー（青・反時計回り）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 15度（PI/12）ずらして交差させる
            pEnemyShot->muki = (PI / 12.0) + i * (PI / 3.0) - pEnemyShotSet->count * rotSpeed;
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青い小玉
            pEnemyShot->param_i[0] = 0; // 螺旋弾であることを示すフラグ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 第3レイヤー（緑）のリング発射 ---
    // 120フレーム（2秒）に1回、真円に広がる緑の弾を発射
    if (pEnemyShotSet->count % 120 == 0 && pEnemyShotSet->count > 0) {
        int ringNum = 36; // 36方向に発射
        for (int i = 0; i < ringNum; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * (2.0 * PI / ringNum);
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotSmallBall[2]; // 緑の小玉
            pEnemyShot->param_i[0] = 1; // リング弾であることを示すフラグ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 弾の移動・挙動制御 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 第3レイヤー（緑リング）の脈動制御
        // 40フレーム目で一度停止し、55フレーム目から再び動き出す
        if (pShot->param_i[0] == 1) {
            if (pShot->speed > 0.0 && pShot->count >= 40) {
                pShot->speed = 0.0;
            }
            else if (pShot->speed == 0.0 && pShot->count >= 55) {
                pShot->speed = 1.5;
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_ThumbnailFriendly_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 螺旋を全画面に広げるため、少し下めの位置に配置
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;

        // 弾幕開始前の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 予告音が鳴り終わったタイミングで弾幕セットを生成
    if (count == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKaleidoscope;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}