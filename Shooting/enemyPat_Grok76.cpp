// enemyPat_Tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 螺旋弾（時計回り / 反時計回り）
static void ShotSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int dir = pEnemyShotSet->param_i[0];      // 1:時計回り, -1:反時計回り
    const int color = pEnemyShotSet->param_i[1];    // 色
    const double baseAngle = pEnemyShotSet->param_d[0];

    if (pEnemyShotSet->count % 2 == 0 && pEnemyShotSet->count < 120) {
        if (pEnemyShotSet->count % 6 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = baseAngle + dir * (pEnemyShotSet->count * 0.18);
        pEnemyShot->speed = 2.4;
        pEnemyShot->kind = img_enemyShotMediumBall[color];
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

// 扇状弾＋直線弾の交差
static void ShotFanAndLine(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int color = pEnemyShotSet->param_i[0];
    const int isFan = pEnemyShotSet->param_i[1];    // 1:扇状, 0:直線

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        if (isFan) {
            // 広めの扇状
            for (int i = -4; i <= 4; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = pEnemyShotSet->muki + i * 0.18;
                pEnemyShot->speed = 2.8;
                pEnemyShot->kind = img_enemyShotScale[color];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        else {
            // 細い直線弾を複数本
            for (int i = -2; i <= 2; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = pEnemyShotSet->muki + i * 0.06;
                pEnemyShot->speed = 3.5;
                pEnemyShot->kind = img_enemyShotBullet[color];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 収縮→拡散の円形弾（両ボス同期用）
static void ShotContractExpand(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int color = pEnemyShotSet->param_i[0];

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count >= 20 && pEnemyShotSet->count <= 60 && pEnemyShotSet->count % 10 == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 円形に16発配置
        for (int i = 0; i < 16 * 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * (DX_PI * 2.0 / 16.0 / 2);
            pEnemyShot->speed = 0.0;                 // 最初は静止
            pEnemyShot->kind = img_enemyShotSmallBall[color];
            pEnemyShot->param_d[0] = 1.0;            // 後で速度変化用
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 20フレーム後に出現 → 30フレームかけて収縮 → その後拡散
        if (pShot->count < 30) {
            // 収縮：中心方向へゆっくり
            pShot->speed = -1.2;
        }
        else if (pShot->count == 30) {
            pShot->speed = 0.0;
        }
        else if (pShot->count < 50) {
            // 一時停止
            pShot->speed = 0.0;
        }
        else {
            // 拡散
            pShot->speed = 3.2;
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 縦レーザー（左右から寄せる）
static void ShotVerticalLaser(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int color = pEnemyShotSet->param_i[0];
    const double startX = pEnemyShotSet->param_d[0];
    const double moveDir = pEnemyShotSet->param_d[1];   // 正:右へ, 負:左へ

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 縦に並んだ短レーザーを複数本
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = startX;
            pEnemyShot->y = 40.0 + i * 55.0;
            pEnemyShot->muki = DX_PI / 2.0;          // 下向き（見た目用）
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[color];
            pEnemyShot->param_d[0] = moveDir;        // 横移動方向
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 徐々に中央へ寄せる
        pShot->x += pShot->param_d[0] * 0.9;
        if (abs(pShot->x - 240.0) < 10) pShot->margin = -9999;
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_TwoBoss_Grok()
{
    static int phase = 0;
    static int phaseTimer = 0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 140.0;
        enemy.y = 80.0;
        enemy.x2 = 340.0;
        enemy.y2 = 80.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        phaseTimer = 0;
    }
    else {
        // 左右にゆっくり揺れる
        enemy.x += 0.6 * sin(count * 0.03);
        enemy.x2 += 0.6 * sin(count * 0.03 + DX_PI);
        enemy.y = 80.0 + 8.0 * sin(count * 0.04);
        enemy.y2 = 80.0 + 8.0 * sin(count * 0.04 + 1.2);
    }

    phaseTimer++;

    // ===== フェーズ0：対向螺旋（約120フレーム） =====
    if (phase == 0) {
        if (phaseTimer == 1) {
            // 左ボス：時計回り・青
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSpiral;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 15.0;
            pSet->param_i[0] = 1;           // 時計回り
            pSet->param_i[1] = 4;           // 青
            pSet->param_d[0] = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;

            // 右ボス：反時計回り・赤
            pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSpiral;
            pSet->x = enemy.x2;
            pSet->y = enemy.y2 + 15.0;
            pSet->param_i[0] = -1;          // 反時計回り
            pSet->param_i[1] = 0;           // 赤
            pSet->param_d[0] = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        if (phaseTimer >= 130) {
            phase = 1;
            phaseTimer = 0;
        }
    }
    // ===== フェーズ1：扇状＋直線の交差（約100フレーム） =====
    else if (phase == 1) {
        if (phaseTimer % 10 == 1) {
            int side = (phaseTimer / 10) % 2;   // 交互に左右

            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotFanAndLine;
            if (side == 0) {
                pSet->x = enemy.x;
                pSet->y = enemy.y + 15.0;
                pSet->param_i[0] = 4;       // 青
                pSet->param_i[1] = 1;       // 扇状
            }
            else {
                pSet->x = enemy.x2;
                pSet->y = enemy.y2 + 15.0;
                pSet->param_i[0] = 0;       // 赤
                pSet->param_i[1] = 0;       // 直線
            }
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        if (phaseTimer >= 100) {
            phase = 2;
            phaseTimer = 0;
        }
    }
    // ===== フェーズ2：収縮→拡散の同期円形弾 =====
    else if (phase == 2) {
        if (phaseTimer == 1) {
            // 両ボス同時
            for (int i = 0; i < 2; i++) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotContractExpand;
                pSet->x = (i == 0) ? enemy.x : enemy.x2;
                pSet->y = (i == 0) ? enemy.y + 15.0 : enemy.y2 + 15.0;
                pSet->param_i[0] = (i == 0) ? 4 : 0;     // 青 / 赤
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
        if (phaseTimer >= 110) {
            phase = 3;
            phaseTimer = 0;
        }
    }
    // ===== フェーズ3：縦レーザーを左右から寄せる =====
    else if (phase == 3) {
        if (phaseTimer == 1) {
            // 左側から右へ寄せるレーザー
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotVerticalLaser;
            pSet->param_i[0] = 4;               // 青
            pSet->param_d[0] = 0.0;            // 開始X
            pSet->param_d[1] = 1.1;             // 右方向
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;

            // 右側から左へ寄せるレーザー
            pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotVerticalLaser;
            pSet->param_i[0] = 0;               // 赤
            pSet->param_d[0] = 480.0;           // 開始X
            pSet->param_d[1] = -1.1;            // 左方向
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        if (phaseTimer >= 90) {
            phase = 0;          // ループ
            phaseTimer = 0;
        }
    }
}