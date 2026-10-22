#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ニュートンのゆりかご：衝突時に飛び出す細かな弾
static void ShotCradleImpact(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵弾セットを作成して共有リストへ追加する
static sEnemyShotSet* CreateShotSet(double x, double y, sEnemyShotSet::PatternFunc patternFunc)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = DX_PI / 2.0;
    pEnemyShotSet->kind = 0;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// ニュートンのゆりかご本体
static void ShotNewtonCradle(sEnemyShotSet* pEnemyShotSet)
{
    static const int BALL_COUNT = 5;
    static const int STRING_PART_COUNT = 11;
    static const int REST_FRAMES = 60/2;
    static const int SWING_FRAMES = 120/2;
    static const int CYCLE_FRAMES = REST_FRAMES + SWING_FRAMES + SWING_FRAMES + REST_FRAMES;
    static const double SPACING = 50.0;
    static const double ROPE_LENGTH = 55.0;
    static const double MAX_SWING = 62.0 * DX_PI / 180.0;

    if (pEnemyShotSet->count == 0) {
        // おもりを表現する大玉
        for (int i = 0; i < BALL_COUNT; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 0.0;
            pEnemyShot->y = 0.0;
            pEnemyShot->muki = DX_PI / 2.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLargeBall[8]; // 橙
            pEnemyShot->param_i[0] = 0; // 0: おもり
            pEnemyShot->param_i[1] = i; // おもり番号
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 糸を表現する銃弾を等間隔に並べる
        for (int i = 0; i < BALL_COUNT; i++) {
            for (int j = 0; j < STRING_PART_COUNT; j++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = 0.0;
                pEnemyShot->y = 0.0;
                pEnemyShot->muki = DX_PI / 2.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotBullet[6]; // 白
                pEnemyShot->param_i[0] = 1; // 1: 糸
                pEnemyShot->param_i[1] = i; // おもり番号
                pEnemyShot->param_i[2] = j; // 糸の分割番号
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 敵本体の下にゆりかご全体を追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 25.0;

    const int cycle = pEnemyShotSet->count / CYCLE_FRAMES;
    const int phase = pEnemyShotSet->count % CYCLE_FRAMES;
    const bool firstLeft = (cycle % 2 == 0);
    int activeBall = -1;
    int swingStart = REST_FRAMES;

    if (phase >= swingStart && phase < swingStart + SWING_FRAMES) {
        activeBall = firstLeft ? 0 : BALL_COUNT - 1;
    }
    else if (phase >= swingStart + SWING_FRAMES && phase < swingStart + SWING_FRAMES * 2) {
        activeBall = firstLeft ? BALL_COUNT - 1 : 0;
    }

    // 端のおもりが戻ってきた瞬間に、反対側のおもりを送り出す
    if (phase == swingStart + SWING_FRAMES || phase == swingStart + SWING_FRAMES * 2) {
        double impactX;
        if (phase == swingStart + SWING_FRAMES) {
            impactX = firstLeft ? -SPACING * 1.5 : SPACING * 1.5;
        }
        else {
            impactX = firstLeft ? SPACING * 1.5 : -SPACING * 1.5;
        }

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pImpactSet = CreateShotSet(pEnemyShotSet->x + impactX, pEnemyShotSet->y + ROPE_LENGTH, ShotCradleImpact);
        for (int i = 0; i < 21; i++) for (int j = 0; j < 7; j++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pImpactSet->x;
            pEnemyShot->y = pImpactSet->y;
            pEnemyShot->muki = DX_PI / 2.0 + (i - 10) * (6.0 * DX_PI / 180.0);
            pEnemyShot->speed = 2.0 + i * 0.18 - j * 0.3;
            pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄
            pEnemyShot->prev = pImpactSet->pEnemyShotHead->prev;
            pEnemyShot->next = pImpactSet->pEnemyShotHead;
            pImpactSet->pEnemyShotHead->prev->next = pEnemyShot;
            pImpactSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int type = pShot->param_i[0];
        const int ballIndex = pShot->param_i[1];
        const double pivotX = pEnemyShotSet->x - SPACING * (BALL_COUNT - 1) / 2.0 + SPACING * ballIndex;
        const double pivotY = pEnemyShotSet->y;
        double ballX = pivotX;
        double ballY = pivotY + ROPE_LENGTH;
        double ropeAngle = DX_PI / 2.0;

        if (activeBall >= 0 && ballIndex == activeBall) {
            int localPhase = phase - swingStart;
            if (localPhase >= SWING_FRAMES) localPhase -= SWING_FRAMES;
            double swingProgress = (double)localPhase / (double)SWING_FRAMES;
            double swingAngle = MAX_SWING * sin(DX_PI * swingProgress);
            if (ballIndex == 0) {
                ropeAngle = DX_PI / 2.0 + swingAngle;
            }
            else {
                ropeAngle = DX_PI / 2.0 - swingAngle;
            }
            ballX = pivotX + ROPE_LENGTH * cos(ropeAngle);
            ballY = pivotY + ROPE_LENGTH * sin(ropeAngle);
        }

        if (type == 0) {
            pShot->x = ballX;
            pShot->y = ballY;
            pShot->muki = ropeAngle;
        }
        else {
            const int segment = pShot->param_i[2];
            const double distance = ROPE_LENGTH * ((double)segment + 0.5) / (double)STRING_PART_COUNT;
            pShot->x = pivotX + distance * cos(ropeAngle);
            pShot->y = pivotY + distance * sin(ropeAngle);
            pShot->muki = ropeAngle;
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_NewtonsCradle_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        CreateShotSet(enemy.x, enemy.y + 25.0, ShotNewtonCradle);
    }
    else {
        enemy.x += 0.75 * (double)muki;
        if (count % 140 == 70) muki *= -1;
    }
}
