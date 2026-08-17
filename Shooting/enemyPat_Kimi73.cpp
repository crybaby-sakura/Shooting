// enemyPat_Tmp.cpp
// 弾幕パターン：残響の鏡陣（Echo Mirror Formation）
// ボスが画面内を瞬間移動し、移動地点に残像を残す。
// 残像は待機後に扇状弾→収束弾＋拡散弾を発射し、
// 最後に本体が中央で十字レーザーを放つ。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------
// 残像の見た目：大玉・シアンを配置し、120フレーム後に上方向へ高速で飛ばして消す
// ---------------------------------------------------------
static void ShotEchoMirror_Decoy(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* p;

    if (pEnemyShotSet->count == 0) {
        p = new sEnemyShot;
        p->x = pEnemyShotSet->x;
        p->y = pEnemyShotSet->y;
        p->muki = -DX_PI / 2.0;
        p->speed = 0.0;
        p->kind = img_enemyShotLargeBall[3]; // 大玉・シアン
        p->prev = pEnemyShotSet->pEnemyShotHead->prev;
        p->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = p;
        pEnemyShotSet->pEnemyShotHead->prev = p;
    }
    else if (pEnemyShotSet->count == 120) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->speed = -20.0;
            pShot = pShot->next;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 残像からの攻撃：30フレーム目に扇状弾、90フレーム目に収束弾＋拡散弾
// ---------------------------------------------------------
static void ShotEchoMirror_Attack(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* p;

    if (pEnemyShotSet->count == 30) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = 0; i < 10; i++) {
            p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = baseAngle + (i - 4.5) * (DX_PI / 180.0 * 12.0);
            p->speed = 2.5;
            p->kind = img_enemyShotSmallBall[4]; // 小玉・青
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }
    else if (pEnemyShotSet->count == 90) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double toBoss = atan2(enemy.y - pEnemyShotSet->y, enemy.x - pEnemyShotSet->x);
        for (int i = 0; i < 2; i++) {
            p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = toBoss + (i == 0 ? -0.05 : 0.05);
            p->speed = 4.0 + i * 0.5;
            p->kind = img_enemyShotMediumBall[6]; // 中玉・白
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }

        for (int i = 0; i < 8 * 5; i++) {
            p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = DX_PI * 2.0 / 8.0 / 5 * i + (GetRand(10) - 5) / 180.0 * DX_PI;
            p->speed = 1.0 + GetRand(100) / 100.0;
            p->kind = img_enemyShotScale[5]; // 鱗弾・マゼンタ
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 十字レーザー：4方向（右・下・左・上）へ高速で伸びる短レーザー
// ---------------------------------------------------------
static void ShotEchoMirror_CrossLaser(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* p;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double dirs[4] = { 0.0, DX_PI / 2.0, DX_PI, -DX_PI / 2.0 };
        for (int i = 0; i < 4; i++) {
            p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = dirs[i];
            p->speed = 6.0;
            p->kind = img_enemyShotLaser[0]; // 短レーザー・赤
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 敵本体パターン：残響の鏡陣
// ---------------------------------------------------------
void EnemyPat_Warp_Kimi()
{
    static int phase;
    static int teleportCount;
    static int maxTeleport;
    static int cycleStart;
    static double decoyX[5];
    static double decoyY[5];

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        teleportCount = 0;
        maxTeleport = 5; // GetRand(2) は0〜2なので、3〜5箇所
        cycleStart = count;
    }

    int t = count - cycleStart;

    // フェーズ0：陣形展開（瞬間移動＋残像配置）
    if (phase == 0) {
        if (t == 60) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        if (t >= 60 && t <= 180 && (t - 60) % 30 == 0 && teleportCount < maxTeleport) {
            // 現在位置に残像を生成
            sEnemyShotSet* pDecoy = new sEnemyShotSet;
            pDecoy->count = 0;
            pDecoy->patternFunc = ShotEchoMirror_Decoy;
            pDecoy->x = enemy.x;
            pDecoy->y = enemy.y;
            pDecoy->muki = 0.0;
            pDecoy->kind = 0;
            pDecoy->pEnemyShotHead = new sEnemyShot;
            pDecoy->pEnemyShotHead->prev = pDecoy->pEnemyShotHead;
            pDecoy->pEnemyShotHead->next = pDecoy->pEnemyShotHead;
            pDecoy->prev = enemyShotSetHead.prev;
            pDecoy->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pDecoy;
            enemyShotSetHead.prev = pDecoy;

            decoyX[teleportCount] = enemy.x;
            decoyY[teleportCount] = enemy.y;

            // 新しい位置へ瞬間移動
            enemy.x = 80.0 + GetRand(320);
            enemy.y = 60.0 + GetRand(200);
            teleportCount++;

            if (teleportCount >= maxTeleport) {
                phase = 1;
            }
        }
    }
    // フェーズ1：全残像からの攻撃を同時開始
    else if (phase == 1) {
        if (t >= 210) {
            for (int i = 0; i < maxTeleport; i++) {
                sEnemyShotSet* pAtk = new sEnemyShotSet;
                pAtk->count = 0;
                pAtk->patternFunc = ShotEchoMirror_Attack;
                pAtk->x = decoyX[i];
                pAtk->y = decoyY[i];
                pAtk->muki = 0.0;
                pAtk->kind = 0;
                pAtk->pEnemyShotHead = new sEnemyShot;
                pAtk->pEnemyShotHead->prev = pAtk->pEnemyShotHead;
                pAtk->pEnemyShotHead->next = pAtk->pEnemyShotHead;
                pAtk->prev = enemyShotSetHead.prev;
                pAtk->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pAtk;
                enemyShotSetHead.prev = pAtk;
            }
            phase = 2;
        }
    }
    // フェーズ2：攻撃待機、本体を中央へ瞬間移動
    else if (phase == 2) {
        if (t >= 270) {
            enemy.x = 240.0;
            enemy.y = 240.0;
            phase = 3;
        }
    }
    // フェーズ3：十字レーザー発射
    else if (phase == 3) {
        if (t >= 330) {
            sEnemyShotSet* pLaser = new sEnemyShotSet;
            pLaser->count = 0;
            pLaser->patternFunc = ShotEchoMirror_CrossLaser;
            pLaser->x = enemy.x;
            pLaser->y = enemy.y;
            pLaser->muki = 0.0;
            pLaser->kind = 0;
            pLaser->pEnemyShotHead = new sEnemyShot;
            pLaser->pEnemyShotHead->prev = pLaser->pEnemyShotHead;
            pLaser->pEnemyShotHead->next = pLaser->pEnemyShotHead;
            pLaser->prev = enemyShotSetHead.prev;
            pLaser->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pLaser;
            enemyShotSetHead.prev = pLaser;

            phase = 4;
        }
    }
    // フェーズ4：終了、次のサイクルへ
    else if (phase == 4) {
        if (t >= 500) {
            cycleStart = count;
            phase = 0;
            teleportCount = 0;
            maxTeleport = 5;
            enemy.x = 240.0;
            enemy.y = 80.0;
        }
    }
}