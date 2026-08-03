// enemyPat_tmp.cpp
// 弾幕名：断罪の輪廻 ― proliferating heads
// 首は敵弾（大玉）として sEnemyShotSet/sEnemyShot に登録し、メインルーチンが管理・描画する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  弾移動パターン：直線
// ------------------------------------------------------------
static void ShotHydra_Linear(sEnemyShotSet* p)
{
    sEnemyShot* s = p->pEnemyShotHead->next;
    while (s != p->pEnemyShotHead) {
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
        s = s->next;
    }
}

// ------------------------------------------------------------
//  弾移動パターン：蛇行
// ------------------------------------------------------------
static void ShotHydra_Wave(sEnemyShotSet* p)
{
    sEnemyShot* s = p->pEnemyShotHead->next;
    while (s != p->pEnemyShotHead) {
        s->muki += sin(s->count * 0.15 + s->param_i[0] * 2.0) * 0.03;
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
        s = s->next;
    }
}

// ------------------------------------------------------------
//  弾移動パターン：壁反射
// ------------------------------------------------------------
static void ShotHydra_Bounce(sEnemyShotSet* p)
{
    sEnemyShot* s = p->pEnemyShotHead->next;
    while (s != p->pEnemyShotHead) {
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);

        if (s->x < s->margin) {
            s->muki = DX_PI - s->muki;
            s->x = s->margin + 1.0;
        }
        else if (s->x > 480.0 - s->margin) {
            s->muki = DX_PI - s->muki;
            s->x = 480.0 - s->margin - 1.0;
        }
        if (s->y < s->margin) {
            s->muki = -s->muki;
            s->y = s->margin + 1.0;
        }
        //else if (s->y > 480.0 - s->margin) {
        //    s->muki = -s->muki;
        //    s->y = 480.0 - s->margin - 1.0;
        //}
        s = s->next;
    }
}

// ------------------------------------------------------------
//  弾生成ヘルパー
// ------------------------------------------------------------
static void AddBullet(double x, double y, double angle, double spd, int kind,
    void (*func)(sEnemyShotSet*), int phase)
{
    sEnemyShotSet* set = new sEnemyShotSet;
    set->count = 0;
    set->patternFunc = func;
    set->x = x;
    set->y = y;
    set->muki = angle;

    sEnemyShot* dummy = new sEnemyShot;
    dummy->prev = dummy;
    dummy->next = dummy;
    set->pEnemyShotHead = dummy;

    sEnemyShot* shot = new sEnemyShot;
    shot->x = x;
    shot->y = y;
    shot->muki = angle;
    shot->speed = spd;
    shot->kind = kind;
    shot->param_i[0] = phase;

    shot->prev = dummy->prev;
    shot->next = dummy;
    dummy->prev->next = shot;
    dummy->prev = shot;

    set->prev = enemyShotSetHead.prev;
    set->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = set;
    enemyShotSetHead.prev = set;
}

// ------------------------------------------------------------
//  首パターン（各首を敵弾として管理）
// ------------------------------------------------------------
static void HydraHeadPattern(sEnemyShotSet* p)
{
    int type = p->param_i[0];
    int idx = p->param_i[1];
    int birth = p->param_i[2];
    double baseAngle = p->param_d[0];

    // 首位置を敵本体に追従
    double ang = baseAngle + sin(count * 0.02 + idx) * 0.1;
    double hx = enemy.x + cos(ang) * 58.0;
    double hy = enemy.y + sin(ang) * 58.0;

    // 首自身の描画用弾の位置更新
    sEnemyShot* headShot = p->pEnemyShotHead->next;
    if (headShot != p->pEnemyShotHead) {
        headShot->x = hx;
        headShot->y = hy;
    }

    int hc = count - birth;

    // 火炎首：扇状3way大玉（赤）
    if (type == 0 && (hc == 0 || (hc % 120 == 0 && hc > 0))) {
        if (hc == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        for (int j = 0; j < 3; j++) {
            double a = baseAngle + (j - 1) * 30.0 / 180.0 * DX_PI;
            AddBullet(hx, hy, a, 1.5 + GetRand(50) / 100.0,
                img_enemyShotLargeBall[0], ShotHydra_Linear, 0);
        }
    }

    // 毒首：自機狙い高速鱗弾（マゼンタ）3連射・蛇行
    if (type == 1 && (hc == 0 || (hc % 100 == 0 && hc > 0))) {
        if (hc == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        for (int j = 0; j < 3; j++) {
            double a = atan2(player.y - hy, player.x - hx)
                + (GetRand(20) - 10) / 180.0 * DX_PI;
            AddBullet(hx, hy, a, 3.5 + GetRand(100) / 100.0,
                img_enemyShotScale[5], ShotHydra_Wave, j);
        }
    }

    // 氷首：ランダム方向中玉（シアン）5発・壁反射
    if (type == 2 && (hc == 0 || (hc % 130 == 0 && hc > 0))) {
        if (hc == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        for (int j = 0; j < 5; j++) {
            double a = baseAngle + (GetRand(180) - 90) / 180.0 * DX_PI;
            AddBullet(hx, hy, a, 2.0 + GetRand(100) / 100.0,
                img_enemyShotMediumBall[3], ShotHydra_Bounce, 0);
        }
    }
}

// ------------------------------------------------------------
//  敵本体：ヒュドラ
// ------------------------------------------------------------
void EnemyPat_Hydra_Kimi()
{
    static int headCount = 0;
    static int nextSpecial = 0;
    static int charged = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        headCount = 0;
        nextSpecial = 300;
        charged = 0;
    }
    else {
        enemy.x = 240.0 + sin(count * 0.02) * 60.0;
    }

    // 首生成：90フレームごとに1本、最大9本
    if (count % 90 == 30 && headCount < 9) {
        int i = headCount;
        int type = i % 3;
        double baseAngle = -DX_PI / 2.0 + (i - 4) * (DX_PI / 3.0) / 4.0;
        baseAngle += DX_PI;

        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = HydraHeadPattern;
        set->x = 0;
        set->y = 0;
        set->muki = 0;
        set->param_i[0] = type;
        set->param_i[1] = i;
        set->param_i[2] = count;
        set->param_i[3] = 0; // 九頭龍の陣発動済みフラグ
        set->param_d[0] = baseAngle;

        // ダミーヘッド
        sEnemyShot* dummy = new sEnemyShot;
        dummy->prev = dummy;
        dummy->next = dummy;
        set->pEnemyShotHead = dummy;

        // 首自身（敵弾として登録）
        sEnemyShot* head = new sEnemyShot;
        double ang = baseAngle + sin(count * 0.02 + i) * 0.1;
        head->x = enemy.x + cos(ang) * 58.0;
        head->y = enemy.y + sin(ang) * 58.0;
        head->muki = 0;
        head->speed = 0;
        head->margin = 1000; // 画面外判定で絶対に消えない
        switch (type) {
        case 0: head->kind = img_enemyShotLargeBall[0]; break; // 赤
        case 1: head->kind = img_enemyShotLargeBall[5]; break; // マゼンタ
        case 2: head->kind = img_enemyShotLargeBall[3]; break; // シアン
        }

        head->prev = dummy->prev;
        head->next = dummy;
        dummy->prev->next = head;
        dummy->prev = head;

        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;

        headCount++;
        if (headCount % 3 == 0 && headCount < 9) {
            nextSpecial = count + 300;
            charged = 0;
        }
    }

    // 九頭龍の陣 予告音（60フレーム前）
    if (count == nextSpecial - 60 && !charged && headCount >= 8) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        charged = 1;
    }

    // 九頭龍の陣 発動
    if (count == nextSpecial) {
        sEnemyShotSet* set = enemyShotSetHead.next;
        while (set != &enemyShotSetHead) {
            if (set->patternFunc == HydraHeadPattern && set->param_i[3] == 0) {
                int type = set->param_i[0];
                sEnemyShot* headShot = set->pEnemyShotHead->next;
                double hx = headShot->x;
                double hy = headShot->y;

                double baseAngle = set->param_d[0];
                if (type == 0) {
                    for (int j = 0; j < 10; j++) {
                        double a = baseAngle + j * DX_PI * 2.0 / 10.0;
                        AddBullet(hx, hy, a, 3.5, img_enemyShotLargeBall[8], ShotHydra_Linear, 0);
                    }
                }
                else if (type == 1) {
                    for (int j = 0; j < 20; j++) {
                        double a = baseAngle + j * DX_PI * 2.0 / 20.0;
                        AddBullet(hx, hy, a, 5.0, img_enemyShotMediumOval[5], ShotHydra_Linear, 0);
                    }
                }
                else if (type == 2) {
                    for (int j = 0; j < 30; j++) {
                        double a = baseAngle + j * DX_PI * 2.0 / 30.0;
                        AddBullet(hx, hy, a, 2.5, img_enemyShotDiamond[3], ShotHydra_Linear, 0);
                    }
                }
                set->param_i[3] = 1;
            }
            set = set->next;
        }

        if (headCount >= 9) {
            nextSpecial = count + 300;
            charged = 0;
            // 次回発動用にフラグをリセット
            sEnemyShotSet* s = enemyShotSetHead.next;
            while (s != &enemyShotSetHead) {
                if (s->patternFunc == HydraHeadPattern) {
                    s->param_i[3] = 0;
                }
                s = s->next;
            }
        }
    }
}