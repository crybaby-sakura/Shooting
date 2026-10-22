#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// バックドラフト用 共有状態
// メインの敵パターンと弾パターンで共有する
// ============================================================
static int g_bdPhase = 0; // 0:充満 1:停止(予告) 2:吸い込み 3:爆発
static double g_bdCx = 240.0;
static double g_bdCy = 120.0;
static int g_bdLevel = 0; // 周回ごとに難易度上昇

// ShotSet生成ヘルパ
static sEnemyShotSet* CreateSet(void(*func)(sEnemyShotSet*), double x, double y, double muki, int kind)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = func;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->kind = kind;
    for (int i = 0; i < 16; i++) { p->param_i[i] = 0; p->param_d[i] = 0.0; }
    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;
    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
    return p;
}

// ============================================================
// 0: 燻りガス - 赤小玉=可燃ガス、黒小玉=煤煙 超低速で漂う
// count==0で生成、以降はg_bdPhaseに応じて停止→吸い込み
// ============================================================
static void ShotGasDrift(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 生成時のみ軽い発射音
        if (pSet->kind % 7 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        int num = 3; // 1セットあたり3発 (赤2黒1)
        for (int i = 0; i < num; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x + (double)(GetRand(60) - 30);
            p->y = pSet->y + (double)(GetRand(30) - 15);
            // 全方位にゆっくり広がる
            p->muki = (GetRand(360) / 180.0) * DX_PI;
            // GetRandは0～x inclusiveなので 0.3～0.8 に調整
            p->speed = 0.3 + (double)GetRand(50) / 100.0;
            p->count = 0;
            // 素材選定: 0:赤(ガス) 7:黒(煙) を交互に
            if (i == 2) p->kind = img_enemyShotSmallBall[7]; // 黒
            else p->kind = img_enemyShotSmallBall[0]; // 赤

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (g_bdPhase == 1) {
            // 1:密閉停止 - 摩擦で止める
            pShot->speed *= 0.88;
            if (pShot->speed < 0.02) pShot->speed = 0.0;
        }
        else if (g_bdPhase == 2) {
            // 2:吸い込み - 中心へ向きを変え加速
            double dx = g_bdCx - pShot->x;
            double dy = g_bdCy - pShot->y;
            double target = atan2(dy, dx);
            double diff = target - pShot->muki;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            pShot->muki += diff * 0.25;
            pShot->speed += 0.14 + g_bdLevel * 0.03;
            if (pShot->speed > 5.0) pShot->speed = 5.0;

            // 中心に到達したら画面外に飛ばしてメイン側で消去させる
            if (dx * dx + dy * dy < 36.0) {
                pShot->x = -100.0;
                pShot->y = -100.0;
            }
        }
        // 通常移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 1: 酸素の流れ - 細いレーザー[白/シアン]を画面端から中心へ
// ドアが開いた瞬間の吸気音を表現
// ============================================================
static void ShotOxygenWind(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        sEnemyShot* p = new sEnemyShot;
        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = pSet->muki; // 中心向きはセット生成時に計算済み
        p->speed = 6.5 + (double)GetRand(100) / 100.0;
        p->count = 0;
        // 短レーザー(64x4) 白=6 が酸素の筋に見える
        p->kind = img_enemyShotLaser[6];
        // 細く見せるために中楕円でも代用可: img_enemyShotMediumOval[6]など
        p->margin = 120;
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        double dx = g_bdCx - pShot->x;
        double dy = g_bdCy - pShot->y;
        if (dx * dx + dy * dy < 100.0) {
            pShot->x = -200.0; // 中心到達で消去
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 2: 爆発本体 - 大玉[赤] 低速だが大きくて避けにくい爆風
// ============================================================
static void ShotExplosionLarge(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int num = 24 + g_bdLevel * 4; // 周回で増える
        for (int i = 0; i < num; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            // 均等 + ランダムゆらぎ
            double base = 2.0 * DX_PI * i / num;
            double r = (double)(GetRand(40) - 20) / 180.0 * DX_PI;
            p->muki = base + r;
            p->speed = 2.2 + (double)GetRand(120) / 100.0 + g_bdLevel * 0.25;
            p->count = 0;
            // 大玉(20x20) 赤=0 が炎の塊
            p->kind = img_enemyShotLargeBall[0];
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 3: 火花 - 小玉[橙] 大玉の隙間を埋める高速弾
static void ShotExplosionSmall(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        int num = 36 + g_bdLevel * 6;
        for (int i = 0; i < num; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            double base = 2.0 * DX_PI * i / num + DX_PI / 36.0; // 大玉の隙間を狙う
            double r = (double)(GetRand(60) - 30) / 180.0 * DX_PI;
            p->muki = base + r;
            p->speed = 3.8 + (double)GetRand(200) / 100.0 + g_bdLevel * 0.3;
            p->count = 0;
            // 小玉(2.5x2.5) 橙=8 が火花
            p->kind = img_enemyShotSmallBall[8];
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 4: 死の火柱 - レーザー[赤] 自機狙い1本
static void ShotDeathFlame(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        sEnemyShot* p = new sEnemyShot;
        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = pSet->muki;
        p->speed = 5.5;
        p->count = 0;
        // 短レーザー 赤=0 が火柱
        p->kind = img_enemyShotLaser[0];
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_Backdraft_MetaAI()
{
    static int muki;
    static int bd_timer; // フェーズ内タイマー

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        bd_timer = 0;
        g_bdPhase = 0;
        g_bdLevel = 0;
        g_bdCx = enemy.x;
        g_bdCy = enemy.y + 10.0;
    }
    else {
        // 横にゆら
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;
        g_bdCx = enemy.x;
        g_bdCy = enemy.y + 10.0;
    }

    // フェーズ管理
    switch (g_bdPhase) {
    case 0: // 充満 - 遅いガスを撒き続ける
        if (count % 7 == 0) {
            CreateSet(ShotGasDrift, enemy.x, enemy.y + 10.0, 0.0, count % 10);
        }
        bd_timer++;
        if (bd_timer > 160 - g_bdLevel * 10) { // 周回で充満時間が短縮=猶予減
            g_bdPhase = 1;
            bd_timer = 0;
            // 予告音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        break;

    case 1: // 停止 - 静寂0.5秒と酸素流入
        if (bd_timer == 1) {
            for (int k = 0; k < 5; k++) {
                // 画面4辺から中心へ酸素レーザーを4本
                double sx[4] = { 0.0, 480.0, (double)GetRand(480), (double)GetRand(480) };
                double sy[4] = { (double)GetRand(480), (double)GetRand(480), 0.0, 480.0 };
                for (int i = 0; i < 4; i++) {
                    double mk = atan2(g_bdCy - sy[i], g_bdCx - sx[i]);
                    CreateSet(ShotOxygenWind, sx[i], sy[i], mk, 0);
                }
            }
        }
        bd_timer++;
        if (bd_timer > 32) {
            g_bdPhase = 2;
            bd_timer = 0;
        }
        break;

    case 2: // 吸い込み
        bd_timer++;
        if (bd_timer > 50) {
            g_bdPhase = 3;
            bd_timer = 0;
        }
        break;

    case 3: // 爆発
        if (bd_timer == 0) {
            // 本体爆発
            CreateSet(ShotExplosionLarge, g_bdCx, g_bdCy, 0.0, 0);
            CreateSet(ShotExplosionSmall, g_bdCx, g_bdCy, 0.0, 0);
            // 難易度2周目以降は火柱追加
            if (g_bdLevel >= 1) {
                double toPlayer = atan2(player.y - g_bdCy, player.x - g_bdCx);
                CreateSet(ShotDeathFlame, g_bdCx, g_bdCy, toPlayer, 0);
            }
        }
        bd_timer++;
        if (bd_timer > 50) {
            g_bdPhase = 0;
            bd_timer = 0;
            //g_bdLevel = (g_bdLevel + 1) % 3; // 0->1->2->0でループ
            g_bdLevel += 3;
        }
        break;
    }
}