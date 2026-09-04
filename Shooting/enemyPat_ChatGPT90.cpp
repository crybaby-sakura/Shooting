// enemyPat_brownian.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ブラウン運動弾
// 各弾が毎フレーム少しずつランダムに向きを変える。
// 大量の小玉を中心に、一部を中玉にして粒子群を表現する。
// ============================================================
static void ShotBrownian(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 1回の放出で大量の粒子を生成
        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;

            // 下方向を中心に広く散らす
            pEnemyShot->muki =
                pEnemyShotSet->muki +
                (GetRand(1200) - 600) / 1000.0;

            pEnemyShot->speed = (70 + GetRand(110)) / 100.0;

            // 大半は小玉、一部だけ中玉
            if (GetRand(5) == 0)
                pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン
            else
                pEnemyShot->kind = img_enemyShotSmallBall[6];  // 白

            // ブラウン運動の揺らぎ幅を個体ごとに変える
            pEnemyShot->param_d[0] =
                (35 + GetRand(45)) / 1000.0;

            // 初期速度にも個体差を付ける
            pEnemyShot->param_d[1] =
                (80 + GetRand(40)) / 100.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // ----------------------------------------------------
        // ブラウン運動
        // 毎フレーム進行方向をランダムウォークさせる。
        // 一度変化した向きが次フレームにも残るため、
        // 完全な乱数移動ではなく、不規則に漂う軌跡になる。
        // ----------------------------------------------------
        double turn = pShot->param_d[0];

        if (GetRand(1) == 0)
            pShot->muki -= turn;
        else
            pShot->muki += turn;

        // 速度もわずかに揺らす
        pShot->speed +=
            (GetRand(20) - 10) / 1000.0;

        if (pShot->speed < 0.65)
            pShot->speed = 0.65;
        if (pShot->speed > 2.0)
            pShot->speed = 2.0;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_BrownianMotion_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;

        muki = 1;
        shot_count = 0;
    }
    else {
        // ボスがゆっくり左右へ揺れる
        enemy.x += 0.65 * (double)muki;

        if (count % 150 == 75)
            muki *= -1;
    }

    // 短い間隔で次々と粒子群を放出
    if (count % 35 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBrownian;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;

        // プレイヤー方向を基準にしつつ、かなり広く散らす
        pEnemyShotSet->muki =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x
            );

        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}