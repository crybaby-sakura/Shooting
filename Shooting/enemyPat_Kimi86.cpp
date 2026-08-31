// enemyPat_spiralFang_hard.cpp
// 弾幕：螺旋の牙（高難易度版）
// ボスが連続突進し、高密度螺旋弾と追従衝撃波、待機中の自機狙い弾で隙を作らない

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const double CHARGE_SPEED = 7.0;   // 突進速度（高速）
static const int    SPIRAL_SHOTS = 32;    // 螺旋弾総数（高密度）
static const int    SPIRAL_INTERVAL = 2;     // 螺旋弾発射間隔（2F）
static const int    SHOCKWAVE_COUNT = 24;    // 衝撃波弾数（全方位高密度）
static const double SHOCKWAVE_SPEED = 3.0;   // 衝撃波速度（短射程だが高速）
static const double RETREAT_SPEED = 4.0;   // 離脱速度
static const int    CHARGE_COUNT = 2;     // 連続突進回数
static const double TRACK_RATE = 0.008;  // 突進方向の自機追従率

// ============================================================
//  弾幕パターン：螺旋の牙（高難易度）
// ============================================================
static void ShotSpiralFang(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // param_i[0] : 螺旋弾発射済み数
        // param_i[1] : 衝撃波発射済みフラグ
        // param_i[2] : 突進開始時のグローバルcount
        // param_i[3] : 現在の連続突進回数（1〜CHARGE_COUNT）
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;
        pEnemyShotSet->param_i[2] = count;
        pEnemyShotSet->param_i[3] = 1;

        // param_d[0] : 突進開始X
        // param_d[1] : 突進開始Y
        // param_d[2] : 突進方向（毎フレーム追従補正される）
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
        pEnemyShotSet->param_d[2] = pEnemyShotSet->muki;
    }

    int elapsed = count - pEnemyShotSet->param_i[2];

    // ボスの仮想位置
    double virtualBossX = pEnemyShotSet->param_d[0] + elapsed * CHARGE_SPEED * cos(pEnemyShotSet->param_d[2]);
    double virtualBossY = pEnemyShotSet->param_d[1] + elapsed * CHARGE_SPEED * sin(pEnemyShotSet->param_d[2]);

    // 突進方向を自機方向に少しずつ追従補正
    double toPlayer = atan2(player.y - virtualBossY, player.x - virtualBossX);
    double diff = toPlayer - pEnemyShotSet->param_d[2];
    while (diff > DX_PI) diff -= DX_PI * 2.0;
    while (diff < -DX_PI) diff += DX_PI * 2.0;
    pEnemyShotSet->param_d[2] += diff * TRACK_RATE;

    // --------------------------------------------------------
    //  螺旋弾発射（高密度：32発、2F間隔）
    // --------------------------------------------------------
    if (elapsed < 64 &&
        elapsed % SPIRAL_INTERVAL == 0 &&
        pEnemyShotSet->param_i[0] < SPIRAL_SHOTS) {

        pEnemyShot = new sEnemyShot;

        pEnemyShot->x = virtualBossX;
        pEnemyShot->y = virtualBossY;

        // 螺旋角度：60°ずつずらした渦巻き（より複雑な軌道）
        double spiralOffset = pEnemyShotSet->param_i[0] * (DX_PI / 3.0);
        pEnemyShot->muki = pEnemyShotSet->param_d[2] + spiralOffset;
        pEnemyShot->speed = 3.5;

        // 小玉、黒色（7:黒）— 背景に紛れやすく視認性低下
        pEnemyShot->kind = img_enemyShotSmallBall[7];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        pEnemyShotSet->param_i[0]++;

        if (pEnemyShotSet->param_i[0] == 1) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // --------------------------------------------------------
    //  接近時の衝撃波（25F目、大玉・高密度・高速）
    // --------------------------------------------------------
    if (elapsed == 25 && pEnemyShotSet->param_i[1] == 0) {
        pEnemyShotSet->param_i[1] = 1;

        for (int i = 0; i < SHOCKWAVE_COUNT; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = virtualBossX;
            pEnemyShot->y = virtualBossY;
            pEnemyShot->muki = i * (DX_PI * 2.0 / SHOCKWAVE_COUNT);
            pEnemyShot->speed = SHOCKWAVE_SPEED;

            // 大玉(20.0x20.0)、白色（6:白）— 巨大で圧迫感
            pEnemyShot->kind = img_enemyShotLargeBall[6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    //  弾の移動
    // --------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
//  待機中の自機狙い弾パターン
// ============================================================
static void ShotAimDuringWait(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (count % 10 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        pEnemyShot = new sEnemyShot;

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 3.0;

        // 銃弾(5.0x2.0)、青色（4:青）— 細くて避けにくい
        pEnemyShot->kind = img_enemyShotBullet[4];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
//  離脱中の後方拡散弾パターン
// ============================================================
static void ShotRetreatScatter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(120) - 60) / 180.0 * DX_PI;
            pEnemyShot->speed = (150 + GetRand(150)) / 100.0;

            // 鱗弾(4.0x3.0)、マゼンタ（5:マゼンタ）
            pEnemyShot->kind = img_enemyShotScale[5];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン：螺旋の牙（高難易度）
// ============================================================
void EnemyPat_CloseCombat_Kimi()
{
    enum {
        PHASE_WAIT = 0,
        PHASE_PRE = 1,
        PHASE_CHARGE = 2,
        PHASE_RETREAT = 3,
    };

    static int    phase = PHASE_WAIT;
    static int    phaseTimer = 0;
    static double chargeDir = 0.0;
    static int    chargeNum = 0;      // 現在の連続突進回数

    const int WAIT_TIME = 90;
    const int PRE_TIME = 30;     // 予兆短縮
    const int CHARGE_TIME = 50;     // 突進時間短縮（高速化に伴い）
    const int RETREAT_TIME = 60;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = PHASE_WAIT;
        phaseTimer = 0;
        chargeNum = 0;
    }

    phaseTimer++;

    switch (phase) {
        // ========================================================
    case PHASE_WAIT:
        // ========================================================
    {
        enemy.x = 240.0 + sin(count * 0.05) * 40.0;
        enemy.y = 60.0 + sin(count * 0.08) * 10.0;

        // 待機中も自機狙いの弾を2F間隔で発射（隙を作らない）
        if (count % 2 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotAimDuringWait;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        if (phaseTimer >= WAIT_TIME) {
            phase = PHASE_PRE;
            phaseTimer = 0;
            chargeNum = 0;
        }
    }
    break;

    // ========================================================
    case PHASE_PRE:
        // ========================================================
    {
        chargeDir = atan2(player.y - enemy.y, player.x - enemy.x);

        // 予兆中の震えを強化
        enemy.x += (GetRand(6) - 3) * 0.5;
        enemy.y += (GetRand(6) - 3) * 0.5;

        if (phaseTimer >= PRE_TIME) {
            phase = PHASE_CHARGE;
            phaseTimer = 0;
            chargeNum++;

            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSpiralFang;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = chargeDir;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    break;

    // ========================================================
    case PHASE_CHARGE:
        // ========================================================
    {
        enemy.x += CHARGE_SPEED * cos(chargeDir);
        enemy.y += CHARGE_SPEED * sin(chargeDir);

        // 画面端または時間経過で離脱
        if (enemy.x < 10.0 || enemy.x > 470.0 || enemy.y < 10.0 || enemy.y > 470.0 ||
            phaseTimer >= CHARGE_TIME) {

            if (chargeNum < CHARGE_COUNT) {
                // 連続突進：短い予兆後に再突進
                if (enemy.x < 10.0) enemy.x = 20.0;
                if (enemy.x > 470.0) enemy.x = 460.0;
                if (enemy.y < 10.0) enemy.y = 20.0;
                if (enemy.y > 470.0) enemy.x = 460.0;
                phase = PHASE_PRE;
                phaseTimer = 0;
            }
            else {
                phase = PHASE_RETREAT;
                phaseTimer = 0;

                // 離脱開始時に後方拡散弾
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotRetreatScatter;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->muki = chargeDir + DX_PI; // 後方

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
    }
    break;

    // ========================================================
    case PHASE_RETREAT:
        // ========================================================
    {
        double targetX = 240.0;
        double targetY = 60.0;
        double dx = targetX - enemy.x;
        double dy = targetY - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist > 3.0) {
            enemy.x += (dx / dist) * RETREAT_SPEED;
            enemy.y += (dy / dist) * RETREAT_SPEED;
        }
        else {
            enemy.x = targetX;
            enemy.y = targetY;
        }

        if (phaseTimer >= RETREAT_TIME || dist <= 3.0) {
            phase = PHASE_WAIT;
            phaseTimer = 0;
        }
    }
    break;
    }
}