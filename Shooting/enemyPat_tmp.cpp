// enemyPat_lagCascade.cpp
// 処理落ちモチーフ弾幕「フレームドロップ・カスケード」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ============================================================
//  定数定義
// ============================================================
static const int PHASE1_WAY = 24;        // 第1段：24way大弾
static const int PHASE1_SPEED = 180 * 3;       // 大弾速度（100分の1単位：1.80）
static const int PHASE1_UPDATE = 6;         // 大弾の位置更新間隔（6フレームに1回）
static const int PHASE2_SPLIT = 3;         // 大弾→小弾の分裂数
static const int PHASE2_SPEED = 150;       // 小弾速度（1.50）
static const int PHASE2_UPDATE = 6;         // 小弾も6フレーム更新
static const int PHASE3_SPLIT = 3;         // 小弾→微弾の分裂数
static const int PHASE3_SPEED = 120;       // 微弾速度（1.20）
static const int PHASE3_UPDATE = 6;         // 微弾も6フレーム更新
static const int LAG_THRESHOLD = 150;       // ラグ発生弾数閾値
static const int LAG_DURATION = 90;        // ラグ停止フレーム数（1.5秒@60fps）
static const double RECOVER_MULT = 1.8;       // 回復後の速度倍率
static const int MARGIN = 0;        // 画面外判定マージン（大きめに取る）

// ============================================================
//  弾の種類・色のエイリアス（可読性のため）
// ============================================================
static const int KIND_LARGE = 0;  // 中玉 赤
static const int KIND_MID = 1;  // 小玉 黄
static const int KIND_SMALL = 3;  // 銃弾 緑
static const int COLOR_RED = 0;
static const int COLOR_YEL = 1;
static const int COLOR_GRN = 2;

// ============================================================
//  弾の生成ヘルパー
// ============================================================
static sEnemyShot* CreateShot(
    sEnemyShotSet* pSet,
    double x, double y,
    double muki, double speed,
    int kindBase, int color,
    int updateInterval,
    int splitGen = 0
)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed / 100.0;
    // count のインクリメントはメインルーチンが自動で行う
    // 分裂直後に即座に動かないよう、初期値を 1 に設定
    p->count = 1;

    // 色を反映（8色周期で種類に応じた画像を選択）
    int imgArray[8] = {
        img_enemyShotSmallBall[color],
        img_enemyShotMediumBall[color],
        img_enemyShotLargeBall[color],
        img_enemyShotBullet[color],
        img_enemyShotScale[color],
        img_enemyShotDiamond[color],
        img_enemyShotMediumOval[color],
        img_enemyShotLaser[color]
    };
    switch (kindBase) {
    case 0: p->kind = imgArray[1]; break;  // 中玉
    case 1: p->kind = imgArray[0]; break;  // 小玉
    case 3: p->kind = imgArray[3]; break;  // 銃弾
    default: p->kind = imgArray[0]; break;
    }

    // パラメータに世代と更新間隔を保存
    p->param_i[0] = splitGen;        // 0=大弾, 1=小弾, 2=微弾
    p->param_i[1] = updateInterval;  // 位置更新間隔（フレーム）
    p->param_i[2] = 0;               // ラグ回復フラグ（0=通常, 1=回復待ち）
    p->param_i[3] = 0;               // 明滅カウンタ
    p->param_d[0] = 0.0;
    p->param_d[1] = 0.0;
    p->param_d[2] = speed / 100.0;   // 元の速度（回復時に使用）
    p->margin = 240;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// ============================================================
//  弾の分裂処理
// ============================================================
static void SplitShot(
    sEnemyShotSet* pSet,
    sEnemyShot* pParent,
    int splitCount,
    int nextGen,
    int speed100,
    int updateInterval,
    int kindBase,
    int color
)
{
    for (int i = 0; i < splitCount; i++) {
        double baseMuki = pParent->muki + DX_PI;
        double spread = DX_PI / 4.0;
        double muki = baseMuki - spread / 2.0
            + spread * i / (splitCount - 1.0);

        // プレイヤー方向への微補正（20%程度）
        //double toPlayer = atan2(player.y - pParent->y, player.x - pParent->x);
        //muki = muki * 0.8 + toPlayer * 0.2;
        
        CreateShot(pSet, pParent->x + 1.0 * cos(muki), pParent->y + 1.0 * sin(muki),
            muki, speed100, kindBase, color, updateInterval, nextGen);
    }
}

// ============================================================
//  弾幕パターン：フレームドロップ・カスケード
// ============================================================
static void ShotLagCascade(sEnemyShotSet* pEnemyShotSet)
{
    // --------------------------------------------------------
    //  第1段：初回生成（24way大弾）
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < PHASE1_WAY; i++) {
            double muki = (DX_PI * 2.0 / PHASE1_WAY) * i;
            CreateShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                muki, PHASE1_SPEED,
                KIND_LARGE, COLOR_RED,
                PHASE1_UPDATE, 0);
        }
    }

    // --------------------------------------------------------
    //  ラグピーク判定：全弾数カウント
    // --------------------------------------------------------
    int totalShots = 0;
    sEnemyShot* pTmp = pEnemyShotSet->pEnemyShotHead->next;
    while (pTmp != pEnemyShotSet->pEnemyShotHead) {
        totalShots++;
        pTmp = pTmp->next;
    }

    // ラグピーク（処理落ち停止）判定
    bool isLagging = (totalShots >= LAG_THRESHOLD);

    if (isLagging) {
        if (pEnemyShotSet->param_i[0] == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        pEnemyShotSet->param_i[0]++;
    }
    else {
        pEnemyShotSet->param_i[0] = 0;
    }

    bool inLagFreeze = (pEnemyShotSet->param_i[0] >= 1 &&
        pEnemyShotSet->param_i[0] <= LAG_DURATION);

    // --------------------------------------------------------
    //  各弾の更新処理
    // --------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        // ラグピーク中：全弾完全停止（位置更新しない）
        if (inLagFreeze) {
            pShot->param_i[3]++;
            // 大弾のみ、プレイヤー位置にわずかなノイズを与える演出
            if (pShot->param_i[0] == 0) {
                static int noiseTimer = 0;
                if (noiseTimer++ % 4 == 0) {
                    int dx = GetRand(2) - 1;
                    int dy = GetRand(2) - 1;
                    player.x += dx;
                    player.y += dy;
                    spawnForceParticles(player.x, player.y, dx, dy);
                }
            }
            pShot = pNext;
            continue;
        }

        // ラグ回復直後の初回更新：速度・更新間隔を変更
        if (pShot->param_i[2] == 1) {
            pShot->param_i[2] = 0;
            pShot->param_i[1] = 1; // 更新間隔を1フレームに
            pShot->speed = pShot->param_d[2] * RECOVER_MULT;

            double toPlayer = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->muki = pShot->muki * 0.7 + toPlayer * 0.3;
        }

        // 位置更新（カクカク演出）
        // メインルーチンが自動で count++ するので、ここではインクリメントしない
        if (pShot->count % pShot->param_i[1] == 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        // --------------------------------------------------------
        //  画面端判定と分裂
        // --------------------------------------------------------
        bool outOfBounds = (pShot->x < -MARGIN ||
            pShot->x > 480 + MARGIN ||
            pShot->y < -MARGIN ||
            pShot->y > 480 + MARGIN);

        if (outOfBounds && pShot->param_i[0] < 2) {
            int nextGen = pShot->param_i[0] + 1;
            int splitCnt = (nextGen == 1) ? PHASE2_SPLIT : PHASE3_SPLIT;
            int spd = (nextGen == 1) ? PHASE2_SPEED : PHASE3_SPEED;
            int upd = (nextGen == 1) ? PHASE2_UPDATE : PHASE3_UPDATE;
            int kindBase = (nextGen == 1) ? KIND_MID : KIND_SMALL;
            int color = (nextGen == 1) ? COLOR_YEL : COLOR_GRN;

            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            SplitShot(pEnemyShotSet, pShot, splitCnt, nextGen,
                spd, upd, kindBase, color);

            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }

        pShot = pNext;
    }

    // --------------------------------------------------------
    //  ラグ解除時：全弾に回復フラグを立てる
    // --------------------------------------------------------
    if (pEnemyShotSet->param_i[0] == LAG_DURATION) {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            p->param_i[2] = 1;
            p = p->next;
        }
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = LAG_DURATION + 1;
    }
}

// ============================================================
//  敵本体のパターン：フレームドロップ・カスケード
// ============================================================
void EnemyPat_Tmp()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLagCascade;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->param_i[0] = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}