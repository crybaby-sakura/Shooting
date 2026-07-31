// enemyPat_kyouranMangekyou.cpp
// 狂乱万華鏡 - カオス系高難易度弾幕パターン
//
// フェーズ分けをせず、以下の3系統を非同期の周期で同時に走らせることで
// 「読めない・覚えられない」重畳型のカオスを狙う。
//   ①可変対称回転弾（万華鏡の芯）    … 20フレーム周期
//   ②自機狙い割り込み弾             … 47フレーム周期
//   ③ドリフトエコー弾（後半ほど歪む）… 83フレーム周期
// 周期を互いに素に近い値にすることで、危険地帯の重なり方が毎回変化する。
//
// 各弾の座標は pShot->count（生成からの経過フレーム）のみから導出する
// 純関数として計算し、速度積分は行わない。count類のインクリメントと
// 画面外弾の削除はメインルーチン側に一任する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// レイヤー①：可変対称回転弾
// バーストごとに対称数N・回転方向・回転速度・開始角をランダム化し、
// 偶数/奇数バーストで半径成長速度（低速/高速）を切り替える。
// 後発の高速弾が先発の低速弾に一定時間後追いつき、圧縮帯を作る。
// ------------------------------------------------------------
static void ShotBaseSpin(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        static const int symmetrySeq[8] = { 3, 5, 7, 4, 6, 5, 3, 7 };
        int burstIndex = pEnemyShotSet->kind; // 呼び出し側でバースト通し番号を格納
        int n = symmetrySeq[burstIndex % 8];

        int dir = (GetRand(1) == 0) ? 1 : -1;
        double angularSpeed = dir * (0.010 + GetRand(10) / 1000.0); // ±0.010〜0.040 rad/frame
        double baseOffset = GetRand(359) / 180.0 * DX_PI;           // 0〜2πの開始角
        double radialSpeed = (burstIndex % 2 == 0) ? 3.2 : 1.6;     // 偶数=高速 / 奇数=低速

        for (int i = 0; i < n; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->kind = img_enemyShotLargeBall[i % 9]; // 虹色に近い彩り＝万華鏡感
            pEnemyShot->margin = 20;

            pEnemyShot->param_d[0] = pEnemyShotSet->x;                   // 中心x
            pEnemyShot->param_d[1] = pEnemyShotSet->y;                   // 中心y
            pEnemyShot->param_d[2] = baseOffset + 2.0 * DX_PI * i / n;   // 初期角
            pEnemyShot->param_d[3] = angularSpeed;                      // 角速度
            pEnemyShot->param_d[4] = radialSpeed;                       // 半径成長速度

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double angle = pShot->param_d[2] + pShot->param_d[3] * t;
        double r = pShot->param_d[4] * t;

        pShot->x = pShot->param_d[0] + r * cos(angle);
        pShot->y = pShot->param_d[1] + r * sin(angle);
        pShot->muki = angle; // 見た目の向きも回転角に合わせる

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// レイヤー②：自機狙い割り込み弾
// ①とは非同期の周期で、発射時点の自機座標へ3WAYを撃ち込む。
// ①の回転とは無関係に飛ぶため、重なる位置・密度が毎回変わり安地を固定させない。
// ------------------------------------------------------------
static void ShotAimedBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double aimAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        static const double spreadDeg[3] = { -8.0, 0.0, 8.0 };
        double speed = 3.4;

        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = aimAngle + spreadDeg[i] / 180.0 * DX_PI;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->kind = img_enemyShotBullet[0]; // 赤＝警戒色として①と区別
            pEnemyShot->margin = 20.0;
            pEnemyShot->muki = angle;
            pEnemyShot->margin = 200;

            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = angle;
            pEnemyShot->param_d[3] = speed;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->param_d[3] * t * cos(pShot->param_d[2]);
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * t * sin(pShot->param_d[2]);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// レイヤー③：ドリフトエコー弾
// 直進しつつ、時間経過で振動周波数が増していく横ずれを付与する。
// 序盤は素直な弾に見えるが、後半ほど蛇行が激しくなり読みを外す。
// ①②とも非同期の周期で発射。
// ------------------------------------------------------------
static void ShotDriftEcho(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double aimAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = aimAngle + (GetRand(40) - 20) / 180.0 * DX_PI; // ±20度でランダムに散らす
            double freq0 = 0.015 + GetRand(10) / 1000.0;                  // 初期振動周波数
            double freqGrowth = 0.0004 + GetRand(4) / 10000.0;           // 周波数の増加率
            double amplitude = 22.0 + GetRand(16);                       // 横ずれ振幅

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->kind = img_enemyShotDiamond[3]; // シアン＝③の識別色
            pEnemyShot->margin = 20.0;
            pEnemyShot->muki = angle;
            pEnemyShot->margin = 200;

            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = angle;
            pEnemyShot->param_d[3] = 2.0; // 直進速度
            pEnemyShot->param_d[4] = freq0;
            pEnemyShot->param_d[5] = freqGrowth;
            pEnemyShot->param_d[6] = amplitude;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double angle = pShot->param_d[2];
        double speed = pShot->param_d[3];
        double freq0 = pShot->param_d[4];
        double freqGrowth = pShot->param_d[5];
        double amplitude = pShot->param_d[6];

        double phase = freq0 * t + 0.5 * freqGrowth * t * t; // 周波数増加を積分した位相
        double baseX = pShot->param_d[0] + speed * t * cos(angle);
        double baseY = pShot->param_d[1] + speed * t * sin(angle);
        double perp = angle + DX_PI / 2.0;

        pShot->x = baseX + amplitude * sin(phase) * cos(perp);
        pShot->y = baseY + amplitude * sin(phase) * sin(perp);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体：狂乱万華鏡
// フェーズ分けをせず、①②③を非同期周期(20/47/83フレーム)で同時に撃ち続け、
// 重なり方が毎回変化する「読めないカオス」を作る。
// ------------------------------------------------------------
void EnemyPat_TooChaotic_Claude()
{
    static int baseBurstIndex;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        baseBurstIndex = 0;
    }
    else {
        // 敵自身も規則的な往復ではなく、周期の異なる2つのsinを重ねた揺らぎで動く
        enemy.x = 240.0 + 120.0 * sin(count / 97.0);
        enemy.y = 80.0 + 20.0 * sin(count / 53.0);
    }

    // レイヤー①：可変対称回転弾（20フレーム周期）
    if (count % 20 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBaseSpin;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->kind = baseBurstIndex++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // レイヤー②：自機狙い割り込み弾（47フレーム周期、①とは非同期）
    if (count % 47 == 8) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAimedBurst;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // レイヤー③：ドリフトエコー弾（83フレーム周期、①②とも非同期）
    if (count % 83 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDriftEcho;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}