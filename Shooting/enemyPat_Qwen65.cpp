// enemyPat_hydraTmp.cpp
// ヒュドラモチーフ弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const int HYDRA_HEAD_NUM = 10;
static const int HYDRA_HP_PER_HEAD = 20;
static const int HYDRA_STUMP_TIME = 300;
static const int HYDRA_FANG_CHARGE = 150;
static const int HYDRA_FANG_INTERVAL = 180;

enum
{
    HYDRA_ROLE_BULLET = 0,
    HYDRA_ROLE_HEAD = 1,
};

enum
{
    HYDRA_STATE_ALIVE = 0,
    HYDRA_STATE_STUMP = 1,
};

static int g_hydraPrevHp = 200;
static int g_hydraDamageAccum = 0;
static int g_hydraPendingStump = 0;

static double HydraNormalizeAngle(double angle)
{
    while (angle > DX_PI) angle -= DX_PI * 2.0;
    while (angle < -DX_PI) angle += DX_PI * 2.0;
    return angle;
}

static void HydraLinkShot(sEnemyShotSet* pSet, sEnemyShot* pShot)
{
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

static sEnemyShot* HydraAddShot(
    sEnemyShotSet* pSet,
    double x,
    double y,
    double muki,
    double speed,
    int kind,
    int role)
{
    sEnemyShot* pShot = new sEnemyShot;

    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->count = 0;
    pShot->kind = kind;
    pShot->margin = 20.0;

    for (int i = 0; i < 8; i++) {
        pShot->param_i[i] = 0;
        pShot->param_d[i] = 0.0;
    }

    pShot->param_i[0] = role;

    HydraLinkShot(pSet, pShot);

    return pShot;
}

static sEnemyShotSet* HydraCreateShotSet(
    void (*patternFunc)(sEnemyShotSet*),
    double x,
    double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;

    pSet->x = x;
    pSet->y = y;
    pSet->muki = 0.0;
    pSet->patternFunc = patternFunc;
    pSet->count = 0;
    pSet->kind = 0;

    for (int i = 0; i < 8; i++) {
        pSet->param_i[i] = 0;
        pSet->param_d[i] = 0.0;
    }

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

static void HydraPlaySound(int handle)
{
    if (CheckSoundMem(handle)) {
        StopSoundMem(handle);
    }
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

static void HydraUpdateHeadPosition(sEnemyShot* pHead)
{
    double offsetAngle = pHead->param_d[0];
    double phase = pHead->param_d[1];
    double radius = pHead->param_d[2];

    double sway = sin(count * 0.045 + phase) * 0.18;
    double angle = DX_PI / 2.0 + offsetAngle + sway;

    pHead->x = enemy.x + cos(angle) * radius;
    pHead->y = enemy.y + sin(angle) * radius;
}

static void HydraFireNeedle(sEnemyShotSet* pSet, sEnemyShot* pHead)
{
    double aim = atan2(player.y - pHead->y, player.x - pHead->x);
    double wiggle = sin(count * 0.13 + pHead->param_d[1]) * 0.30;
    double speed = 2.55 + GetRand(50) / 100.0;

    HydraAddShot(
        pSet,
        pHead->x,
        pHead->y,
        aim + wiggle,
        speed,
        img_enemyShotScale[2],
        HYDRA_ROLE_BULLET);
}

static void HydraFireFang(sEnemyShotSet* pSet, sEnemyShot* pHead)
{
    double aim = atan2(player.y - pHead->y, player.x - pHead->x);

    HydraAddShot(
        pSet,
        pHead->x,
        pHead->y,
        aim,
        4.35,
        img_enemyShotMediumOval[5],
        HYDRA_ROLE_BULLET);

    HydraAddShot(
        pSet,
        pHead->x,
        pHead->y,
        aim - 0.16,
        3.0,
        img_enemyShotSmallBall[4],
        HYDRA_ROLE_BULLET);

    HydraAddShot(
        pSet,
        pHead->x,
        pHead->y,
        aim + 0.16,
        3.0,
        img_enemyShotSmallBall[4],
        HYDRA_ROLE_BULLET);
}

static void HydraFireStumpPoison(sEnemyShotSet* pSet, sEnemyShot* pHead)
{
    double aim = atan2(player.y - pHead->y, player.x - pHead->x);

    sEnemyShot* pShot = HydraAddShot(
        pSet,
        pHead->x,
        pHead->y,
        aim,
        1.15,
        img_enemyShotMediumBall[2],
        HYDRA_ROLE_BULLET);

    pShot->param_i[1] = 1;
    pShot->param_i[2] = 220 + GetRand(60);
    pShot->param_d[0] = 0.028 + GetRand(20) / 1000.0;
}

static void HydraFireSplash(sEnemyShotSet* pSet, double x, double y)
{
    double center = atan2(player.y - y, player.x - x);

    for (int i = -6; i <= 6; i++) {
        double angle = center + i * 0.13;
        double speed = 2.05 + GetRand(70) / 100.0;

        HydraAddShot(
            pSet,
            x,
            y,
            angle,
            speed,
            img_enemyShotDiamond[0],
            HYDRA_ROLE_BULLET);
    }
}

static void HydraFireRegenBurst(sEnemyShotSet* pSet, double x, double y)
{
    for (int i = 0; i < 16; i++) {
        double angle = i * DX_PI * 2.0 / 16.0 + GetRand(10) / 100.0;
        double speed = 2.10 + GetRand(50) / 100.0;

        HydraAddShot(
            pSet,
            x,
            y,
            angle,
            speed,
            img_enemyShotScale[3],
            HYDRA_ROLE_BULLET);
    }
}

static void ShotHydra(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0 ||
        pEnemyShotSet->pEnemyShotHead->next == pEnemyShotSet->pEnemyShotHead) {
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;
        pEnemyShotSet->param_i[2] = 0;

        for (int i = 0; i < HYDRA_HEAD_NUM; i++) {
            sEnemyShot* pHead = HydraAddShot(
                pEnemyShotSet,
                enemy.x,
                enemy.y,
                DX_PI / 2.0,
                0.0,
                img_enemyShotMediumOval[2],
                HYDRA_ROLE_HEAD);

            pHead->margin = 120.0;

            pHead->param_i[1] = HYDRA_STATE_ALIVE;
            pHead->param_i[2] = 0;
            pHead->param_i[3] = 0;

            pHead->param_d[0] = (-112.5 + 25.0 * i) * DX_PI / 180.0;
            pHead->param_d[1] = GetRand(628) / 100.0;
            pHead->param_d[2] = 42.0 + GetRand(16);
        }
    }

    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    sEnemyShot* headSentinel = pEnemyShotSet->pEnemyShotHead;

    sEnemyShot* aliveHeads[HYDRA_HEAD_NUM];
    int aliveNum = 0;

    for (sEnemyShot* pShot = headSentinel->next; pShot != headSentinel; pShot = pShot->next) {
        if (pShot->param_i[0] == HYDRA_ROLE_HEAD) {
            HydraUpdateHeadPosition(pShot);

            if (pShot->param_i[1] == HYDRA_STATE_ALIVE) {
                if (aliveNum < HYDRA_HEAD_NUM) {
                    aliveHeads[aliveNum] = pShot;
                    aliveNum++;
                }
            }
        }
    }

    int consumedStump = 0;

    while (g_hydraPendingStump > 0 && aliveNum > 0) {
        int index = GetRand(aliveNum - 1);
        sEnemyShot* pHead = aliveHeads[index];

        pHead->param_i[1] = HYDRA_STATE_STUMP;
        pHead->param_i[2] = 30 + GetRand(30);
        pHead->param_i[3] = HYDRA_STUMP_TIME + GetRand(40);

        HydraFireSplash(pEnemyShotSet, pHead->x, pHead->y);

        aliveHeads[index] = aliveHeads[aliveNum - 1];
        aliveNum--;

        g_hydraPendingStump--;
        consumedStump++;
    }

    if (consumedStump > 0) {
        HydraPlaySound(sound_enemyShot_medium);
    }

    if (aliveNum == 0) {
        g_hydraPendingStump = 0;
    }

    int& fangTimer = pEnemyShotSet->param_i[0];
    bool telegraph = false;
    bool fireFang = false;

    if (aliveNum > 0) {
        fangTimer++;

        if (fangTimer == HYDRA_FANG_CHARGE) {
            HydraPlaySound(sound_enemyCharge);
        }

        if (fangTimer >= HYDRA_FANG_INTERVAL) {
            fangTimer = 0;
            fireFang = true;
            HydraPlaySound(sound_enemyShot_heavy);
        }
        else if (fangTimer >= HYDRA_FANG_CHARGE) {
            telegraph = true;
        }
    }
    else {
        fangTimer = 0;
    }

    bool fireNeedle = false;
    if (pEnemyShotSet->count > 0 && aliveNum > 0 && !telegraph) {
        if (pEnemyShotSet->count % 26 == 0) {
            fireNeedle = true;
            HydraPlaySound(sound_enemyShot_light);
        }
    }

    for (sEnemyShot* pShot = headSentinel->next; pShot != headSentinel; ) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == HYDRA_ROLE_HEAD) {
            if (pShot->param_i[1] == HYDRA_STATE_ALIVE) {
                if (telegraph) {
                    pShot->kind = img_enemyShotMediumOval[8];
                }
                else {
                    pShot->kind = img_enemyShotMediumOval[2];
                }

                if (fireNeedle) {
                    HydraFireNeedle(pEnemyShotSet, pShot);
                }

                if (fireFang) {
                    HydraFireFang(pEnemyShotSet, pShot);
                }
            }
            else if (pShot->param_i[1] == HYDRA_STATE_STUMP) {
                if (pShot->param_i[3] > 0) {
                    pShot->param_i[3]--;
                }

                if (pShot->param_i[3] <= 0) {
                    pShot->param_i[1] = HYDRA_STATE_ALIVE;
                    pShot->param_i[2] = 0;
                    pShot->param_i[3] = 0;
                    pShot->kind = img_enemyShotMediumOval[3];

                    HydraFireRegenBurst(pEnemyShotSet, pShot->x, pShot->y);
                    HydraPlaySound(sound_enemyShot_extreme);
                }
                else {
                    if (pShot->param_i[3] < 60) {
                        pShot->kind = img_enemyShotMediumBall[3];
                    }
                    else {
                        pShot->kind = img_enemyShotMediumBall[8];
                    }

                    if (pShot->param_i[2] > 0) {
                        pShot->param_i[2]--;
                    }

                    if (pShot->param_i[2] <= 0) {
                        HydraFireStumpPoison(pEnemyShotSet, pShot);
                        pShot->param_i[2] = 46 + GetRand(18);
                    }
                }
            }
        }

        pShot = pNext;
    }

    for (sEnemyShot* pShot = headSentinel->next; pShot != headSentinel; pShot = pShot->next) {
        if (pShot->param_i[0] == HYDRA_ROLE_HEAD) {
            continue;
        }

        if (pShot->param_i[1] == 1 && pShot->param_i[2] > 0 && pShot->count < pShot->param_i[2]) {
            double target = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = HydraNormalizeAngle(target - pShot->muki);
            double turn = pShot->param_d[0];

            if (diff > turn) {
                pShot->muki += turn;
            }
            else if (diff < -turn) {
                pShot->muki -= turn;
            }
            else {
                pShot->muki = target;
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
    }
}

void EnemyPat_Hydra_Qwen()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;

        g_hydraPrevHp = 200;
        g_hydraDamageAccum = 0;
        g_hydraPendingStump = 0;

        HydraCreateShotSet(ShotHydra, enemy.x, enemy.y);
    }
    else {
        enemy.x = 240.0 + sin(count * 0.017) * 110.0;
        enemy.y = 70.0 + sin(count * 0.031) * 22.0;
    }

    if (enemy.hp < g_hydraPrevHp) {
        g_hydraDamageAccum += g_hydraPrevHp - enemy.hp;
        g_hydraPrevHp = enemy.hp;

        while (g_hydraDamageAccum >= HYDRA_HP_PER_HEAD) {
            g_hydraDamageAccum -= HYDRA_HP_PER_HEAD;

            if (g_hydraPendingStump < HYDRA_HEAD_NUM) {
                g_hydraPendingStump++;
            }
        }
    }
    else {
        g_hydraPrevHp = enemy.hp;
    }
}