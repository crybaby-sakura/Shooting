#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 青天の霹靂 モチーフ弾幕
// 既存素材のみ使用：小玉・中玉・大玉・短レーザー ＋ 色（青・シアン・白・黄）
// ============================================================

// メイン稲妻（太い縦レーザー＋扇状小弾）
static void ShotLightningMain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 重い発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 1. 縦に走る短レーザーを複数重ねて「稲妻」を表現（下方向・高速）
        //    色はシアン(3)・青(4)・白(6)をランダムに混ぜる
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(20) - 10);  // わずかに左右ブレ
            pEnemyShot->y = pEnemyShotSet->y - 20.0 + i * 8.0;     // 縦に少しずらして配置
            pEnemyShot->muki = DX_PI / 2.0;                         // 真下
            pEnemyShot->speed = 9.0 + GetRand(30) / 10.0;           // 高速
            int col = 3 + GetRand(3);                               // 3,4,5,6 → シアン〜白寄り
            if (col > 6) col = 6;
            pEnemyShot->kind = img_enemyShotLaser[col];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 2. 稲妻起点から扇状に小玉を高速で散らす（霹靂の衝撃波）
        const int N = 21;
        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 真下を中心に ±60度程度
            double base = DX_PI / 2.0;
            pEnemyShot->muki = base + (i - N/2) * (DX_PI / 18.0) + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.5 + GetRand(150) / 100.0;
            int col = (GetRand(2) == 0) ? 3 : 6;  // シアン or 白
            pEnemyShot->kind = img_enemyShotSmallBall[col];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 連鎖稲妻（短い縦レーザー＋中玉の放射）
static void ShotLightningChain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 短い縦レーザー（青寄り）
        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(16) - 8);
            pEnemyShot->y = pEnemyShotSet->y - 10.0 + i * 10.0;
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;  // 少し傾き
            pEnemyShot->speed = 7.5 + GetRand(20) / 10.0;
            int col = 4 + GetRand(2);  // 青 or シアン寄り
            if (col > 6) col = 4;
            pEnemyShot->kind = img_enemyShotLaser[col];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 通過点から中玉を放射状に（余波）
        for (int i = 0; i < 8*2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 20.0;
            pEnemyShot->muki = (i * 45.0/2 + GetRand(15) - 7) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.0 + GetRand(100) / 100.0;
            int col = (GetRand(2) == 0) ? 3 : 1;  // シアン or 黄
            pEnemyShot->kind = img_enemyShotMediumBall[col];
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

// 余韻の雨（低速下方向小玉）
static void ShotAfterRain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 画面上部からゆっくり落ちる小玉（青天が晴れた後の雨のような余韻）
        for (int i = 0; i < 12*2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 40.0 + GetRand(400);
            pEnemyShot->y = -10.0 - GetRand(30);
            pEnemyShot->margin = 50;
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(30) - 15) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.2 + GetRand(80) / 100.0;
            int col = (GetRand(3) == 0) ? 6 : 3;  // 白多め、たまにシアン
            pEnemyShot->kind = img_enemyShotSmallBall[col];
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
// 敵本体パターン
// ============================================================
void EnemyPat_SeitenNoHekireki_Grok()
{
    static int muki;
    static int phase_timer;   // フェーズ管理用
    static int bolt_count;    // 何回目の霹靂か

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase_timer = 0;
        bolt_count = 0;
    }
    else {
        // ゆっくり左右に揺れる（青天の静けさを残す）
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 80.0 || enemy.x > 400.0) muki *= -1;
        if (count % 180 == 90) muki *= -1;  // たまに反転
    }

    phase_timer++;

    // ---------- 静 → 霹靂 → 連鎖 → 余韻 のサイクル ----------
    // 1サイクル roughly 240フレーム前後

    // 予告チャージ音（メイン稲妻の少し前）
    if (phase_timer == 90) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // メイン稲妻（突然の霹靂）
    if (phase_timer == 120) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLightningMain;
        // 自機のXに少し寄せつつランダムブレ
        pEnemyShotSet->x = player.x + (GetRand(80) - 40);
        if (pEnemyShotSet->x < 40.0) pEnemyShotSet->x = 40.0;
        if (pEnemyShotSet->x > 440.0) pEnemyShotSet->x = 440.0;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = bolt_count;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 連鎖稲妻（2〜3本、位置ランダム）
    if (phase_timer == 150 || phase_timer == 165 || phase_timer == 180) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLightningChain;
        pEnemyShotSet->x = 60.0 + GetRand(360);
        pEnemyShotSet->y = 30.0 + GetRand(40);
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 余韻の雨
    if (phase_timer == 200) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAfterRain;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 0.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // サイクルリセット（少し間隔を空けて次の霹靂へ）
    if (phase_timer >= 260) {
        phase_timer = 0;
        bolt_count++;
    }
}