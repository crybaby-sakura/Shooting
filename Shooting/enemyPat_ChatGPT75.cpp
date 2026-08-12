// enemyPat_tmp.cpp
// 弾幕：星喰む五芒星
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const double STAR_PI2 = DX_PI * 2.0;
static const double STAR_R_OUT = 145.0 * 2;
static const double STAR_R_IN = 55.0 * 2;
static const int STAR_SEGMENTS = 10;
static const int STAR_BULLETS_PER_SEGMENT = 8 * 3;

static void GetStarNode(double cx, double cy, double angle, int node,
    double* x, double* y)
{
    static const int outerIndex[10] = { 0,-1,1,-1,2,-1,3,-1,4,-1 };
    static const int innerIndex[10] = { -1,2,-1,3,-1,4,-1,0,-1,1 };
    node %= STAR_SEGMENTS;
    const bool outer = outerIndex[node] >= 0;
    const int index = outer ? outerIndex[node] : innerIndex[node];
    const double radius = outer ? STAR_R_OUT : STAR_R_IN;
    const double a = angle - DX_PI / 2.0 + STAR_PI2 * index / 5.0;
    *x = cx + cos(a) * radius;
    *y = cy + sin(a) * radius;
}

static void UpdateStarBullet(sEnemyShot* shot, sEnemyShotSet* set)
{
    const double x0 = shot->param_d[0];
    const double y0 = shot->param_d[1];
    const double x1 = shot->param_d[2];
    const double y1 = shot->param_d[3];
    const double dx = x1 - x0;
    const double dy = y1 - y0;
    const double length = sqrt(dx * dx + dy * dy);
    if (length <= 0.0) {
        shot->x += shot->speed * cos(shot->muki);
        shot->y += shot->speed * sin(shot->muki);
        return;
    }

    const double t = fmod(shot->count * shot->speed + shot->param_d[4] * 600, length * 2.0);
    const double d = (t <= length) ? t : length * 2.0 - t;
    const double ratio = d / length;
    shot->x = set->x - 240 + x0 + dx * ratio;
    shot->y = set->y - (65 + 175) + y0 + dy * ratio;
    shot->muki = atan2(dy, dx);
}

static void ShotPentagram(sEnemyShotSet* set)
{
    const double angle = set->param_d[0];

    // 星の中心を敵に追従させる。
    set->x = enemy.x;
    set->y = enemy.y + 175.0;

    // 五芒星10辺を弾列で描く。
    if (set->count == 0) {
        for (int seg = 0; seg < STAR_SEGMENTS; seg++) {
            double x0, y0, x1, y1;
            GetStarNode(set->x, set->y, angle, seg, &x0, &y0);
            GetStarNode(set->x, set->y, angle, seg + 1, &x1, &y1);

            const double dx = x1 - x0;
            const double dy = y1 - y0;
            const double length = sqrt(dx * dx + dy * dy);

            for (int j = 0; j < STAR_BULLETS_PER_SEGMENT; j++) {
                sEnemyShot* shot = new sEnemyShot;
                const double ratio = (double)j / STAR_BULLETS_PER_SEGMENT;

                shot->param_d[0] = x0;
                shot->param_d[1] = y0;
                shot->param_d[2] = x1;
                shot->param_d[3] = y1;
                shot->param_d[4] = ratio;
                shot->x = x0 + dx * ratio;
                shot->y = y0 + dy * ratio;
                shot->muki = atan2(dy, dx);
                shot->speed = 1.7;
                shot->kind = img_enemyShotMediumBall[(seg + 3) % 9];
                shot->margin = 240;

                shot->prev = set->pEnemyShotHead->prev;
                shot->next = set->pEnemyShotHead;
                set->pEnemyShotHead->prev->next = shot;
                set->pEnemyShotHead->prev = shot;
            }
        }

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* shot = set->pEnemyShotHead->next;
    while (shot != set->pEnemyShotHead) {
        UpdateStarBullet(shot, set);
        shot = shot->next;
    }

    // 5頂点から中央へ。
    if (set->count % 90 == 30) {
        for (int i = 0; i < 5; i++) {
            double x, y;
            GetStarNode(set->x, set->y, angle, i * 2, &x, &y);
            sEnemyShot* shot = new sEnemyShot;
            shot->x = x;
            shot->y = y;
            shot->muki = atan2(set->y - y, set->x - x);
            shot->speed = 1.35;
            shot->kind = img_enemyShotMediumOval[(i + 8) % 9];
            shot->prev = set->pEnemyShotHead->prev;
            shot->next = set->pEnemyShotHead;
            set->pEnemyShotHead->prev->next = shot;
            set->pEnemyShotHead->prev = shot;
        }
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 中央から外へ5方向。
    if (set->count % 160 == 100) {
        for (int i = 0; i < 5; i++) {
            const double a = angle + DX_PI / 5.0 + STAR_PI2 * i / 5.0;
            sEnemyShot* shot = new sEnemyShot;
            shot->x = set->x;
            shot->y = set->y;
            shot->muki = a;
            shot->speed = 1.9;
            shot->kind = img_enemyShotDiamond[(i + 2) % 9];
            shot->prev = set->pEnemyShotHead->prev;
            shot->next = set->pEnemyShotHead;
            set->pEnemyShotHead->prev->next = shot;
            set->pEnemyShotHead->prev = shot;
        }
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    set->param_d[0] += set->param_d[1];

    if (set->count > 0 && set->count % 240 == 0) {
        set->param_d[1] *= -1.0;
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
}

void EnemyPat_Pentagram_ChatGPT()
{
    static int moveDir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 65.0;
        enemy.maxHp = enemy.hp = 200;
        player.y = 240;
        moveDir = 1;
    }
    else {
        enemy.x += 0.70 * (double)moveDir;
        if (enemy.x < 120.0) {
            enemy.x = 120.0;
            moveDir = 1;
        }
        else if (enemy.x > 360.0) {
            enemy.x = 360.0;
            moveDir = -1;
        }
    }

    if (count == 1) {
        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = ShotPentagram;
        set->x = enemy.x;
        set->y = enemy.y + 125.0;
        set->param_d[0] = 0.0;
        set->param_d[1] = STAR_PI2 / 720.0;

        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }
}