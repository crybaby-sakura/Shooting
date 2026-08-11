// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// たらこスパゲッティ
//
// 前半:
//   細長い銃弾を麺に見立て、波打つ麺を大量に形成する。
//
// 中盤:
//   赤い小玉をたらこ粒として麺に沿って流す。
//
// 後半:
//   麺そのものが中央へ巻き込まれ、一本の巨大な渦になる。
//   たらこ粒も同じ中心線を通って渦へ巻き込まれる。
// ============================================================

const int T = 660;

// ============================================================
// 麺
// ============================================================
static void ShotSpaghetti(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < 32; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->param_i[0] = i;
            pShot->margin = 9999;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;

            // 細長い弾を麺として使用
            pShot->kind = img_enemyShotBullet[6];
            pShot->speed = 0.0;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;

    while (pShot != pSet->pEnemyShotHead) {
        int i = pShot->param_i[0];

        // ====================================================
        // 通常時：横に流れる波打った麺
        // ====================================================
        if (pSet->count < 300) {
            double x =
                -140.0 +
                i * 17.0 +
                pSet->count * 0.8;

            double phase =
                pSet->param_d[0] +
                i * 0.22 +
                pSet->count * 0.035;

            double y =
                pSet->y +
                sin(phase) * 50.0;

            pShot->x = x;
            pShot->y = y;

            // 麺の接線方向
            double dy =
                cos(phase) * 50.0 * 0.035;

            pShot->muki = atan2(dy, 2.0);
        }

        // ====================================================
        // 300～480フレーム：
        // 横向きの麺を徐々に中央の渦へ変形
        // ====================================================
        else {
            double k =
                (pSet->count - 300) / 180.0;

            if (k > 1.0)
                k = 1.0;

            // 滑らかに変形
            double ease = k * k * (3.0 - 2.0 * k);

            // 元の麺の位置
            double oldX =
                -140.0 +
                i * 17.0 +
                pSet->count * 0.8;

            double phase =
                pSet->param_d[0] +
                i * 0.22 +
                pSet->count * 0.035;

            double oldY =
                pSet->y +
                sin(phase) * 50.0;

            // ------------------------------------------------
            // 最終形状となる渦
            //
            // i は「渦の内外」ではなく、
            // 麺の一本一本の位相差として使用する。
            // ------------------------------------------------
            double a =
                pSet->param_d[0] +
                i * 0.11 +
                (pSet->count - 300) * 0.055;

            // 時間とともに半径を縮める
            double radius =
                230.0 * (1.0 - ease);

            // 麺ごとに少しだけ外周方向へずらす
            double offset =
                (i - 15.5) * 1.2;

            radius += offset;

            double spiralX =
                240.0 +
                cos(a) * radius;

            double spiralY =
                240.0 +
                sin(a) * radius;

            // 横方向の麺から渦へ補間
            pShot->x =
                oldX * (1.0 - ease) +
                spiralX * ease;

            pShot->y =
                oldY * (1.0 - ease) +
                spiralY * ease;

            // 渦の接線方向
            pShot->muki =
                a + DX_PI / 2.0;
        }

        if (pShot->count == T) pShot->margin = -9999;

        pShot = pShot->next;
    }
}


// ============================================================
// たらこ粒
// ============================================================
static void ShotTarako(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < 8; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            pShot->param_i[0] = i;
            pShot->margin = 40;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;

            // 赤い小玉をたらこ粒として使用
            pShot->kind = img_enemyShotSmallBall[0];
            pShot->speed = 0.0;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;

    while (pShot != pSet->pEnemyShotHead) {
        int i = pShot->param_i[0];

        // ====================================================
        // 通常時：
        // 麺の波の上を流れる
        // ====================================================
        if (pSet->count < 260) {
            double t =
                pSet->count * 0.008 +
                i * 0.8 +
                pSet->param_d[0];

            double x =
                -30.0 +
                fmod(t * 130.0, 540.0);

            double y =
                pSet->y +
                sin(t) * 50.0;

            pShot->x = x;
            pShot->y = y;

            pShot->muki =
                atan2(cos(t) * 50.0 * 0.045, 1.0);
        }

        // ====================================================
        // 260～440フレーム：
        // たらこ粒も麺と同じ渦へ移行
        // ====================================================
        else {
            double k =
                (pSet->count - 260) / 180.0;

            if (k > 1.0)
                k = 1.0;

            double ease =
                k * k * (3.0 - 2.0 * k);

            // 元の位置
            double t =
                pSet->count * 0.008 +
                i * 0.8 +
                pSet->param_d[0];

            double oldX =
                -30.0 +
                fmod(t * 130.0, 540.0);

            double oldY =
                pSet->y +
                sin(t) * 50.0;

            // ------------------------------------------------
            // 麺と同じ中心を持つ渦
            // ------------------------------------------------
            double a =
                pSet->param_d[0] +
                i * 0.65 +
                (pSet->count - 260) * 0.015;

            double radius =
                225.0 * (1.0 - ease);

            // 粒ごとに渦の位置を少しずらす
            radius += (i - 3.5) * 4.0;

            double spiralX =
                240.0 +
                cos(a) * radius;

            double spiralY =
                240.0 +
                sin(a) * radius;

            pShot->x =
                oldX * (1.0 - ease) +
                spiralX * ease;

            pShot->y =
                oldY * (1.0 - ease) +
                spiralY * ease;

            pShot->muki =
                a + DX_PI / 2.0;
        }

        if (pShot->count == T) pShot->margin = -9999;

        pShot = pShot->next;
    }
}


// ============================================================
// 敵本体
// ============================================================
void EnemyPat_TarakoSpaghetti_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 45.0;

        enemy.maxHp = enemy.hp = 200;

        muki = 1;
    }
    else {
        enemy.x += muki * 0.75;

        if (enemy.x > 400.0)
            muki = -1;

        if (enemy.x < 80.0)
            muki = 1;
    }

    int countT = count % T;

    // ========================================================
    // 麺
    // ========================================================
    if (countT < 300 && countT % 18 == 1) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = ShotSpaghetti;

        pSet->x = 240.0;

        pSet->y =
            40.0 +
            ((countT / 18) % 9) * 60.0;

        pSet->muki = 0.0;

        pSet->param_d[0] =
            ((countT / 18) % 9) * 0.65;

        pSet->pEnemyShotHead = new sEnemyShot;

        pSet->pEnemyShotHead->prev =
            pSet->pEnemyShotHead;

        pSet->pEnemyShotHead->next =
            pSet->pEnemyShotHead;

        pSet->prev =
            enemyShotSetHead.prev;

        pSet->next =
            &enemyShotSetHead;

        enemyShotSetHead.prev->next =
            pSet;

        enemyShotSetHead.prev =
            pSet;
    }


    // ========================================================
    // たらこ
    // ========================================================
    if (countT >= 100 &&
        countT < 440 &&
        countT % 28 == 1) {

        sEnemyShotSet* pSet =
            new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = ShotTarako;

        pSet->x = 0.0;

        pSet->y =
            40.0 +
            ((countT / 28) % 7) * 70.0;

        pSet->muki = 0.0;

        pSet->param_d[0] =
            ((countT / 28) % 7) * 0.8;

        pSet->pEnemyShotHead =
            new sEnemyShot;

        pSet->pEnemyShotHead->prev =
            pSet->pEnemyShotHead;

        pSet->pEnemyShotHead->next =
            pSet->pEnemyShotHead;

        pSet->prev =
            enemyShotSetHead.prev;

        pSet->next =
            &enemyShotSetHead;

        enemyShotSetHead.prev->next =
            pSet;

        enemyShotSetHead.prev =
            pSet;
    }
}