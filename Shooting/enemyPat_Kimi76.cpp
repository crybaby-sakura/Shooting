// enemyPat_tmp.cpp
// 双星の共鳴軌道 — Resonant Binary
// 2体のボスが左右に配置され、フェーズに応じて異なる弾幕を展開する。
// フェーズ1：単独射出（左=赤誘導弾、右=青直進弾）
// フェーズ2：共鳴交差（赤・青の共鳴弾が接触すると紫拡散弾を生成）
// フェーズ3：軌道交換（左右が入れ替わり、共鳴弾の色も逆転）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 前方宣言
// ------------------------------------------------------------
static void ShotRedHoming(sEnemyShotSet* pSet);
static void ShotBlueStraight(sEnemyShotSet* pSet);
static void ShotResonanceRed(sEnemyShotSet* pSet);
static void ShotResonanceBlue(sEnemyShotSet* pSet);
static void ShotPurpleSpread(sEnemyShotSet* pSet);

// ------------------------------------------------------------
// フェーズ1：赤誘導弾（左ボス / フェーズ3では右ボス）
// プレイヤー方向へゆるやかに向きを補正する
// ------------------------------------------------------------
static void ShotRedHoming(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    if (pSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = pSet->muki;
        pShot->speed = 2.0;
        pShot->kind = img_enemyShotSmallBall[0]; // 赤 小玉
        pShot->param_i[0] = 0;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        // 6フレーム毎にプレイヤー方向へゆるやかに補正
        if (p->count % 6 == 0) {
            double targetMuki = atan2(player.y - p->y, player.x - p->x);
            double diff = targetMuki - p->muki;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            p->muki += diff * 0.06;
        }
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
// フェーズ1：青直進弾（右ボス / フェーズ3では左ボス）
// プレイヤー方向を狙った高速直進弾
// ------------------------------------------------------------
static void ShotBlueStraight(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    if (pSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = pSet->muki;
        pShot->speed = 3.5;
        pShot->kind = img_enemyShotMediumBall[4]; // 青 中玉
        pShot->param_i[0] = 1;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
// フェーズ2：赤共鳴弾
// 青共鳴弾との接触判定を行い、接触時に紫拡散弾を生成する
// ------------------------------------------------------------
static void ShotResonanceRed(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    if (pSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        // プレイヤー方向から少しランダムに外す（共鳴地点を中央付近に誘導）
        pShot->muki = pSet->muki + (GetRand(24) - 12) / 180.0 * DX_PI;
        pShot->speed = 2.2;
        pShot->kind = img_enemyShotMediumBall[0]; // 赤 中玉
        pShot->param_i[0] = 10; // 赤共鳴弾識別子

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    // 移動
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }

    // 共鳴判定（発射から10フレーム後から行う）
    if (pSet->count >= 10) {
        sEnemyShot* pMy = pSet->pEnemyShotHead->next;
        if (pMy != pSet->pEnemyShotHead && pMy->x > -1000.0) {
            sEnemyShotSet* pOtherSet = enemyShotSetHead.next;
            while (pOtherSet != &enemyShotSetHead) {
                if (pOtherSet->patternFunc == ShotResonanceBlue) {
                    sEnemyShot* pOther = pOtherSet->pEnemyShotHead->next;
                    while (pOther != pOtherSet->pEnemyShotHead) {
                        double dx = pMy->x - pOther->x;
                        double dy = pMy->y - pOther->y;
                        // 距離22px以内で共鳴発生
                        if (dx * dx + dy * dy < 22.0 * 22.0) {
                            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                            // 拡散弾セット生成
                            sEnemyShotSet* pSpread = new sEnemyShotSet;
                            pSpread->count = 0;
                            pSpread->patternFunc = ShotPurpleSpread;
                            pSpread->x = (pMy->x + pOther->x) * 0.5;
                            pSpread->y = (pMy->y + pOther->y) * 0.5;
                            pSpread->muki = 0.0;
                            pSpread->pEnemyShotHead = new sEnemyShot;
                            pSpread->pEnemyShotHead->prev = pSpread->pEnemyShotHead;
                            pSpread->pEnemyShotHead->next = pSpread->pEnemyShotHead;

                            pSpread->prev = enemyShotSetHead.prev;
                            pSpread->next = &enemyShotSetHead;
                            enemyShotSetHead.prev->next = pSpread;
                            enemyShotSetHead.prev = pSpread;

                            // 両弾を画面外に飛ばしてメインルーチンの消去処理に委任
                            pMy->x = -9999.0;
                            pOther->x = -9999.0;
                            break;
                        }
                        pOther = pOther->next;
                    }
                }
                if (pMy->x < -1000.0) break; // 既に共鳴処理済み
                pOtherSet = pOtherSet->next;
            }
        }
    }
}

// ------------------------------------------------------------
// フェーズ2：青共鳴弾
// 移動のみ。共鳴判定は赤共鳴弾側で一元管理（二重生成防止）
// ------------------------------------------------------------
static void ShotResonanceBlue(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    if (pSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = pSet->muki + (GetRand(24) - 12) / 180.0 * DX_PI;
        pShot->speed = 2.2;
        pShot->kind = img_enemyShotMediumBall[4]; // 青 中玉
        pShot->param_i[0] = 11; // 青共鳴弾識別子

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
// 共鳴発生時：紫拡散弾
// 中心に大玉を表示し、2フレーム後に8方向へ分裂
// ------------------------------------------------------------
static void ShotPurpleSpread(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    if (pSet->count == 0) {
        // 中心に大玉（演出用、2フレーム後に消去）
        pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->muki = 0.0;
        pShot->speed = 0.0;
        pShot->kind = img_enemyShotLargeBall[5]; // マゼンタ 大玉
        pShot->param_i[0] = 0; // 未分裂

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }
    else if (pSet->count == 2) {
        // 2フレーム後に8方向へ分裂
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 0) {
                p->param_i[0] = 1; // 分裂済みマーク
                for (int i = 0; i < 8; i++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = p->x;
                    pNew->y = p->y;
                    pNew->muki = i * DX_PI / 4.0;
                    pNew->speed = 2.5 + GetRand(10) / 10.0; // 2.5～3.5
                    pNew->kind = img_enemyShotSmallBall[5]; // マゼンタ 小玉
                    pNew->param_i[0] = 2;

                    pNew->prev = pSet->pEnemyShotHead->prev;
                    pNew->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pNew;
                    pSet->pEnemyShotHead->prev = pNew;
                }
                // 中心大玉は画面外に飛ばして消去予約
                p->x = -9999.0;
            }
            p = p->next;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン：双星の共鳴軌道
// ------------------------------------------------------------
void EnemyPat_TwoBoss_Kimi()
{
    static int muki = 1;
    static bool swapped = false;

    // --- 初期化 ---
    if (count == 1) {
        enemy.x = 80.0 + 20;
        enemy.y = 60.0;
        enemy.x2 = 400.0 - 20;
        enemy.y2 = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        swapped = false;
    }
    else {
        // --- 敵移動（左右にゆるやかに揺れる）---
        enemy.x += 0.7 * (double)muki;
        enemy.x2 += 0.7 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // --- フェーズ管理（1200フレーム周期）---
    int cycle = count % 1200;
    int phase;
    if (cycle < 400) {
        phase = 0; // フェーズ1：単独射出
    }
    else if (cycle < 800) {
        phase = 1; // フェーズ2：共鳴交差
    }
    else {
        phase = 2; // フェーズ3：軌道交換＋共鳴
    }

    // --- 軌道交換（フェーズ3開始時、1回だけ）---
    if (phase == 2 && !swapped) {
        double tmpX = enemy.x;
        enemy.x = enemy.x2;
        enemy.x2 = tmpX;
        double tmpY = enemy.y;
        enemy.y = enemy.y2;
        enemy.y2 = tmpY;
        swapped = true;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (phase == 0) swapped = false; // 次の周期でリセット

    // --- 弾生成 ---
    int interval = (phase == 0) ? 2 : 2;
    if (count % interval == 1) {
        bool isLeft = (count % (interval * 2) == 1);
        double ex = isLeft ? enemy.x : enemy.x2;
        double ey = isLeft ? enemy.y : enemy.y2;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->x = ex;
        pSet->y = ey + 15.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // フェーズと左右によって発射パターンを決定
        if (phase == 0) {
            // フェーズ1：左=赤誘導、右=青直進
            pSet->patternFunc = isLeft ? ShotRedHoming : ShotBlueStraight;
        }
        else if (phase == 1) {
            // フェーズ2：左=赤共鳴、右=青共鳴
            pSet->patternFunc = isLeft ? ShotResonanceRed : ShotResonanceBlue;
        }
        else {
            // フェーズ3：入れ替わっているので左=青共鳴、右=赤共鳴
            pSet->patternFunc = isLeft ? ShotResonanceBlue : ShotResonanceRed;
        }

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}