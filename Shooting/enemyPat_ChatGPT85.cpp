// enemyPat_TASFutureSync.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// TAS前提「未来予測・完全同期」
//
// ・一定周期ごとに、次の安全地帯が先回りして移動する。
// ・画面左右から進む弾壁の中央だけが細く開く。
// ・安全地帯の中心は周期ごとに大きく蛇行するため、
//   自機を決め打ちのルートで移動させる必要がある。
// ・後半は小弾の追撃列が安全地帯の「直前の位置」を埋め、
//   その場に留まるだけでは抜けられない。
//
// count / pEnemyShotSet->count / pEnemyShot->count の加算、
// 画面外弾の削除はメインルーチン側で行う前提。
// ============================================================

static const double SCREEN_CX = 240.0;
static const double SCREEN_H = 480.0;

// ------------------------------------------------------------
// 敵弾生成共通処理
// ------------------------------------------------------------
static void AddShot(
    sEnemyShotSet* set,
    double x, double y,
    double muki, double speed,
    int kind,
    int param0 = 0,
    double param0d = 0.0)
{
    sEnemyShot* shot = new sEnemyShot;
    shot->x = x;
    shot->y = y;
    shot->muki = muki;
    shot->speed = speed;
    shot->kind = kind;
    shot->param_i[0] = param0;
    shot->param_d[0] = param0d;
    shot->margin = 240;

    shot->prev = set->pEnemyShotHead->prev;
    shot->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = shot;
    set->pEnemyShotHead->prev = shot;
}

// ------------------------------------------------------------
// 未来予測ルート
// cycleごとに安全地帯が大きくジャンプしながら蛇行する。
// 「次の瞬間にどこへ移動するか」を先読みできないと厳しい。
// ------------------------------------------------------------
static double SafeX(int cycle)
{
    // 8拍で左右を大きく横切る。TASではこの座標列を
    // あらかじめ入力ルートとして把握して追従する。
    static const double route[8] = {
        70.0, 142.0, 334.0, 410.0,
        396.0, 316.0, 126.0, 48.0
    };
    return route[cycle & 7];
}

// 次の安全地帯を「予測」した位置。
static double FutureSafeX(int cycle)
{
    return SafeX(cycle + 1);
}

// ------------------------------------------------------------
// 弾壁：横から飛来する小玉の列
// param_i[0] : side (0=左,1=右)
// param_i[1] : cycle
// param_i[2] : bullet index
// param_d[0] : 安全地帯X
// ------------------------------------------------------------
static void ShotFutureWall(sEnemyShotSet* set)
{
    if (set->count == 0) {
        const int side = set->param_i[0];
        const int cycle = set->param_i[1];
        const double safeX = set->param_d[0];

        // 上下から画面を横断する「弾壁」。
        // side=0 は上、side=1 は下から進む。
        const double y = side == 0 ? -18.0 : 498.0;
        const double vy = side == 0 ? 2.70 : -2.70;
        const double gap = 17.0;
        int index = 0;

        for (double x = -10.0; x <= 490.0; x += 11.0) {
            // 安全地帯は未来のSafeXを中心とする細い縦穴。
            // 周辺だけ少し蛇行させ、完全な直線読みを防ぐ。
            const double localSafe = safeX +
                11.0 * sin((x + cycle * 41.0) * 0.052);

            if (fabs(x - localSafe) > gap) {
                const double muki = side == 0 ? DX_PI * 0.5 : -DX_PI * 0.5;
                AddShot(
                    set,
                    x, y,
                    muki,
                    fabs(vy),
                    img_enemyShotSmallBall[(cycle + index) % 8],
                    side,
                    localSafe);
            }
            ++index;
        }
    }

    sEnemyShot* shot = set->pEnemyShotHead->next;
    while (shot != set->pEnemyShotHead) {
        shot->x += shot->speed * cos(shot->muki);
        shot->y += shot->speed * sin(shot->muki);

        // 穴の輪郭だけが微妙に揺れるようにする。
        // 揺れ幅は小さく、TASなら先読み可能な範囲に収める。
        shot->x += 0.22 * sin((double)shot->count * 0.09 + shot->param_d[0] * 0.013);
        shot = shot->next;
    }
}

// ------------------------------------------------------------
// 追撃弾：直前の安全地帯を埋める短い列
// TASルートを数フレームずらすと引っ掛かるため、
// 単純に壁だけ避けても攻略できない。
// ------------------------------------------------------------
static void ShotPastTrail(sEnemyShotSet* set)
{
    if (set->count == 0) {
        const int cycle = set->param_i[0];
        const double oldX = set->param_d[0];

        // 前周期の安全地帯付近から斜めに流す。
        for (int i = 0; i < 19; ++i) {
            const double y = 20.0 + i * 24.0;
            const double bend = 21.0 * sin(i * 0.55 + cycle * 0.9);
            const double x = oldX + bend;
            const double angle = atan2(enemy.y - y, enemy.x - x);

            AddShot(
                set,
                x, y,
                angle,
                1.55,
                img_enemyShotDiamond[(i + cycle) % 8],
                i,
                oldX);
        }
    }

    sEnemyShot* shot = set->pEnemyShotHead->next;
    while (shot != set->pEnemyShotHead) {
        shot->x += shot->speed * cos(shot->muki);
        shot->y += shot->speed * sin(shot->muki);
        shot = shot->next;
    }
}

// ------------------------------------------------------------
// 追い込みの小弾円弧
// 後半のみ使用。
// ------------------------------------------------------------
static void ShotPredictionRing(sEnemyShotSet* set)
{
    if (set->count == 0) {
        const int cycle = set->param_i[0];
        const double safeX = set->param_d[0];

        // 中央ボス付近に輪を作り、安全地帯とは反対側へ圧力をかける。
        for (int i = 0; i < 24; ++i) {
            const double a = (2.0 * DX_PI * i) / 24.0;
            const double x = enemy.x + 74.0 * cos(a);
            const double y = enemy.y + 74.0 * sin(a);
            const double outward = a + 0.18 * sin(cycle * 0.7);

            AddShot(
                set,
                x, y,
                outward,
                1.85,
                img_enemyShotMediumBall[(i + cycle) % 8],
                cycle,
                safeX);
        }
    }

    sEnemyShot* shot = set->pEnemyShotHead->next;
    while (shot != set->pEnemyShotHead) {
        // 円からゆっくり外へ開く。
        const double cx = enemy.x;
        const double cy = enemy.y;
        const double a = atan2(shot->y - cy, shot->x - cx);
        const double r = hypot(shot->x - cx, shot->y - cy) + 0.35;
        const double rr = r + 0.85;
        shot->x = cx + rr * cos(a + 0.012);
        shot->y = cy + rr * sin(a + 0.012);
        shot = shot->next;
    }
}

// ------------------------------------------------------------
// 弾幕本体
// ------------------------------------------------------------
void EnemyPat_TAS_ChatGPT()
{
    static int enemyMoveDir;
    static int shotCycle;
    static double lastSafeX;

    if (count == 1) {
        enemy.x = SCREEN_CX;
        enemy.y = 58.0;
        enemy.maxHp = enemy.hp = 200;
        enemyMoveDir = 1;
        shotCycle = 0;
        lastSafeX = SafeX(0);
    }
    else {
        // ボスは画面上部をゆっくり左右移動。
        enemy.x += enemyMoveDir * 0.72;
        if (enemy.x < 95.0 || enemy.x > 385.0)
            enemyMoveDir *= -1;
    }

    // --------------------------------------------------------
    // 1周期 = 180F
    // 予測ルートは周期開始時点ですでに「次の場所」を提示する。
    // TASでは、この先読みされたルートを正確に通る。
    // --------------------------------------------------------
    if (count % 180 == 1) {
        const double safeX = SafeX(shotCycle);
        const double futureX = FutureSafeX(shotCycle);

        // 上下の弾壁。未来のX座標にだけ細い安全路が開く。
        for (int side = 0; side < 2; ++side) {
            sEnemyShotSet* set = new sEnemyShotSet;
            set->count = 0;
            set->patternFunc = ShotFutureWall;
            set->x = enemy.x;
            set->y = -18.0;
            set->muki = 0.0;
            set->kind = side;
            set->param_i[0] = side;
            set->param_i[1] = shotCycle;
            set->param_d[0] = futureX;

            set->pEnemyShotHead = new sEnemyShot;
            set->pEnemyShotHead->prev = set->pEnemyShotHead;
            set->pEnemyShotHead->next = set->pEnemyShotHead;

            set->prev = enemyShotSetHead.prev;
            set->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = set;
            enemyShotSetHead.prev = set;
        }

        // 前周期の「安全だった場所」を追撃で埋める。
        if (shotCycle > 0) {
            sEnemyShotSet* set = new sEnemyShotSet;
            set->count = 0;
            set->patternFunc = ShotPastTrail;
            set->x = enemy.x;
            set->y = enemy.y;
            set->muki = 0.0;
            set->kind = shotCycle;
            set->param_i[0] = shotCycle;
            set->param_d[0] = lastSafeX;

            set->pEnemyShotHead = new sEnemyShot;
            set->pEnemyShotHead->prev = set->pEnemyShotHead;
            set->pEnemyShotHead->next = set->pEnemyShotHead;

            set->prev = enemyShotSetHead.prev;
            set->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = set;
            enemyShotSetHead.prev = set;
        }

        // 後半になるほど圧力を追加。
        if (shotCycle >= 4) {
            sEnemyShotSet* set = new sEnemyShotSet;
            set->count = 0;
            set->patternFunc = ShotPredictionRing;
            set->x = enemy.x;
            set->y = enemy.y;
            set->muki = 0.0;
            set->kind = shotCycle;
            set->param_i[0] = shotCycle;
            set->param_d[0] = futureX;

            set->pEnemyShotHead = new sEnemyShot;
            set->pEnemyShotHead->prev = set->pEnemyShotHead;
            set->pEnemyShotHead->next = set->pEnemyShotHead;

            set->prev = enemyShotSetHead.prev;
            set->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = set;
            enemyShotSetHead.prev = set;
        }

        lastSafeX = safeX;
        ++shotCycle;

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
}