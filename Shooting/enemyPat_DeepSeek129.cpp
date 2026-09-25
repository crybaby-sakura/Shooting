#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 時間停止中かどうか
static bool g_timeStopped = false;

// 時間停止中に固定する自機の前フレーム位置
static double g_prevPlayerX = 0.0;
static double g_prevPlayerY = 0.0;

// 敵弾を1つ追加するヘルパー
static void AddEnemyShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// 敵弾セットを追加するヘルパー
static void CreateEnemyShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// 時間停止中でなければ敵弾を移動させる
static void MoveShotsIfNotStopped(sEnemyShotSet* pSet)
{
    if (g_timeStopped) return;

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// 弾幕：全方向リング
static void ShotRing(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int n = 16;
        for (int i = 0; i < n; ++i) {
            double angle = pSet->muki + i * 2.0 * DX_PI / n;
            int color = (pSet->kind + i) % 9;
            AddEnemyShot(pSet, pSet->x, pSet->y, angle, 2.5, img_enemyShotSmallBall[color]);
        }
    }
    MoveShotsIfNotStopped(pSet);
}

// 弾幕：自機狙い扇
static void ShotAimedFan(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int n = 5;
        for (int i = 0; i < n; ++i) {
            double angle = pSet->muki + (i - (n - 1) / 2.0) * 0.18;
            AddEnemyShot(pSet, pSet->x, pSet->y, angle, 3.5, img_enemyShotMediumBall[4]);
        }
    }
    MoveShotsIfNotStopped(pSet);
}

// 弾幕：螺旋
static void ShotSpiral(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int n = 4;
        for (int i = 0; i < n; ++i) {
            double angle = pSet->muki + i * DX_PI / 2.0;
            AddEnemyShot(pSet, pSet->x, pSet->y, angle, 2.0, img_enemyShotDiamond[1]);
        }
    }
    MoveShotsIfNotStopped(pSet);
}

// 弾幕：大玉リング
static void ShotLargeRing(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int n = 8;
        for (int i = 0; i < n; ++i) {
            double angle = pSet->muki + i * 2.0 * DX_PI / n;
            AddEnemyShot(pSet, pSet->x, pSet->y, angle, 1.5, img_enemyShotLargeBall[0]);
        }
    }
    MoveShotsIfNotStopped(pSet);
}

// 敵本体のパターン
void EnemyPat_TimeStop_DeepSeek()
{
    static int shotKind = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;

        g_timeStopped = false;
        g_prevPlayerX = player.x;
        g_prevPlayerY = player.y;
        shotKind = 0;
    }

    // 1周期 = 予告1秒 + 時間停止3秒 + 通常3秒 = 7秒 = 420フレーム
    const int period = 420;
    int phase = (count - 1) % period;

    bool isWarning = (phase == 0);
    bool isTimeStop = (phase >= 60 && phase < 240);
    bool isNormal = (phase >= 240 && phase < 420);

    // 予告音
    if (isWarning) {
        if (CheckSoundMem(sound_enemyCharge)) {
            StopSoundMem(sound_enemyCharge);
        }
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    g_timeStopped = isTimeStop;

    // 時間停止中：自機位置を前フレーム位置に強制固定
    if (g_timeStopped) {
        player.x = g_prevPlayerX;
        player.y = g_prevPlayerY;

        // 自機ショットの y 座標を +15.0 して、メインルーチンの -15.0 を打ち消す
        sPlayerShot* p = playerShotHead.next;
        while (p != &playerShotHead) {
            p->y += 15.0;
            p = p->next;
        }
    }

    // ボス移動
    if (g_timeStopped) {
        // 時間停止中：激しく動き回る
        int t = phase - 60; // 0..179
        double rt = t * 0.05;

        enemy.x = 240.0 + 140.0 * cos(rt * 1.7);
        enemy.y = 200.0 + 100.0 * sin(rt * 2.3);

        // 画面内にクランプ
        if (enemy.x < 30.0)  enemy.x = 30.0;
        if (enemy.x > 450.0) enemy.x = 450.0;
        if (enemy.y < 30.0)  enemy.y = 30.0;
        if (enemy.y > 450.0) enemy.y = 450.0;
    }
    else {
        // 通常時：ゆっくり左右に移動
        enemy.x = 240.0 + 120.0 * sin(count * 0.01);
        enemy.y = 80.0 + 20.0 * sin(count * 0.02);
    }

    // 時間停止中：ボスは動き回りながら敵弾を撃ちまくる
    if (g_timeStopped) {
        int t = phase - 60; // 0..179

        if (t % 4 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            
            double aim = atan2(player.y - enemy.y, player.x - enemy.x);
            int pattern = (t / 4) % 4;

            switch (pattern) {
            case 0:
                CreateEnemyShotSet(ShotRing, enemy.x, enemy.y, aim, shotKind++);
                break;
            case 1:
                CreateEnemyShotSet(ShotAimedFan, enemy.x, enemy.y, aim, shotKind++);
                break;
            case 2:
                CreateEnemyShotSet(ShotSpiral, enemy.x, enemy.y, aim, shotKind++);
                break;
            case 3:
                CreateEnemyShotSet(ShotLargeRing, enemy.x, enemy.y, aim + DX_PI * 0.25, shotKind++);
                break;
            }
        }
    }

    // 前フレームの自機位置を保存
    g_prevPlayerX = player.x;
    g_prevPlayerY = player.y;
}