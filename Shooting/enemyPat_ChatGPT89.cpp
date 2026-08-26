// enemyPat_jumpscare.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ジャンプスケア弾幕
// 普段は静かな「顔」が形成され、終盤に顔全体が急接近する。
// 使う素材：小玉・中玉・大玉
// ============================================================

static double FACE_CX = 240.0;
static double FACE_CY = 205.0;
static int FACE_MAX = 72;

static void AddFaceShot(sEnemyShotSet* pSet, double x, double y, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = 0.0;
    pShot->speed = 0.0;
    pShot->kind = kind;

    // 初期位置を保存。param_d[0], [1] は顔の中心からの相対座標。
    pShot->param_d[0] = x - FACE_CX;
    pShot->param_d[1] = y - FACE_CY;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

static void ShotJumpScare(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    if (pEnemyShotSet->count == 0) {
        // 顔の輪郭：中玉
        const int outlineCount = 28;
        for (int i = 0; i < outlineCount; i++) {
            const double a = 2.0 * DX_PI * i / outlineCount;
            const double x = FACE_CX + 92.0 * cos(a);
            const double y = FACE_CY + 112.0 * sin(a);
            AddFaceShot(pEnemyShotSet, x, y, img_enemyShotMediumBall[5]);
        }

        // 左右の目：大玉を核に、小玉を周囲に添える。
        for (int side = -1; side <= 1; side += 2) {
            const double ex = FACE_CX + 42.0 * side;
            const double ey = FACE_CY - 25.0;
            AddFaceShot(pEnemyShotSet, ex, ey, img_enemyShotLargeBall[0]);
            AddFaceShot(pEnemyShotSet, ex + 10.0 * side, ey, img_enemyShotSmallBall[6]);
            AddFaceShot(pEnemyShotSet, ex - 10.0 * side, ey, img_enemyShotSmallBall[6]);
            AddFaceShot(pEnemyShotSet, ex, ey + 10.0, img_enemyShotSmallBall[6]);
            AddFaceShot(pEnemyShotSet, ex, ey - 10.0, img_enemyShotSmallBall[6]);
        }

        // 眉：小玉を並べて不気味な表情にする。
        for (int side = -1; side <= 1; side += 2) {
            for (int i = -2; i <= 2; i++) {
                const double x = FACE_CX + side * (30.0 + 8.0 * i);
                const double y = FACE_CY - 48.0 - 0.55 * fabs(8.0 * i);
                AddFaceShot(pEnemyShotSet, x, y, img_enemyShotSmallBall[1]);
            }
        }

        // 鼻：中玉を縦に配置。
        AddFaceShot(pEnemyShotSet, FACE_CX, FACE_CY + 5.0, img_enemyShotMediumBall[8]);
        AddFaceShot(pEnemyShotSet, FACE_CX, FACE_CY + 18.0, img_enemyShotSmallBall[8]);

        // 口：横長に並べ、中央だけ大玉にして「開いた口」を表現。
        for (int i = -4; i <= 4; i++) {
            const double x = FACE_CX + 11.0 * i;
            const double y = FACE_CY + 58.0 + 5.0 * fabs(i);
            AddFaceShot(pEnemyShotSet, x, y, img_enemyShotMediumBall[7]);
        }
        for (int i = -2; i <= 2; i++) {
            const double x = FACE_CX + 13.0 * i;
            const double y = FACE_CY + 73.0;
            AddFaceShot(pEnemyShotSet, x, y, img_enemyShotSmallBall[6]);
        }
    }

    // 顔の生成直後から少し待って、輪郭をゆっくり明滅させる。
    // 中盤までは位置を維持し、終盤だけ一気に「ズーム」する。
    double zoom = 1.0;
    if (pEnemyShotSet->count >= 150) {
        const double t = (pEnemyShotSet->count - 150) / 36.0;
        zoom = 1.0 + 5.0 * (t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t));
    }

    pShot = pEnemyShotSet->pEnemyShotHead->next;
    int index = 0;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 各弾の相対位置を拡大し、画面中央から顔が迫ってくるように見せる。
        pShot->x = FACE_CX + pShot->param_d[0] * zoom;
        pShot->y = FACE_CY + pShot->param_d[1] * zoom;

        // 急接近直前だけ微妙に震わせ、輪郭を不安定にする。
        if (pEnemyShotSet->count >= 138 && pEnemyShotSet->count < 150) {
            const double shake = (pEnemyShotSet->count - 138) * 0.45;
            pShot->x += sin(index * 1.73) * shake;
            pShot->y += cos(index * 1.21) * shake;
        }

        if (pShot->count >= 300) pShot->margin = -9999;

        index++;
        pShot = pShot->next;
    }

    // 効果音：形成開始→接近開始。
    if (pEnemyShotSet->count == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (pEnemyShotSet->count == 150) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
}

// 敵本体のパターン
void EnemyPat_JumpScare_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.65 * (double)muki;
        if (count % 140 == 70) muki *= -1;
    }

    // 顔を構成する弾幕を一つのセットとして生成。
    if (count % 300 == 1) {
        FACE_CX = 240.0 + GetRand(100) - 50;
        FACE_CY = 240.0 + GetRand(100) - 50;
        FACE_MAX = 60 + GetRand(20);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotJumpScare;
        pEnemyShotSet->x = FACE_CX;
        pEnemyShotSet->y = FACE_CY;
        pEnemyShotSet->muki = 0.0;
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