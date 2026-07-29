// enemyPat_RippleCounter.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ====================================================================
// 状態共有用のファイルスコープ変数（波紋ギミック用）
// ====================================================================
static int g_rippleHitCount = 0; // 現在蓄積している波紋の数
static int g_burstPhase = 0;     // 0:波紋生成中, 1:一時停止, 2:自機へ突撃
static int g_burstTimer = 0;     // バースト演出用のタイマー
static int g_prevHp = 0;         // 前フレームのボスHP（被弾検知用）

// ====================================================================
// 弾幕パターン：波紋（1層分）
// 生成時に16発の弾を放ち、あとは全体フェイズ(g_burstPhase)に従って動く
// ====================================================================
static void ShotRipple(sEnemyShotSet* pSet)
{
    // 初回のみ16方向の弾を生成
    if (pSet->count == 0) {
        int way = 16;
        double baseAngle = pSet->param_d[0]; // ずらした発射角度を受け取る

        for (int i = 0; i < way; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = baseAngle + (DX_PI * 2.0 / way) * i;
            pShot->speed = 6.0;
            pShot->kind = img_enemyShotSmallBall[3]; // シアンの小玉

            // 弾の個別状態 (0:拡散中, 1:停止中, 2:突撃中)
            pShot->param_i[0] = 0;
            pShot->margin = 999;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の挙動更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {

        // 全体のフェイズ(g_burstPhase)を見て、弾の内部状態を進行させる
        if (pShot->param_i[0] == 0 && g_burstPhase == 1) {
            pShot->param_i[0] = 1; // 停止状態へ
        }
        else if (pShot->param_i[0] == 1 && g_burstPhase == 2) {
            pShot->param_i[0] = 2; // 突撃状態へ
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 0.5; // 突撃時の初速
        }

        // 状態に応じた速度変化
        if (pShot->param_i[0] == 0) {
            if (pShot->speed > 0.7) pShot->speed -= 0.03; // 徐々に減速して漂う
        }
        else if (pShot->param_i[0] == 1) {
            pShot->speed = 0.0; // 完全に停止
        }
        else if (pShot->param_i[0] == 2) {
            if (pShot->speed < 8.0) pShot->speed += 0.15; // 高速まで加速
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ====================================================================
// ボスの基本攻撃 A：定期的な自機狙い3way弾
// ====================================================================
static void ShotBaseAim(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseMuki = atan2(player.y - pSet->y, player.x - pSet->x);
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = baseMuki + (i - 1) * 0.25;
            pShot->speed = 3.5;
            pShot->kind = img_enemyShotMediumBall[0]; // 赤の中玉

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ====================================================================
// ボスの基本攻撃 B：ゆったりとした回転レーザー
// ====================================================================
static void ShotBaseSpiral(sEnemyShotSet* pSet)
{
    if (pSet->count % 5 == 0 && pSet->count <= 75) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int way = 4;
        for (int i = 0; i < way; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = (DX_PI * 2.0 / way) * i + (pSet->count * 0.02);
            pShot->speed = 2.5;
            pShot->kind = img_enemyShotMediumOval[5]; // マゼンタの中楕円弾

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ====================================================================
// 敵本体のパターン
// ====================================================================
void EnemyPat_Counter_Gemini()
{
    static int muki;

    // --- 初期化 ---
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        g_rippleHitCount = 0;
        g_burstPhase = 0;
        g_burstTimer = 0;
        g_prevHp = enemy.hp;
    }

    // --- 敵の移動 ---
    if (count > 60) {
        enemy.x += 0.8 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // ================================================================
    // 波紋カウンターの進行管理
    // ================================================================

    // [フェイズ1・2] バーストの進行
    if (g_burstPhase == 1) {
        g_burstTimer++;
        if (g_burstTimer > 40) {
            g_burstPhase = 2; // 突撃開始
            g_burstTimer = 0;
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
    }
    else if (g_burstPhase == 2) {
        g_burstTimer++;
        if (g_burstTimer > 120) {
            // 突撃開始から約0.5秒後、通常状態に復帰。
            // 突撃開始後の弾はすでにShotSet側で内部状態が「2」に固定されているため、
            // 全体フェイズを0に戻しても止まらずに飛んでいきます。
            g_burstPhase = 0;
            g_burstTimer = 0;
            g_rippleHitCount = 0;
        }
    }

    // [フェイズ0] 被弾判定と波紋1層分のShotSet生成
    if (g_burstPhase == 0 && enemy.hp < g_prevHp) {

        // 生成過多による音割れを防ぐため、適度に間引いて鳴らす
        if (g_rippleHitCount % 1 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->patternFunc = ShotRipple;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->param_d[0] = g_rippleHitCount * 0.05; // 発生ごとの角度ズレ

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        g_rippleHitCount++;

        // 規定回数（15回）被弾したらバースト発動へ移行
        if (g_rippleHitCount >= 15) {
            g_burstPhase = 1;
            g_burstTimer = 0;
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }

    // ================================================================
    // 基本攻撃のスケジュール
    // ================================================================
    if (count % 120 == 60) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->patternFunc = ShotBaseAim;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    if (count % 250 == 150) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->patternFunc = ShotBaseSpiral;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --- 次フレームの被弾検知用に現在のHPを保存 ---
    g_prevHp = enemy.hp;
}