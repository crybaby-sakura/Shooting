#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 時間停止用の共有状態
// ------------------------------------------------------------
static bool g_isTimeStop = false;
static double g_lockedPlayerX = 240.0;
static double g_lockedPlayerY = 400.0;

// 0:NORMAL 3秒, 1:WARNING 1秒, 2:TIMESTOP 3秒
static int s_phase = 0;
static int s_phaseTimer = 0;
static int s_muki = 1;
static double s_bossVx = 0.0;
static double s_bossVy = 0.0;
static int s_shotKindCounter = 0;

// ------------------------------------------------------------
// 通常弾: 自機狙い3way
// ------------------------------------------------------------
static void Shot_Normal(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // GetRand(x)は 0..x の x+1通りなので注意
            double spread = (i - 1) * (15.0 / 180.0 * DX_PI); // -15度,0,15度
            pEnemyShot->muki = pEnemyShotSet->muki + spread + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.0 + GetRand(100) / 100.0;

            // 中玉・赤〜黄で視認性確保
            pEnemyShot->kind = img_enemyShotMediumBall[GetRand(1)]; // 0:赤,1:黄

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 時間停止中は移動処理一切なし
    if (g_isTimeStop) return;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 時止め用: 全方位リング 止まってから動き出す
// ------------------------------------------------------------
static void Shot_TimeStop_Ring(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int way = 20;
        for (int i = 0; i < way; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + 2.0 * DX_PI / way * i;
            pEnemyShot->speed = 2.0 + GetRand(100) / 100.0; // 2.0-3.0

            // 小玉・白で密度を出す
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 6:白

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    if (g_isTimeStop) return;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 時止め用: ランダムばら撒き 配置して固める用
// ------------------------------------------------------------
static void Shot_TimeStop_Scatter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            // ボス周辺にランダム配置
            pEnemyShot->x = pEnemyShotSet->x + GetRand(200) - 100;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(120) - 20;
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI; // 全方向ランダム
            pEnemyShot->speed = 1.5 + GetRand(150) / 100.0;

            // 鱗弾・青系で時止め感
            pEnemyShot->kind = img_enemyShotScale[4 + GetRand(1)]; // 4:青,3:シアン寄りも可

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    if (g_isTimeStop) return;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// ボス本体
// ------------------------------------------------------------
void EnemyPat_TimeStop_MetaAI()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        s_muki = 1;
        s_phase = 0; // NORMALから開始
        s_phaseTimer = 0;
        s_bossVx = 0.0;
        s_bossVy = 0.0;
        s_shotKindCounter = 0;
        g_isTimeStop = false;
        g_lockedPlayerX = player.x;
        g_lockedPlayerY = player.y;
    }
    else {
        s_phaseTimer++;
    }

    // --------------------------------------------------------
    // フェーズ遷移
    // --------------------------------------------------------
    if (s_phase == 0) { // NORMAL 3秒
        if (s_phaseTimer >= 180) {
            s_phase = 1;
            s_phaseTimer = 0;
            // 予告音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }
    else if (s_phase == 1) { // WARNING 1秒
        if (s_phaseTimer >= 60) {
            s_phase = 2;
            s_phaseTimer = 0;
            g_isTimeStop = true;
            // 自機位置をロック
            g_lockedPlayerX = player.x;
            g_lockedPlayerY = player.y;
            // ボス移動用初速
            s_bossVx = (GetRand(200) - 100) / 40.0;
            s_bossVy = (GetRand(200) - 100) / 60.0;
        }
    }
    else if (s_phase == 2) { // TIMESTOP 3秒
        // 時間停止中の処理: 自機固定、自機弾停止
        player.x = g_lockedPlayerX;
        player.y = g_lockedPlayerY;

        sPlayerShot* pPS = playerShotHead.next;
        while (pPS != &playerShotHead) {
            pPS->y += 15.0; // メインの -15.0 を打ち消し
            pPS = pPS->next;
        }

        if (s_phaseTimer >= 180) {
            s_phase = 0;
            s_phaseTimer = 0;
            g_isTimeStop = false;
        }
    }

    // --------------------------------------------------------
    // ボスの動き
    // --------------------------------------------------------
    if (s_phase == 0) { // 通常はゆっくり横移動
        enemy.x += 0.98 * (double)s_muki;
        if (count % 120 == 60) s_muki *= -1;
        enemy.y = 60.0 + sin(count * 0.02) * 10.0;
    }
    else if (s_phase == 1) { // 予告中は微振動
        enemy.x += (GetRand(100) - 50) / 150.0;
        enemy.y += (GetRand(100) - 50) / 150.0;
    }
    else { // 時止め中は動き回る
        if (s_phaseTimer % 30 == 1) {
            s_bossVx = (GetRand(200) - 100) / 25.0; // -4.0〜4.0
            s_bossVy = (GetRand(200) - 100) / 40.0; // -2.5〜2.5
        }
        enemy.x += s_bossVx;
        enemy.y += s_bossVy;

        if (enemy.x < 40.0) { enemy.x = 40.0; s_bossVx *= -1; }
        if (enemy.x > 440.0) { enemy.x = 440.0; s_bossVx *= -1; }
        if (enemy.y < 30.0) { enemy.y = 30.0; s_bossVy *= -1; }
        if (enemy.y > 180.0) { enemy.y = 180.0; s_bossVy *= -1; }
    }

    // --------------------------------------------------------
    // 弾生成
    // --------------------------------------------------------
    if (s_phase == 0 && count % 20 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Shot_Normal;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = s_shotKindCounter++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    if (s_phase == 2 && count % 6 == 0) { // 時止め中は撃ちまくる
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        // リングとばら撒きを交互
        pSet->patternFunc = (s_phaseTimer % 2 == 0) ? Shot_TimeStop_Ring : Shot_TimeStop_Scatter;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = s_shotKindCounter++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}