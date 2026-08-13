// enemyPat_tmp.cpp
// 双星「シンクロナイズド・オービット」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const double PI2 = DX_PI * 2.0;

// ------------------------------------------------------------
// 弾幕本体
// 1つの ShotSet が、発射した弾を「回転しながら外へ流す」。
// param_d[0] : 初期角度
// param_d[1] : 回転方向 (+1 / -1)
// param_d[2] : 初期半径
// param_d[3] : 螺旋の位相
// ------------------------------------------------------------
static void ShotSyncOrbit(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 1つの発射元から、5本の回転する螺旋弾を出す。
        for (int i = 0; i < 5; ++i) {
            sEnemyShot* pShot = new sEnemyShot;

            const double localAngle = i * PI2 / 5.0;
            const double angle = pEnemyShotSet->muki + localAngle;

            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = angle;
            pShot->speed = 1.0;

            // 左右で色を変える。左はシアン、右はマゼンタ。
            const int color = pEnemyShotSet->kind;
            pShot->kind = img_enemyShotMediumBall[color];

            pShot->param_d[0] = angle;
            pShot->param_d[1] = pEnemyShotSet->param_d[1];
            pShot->param_d[2] = 0.0;
            pShot->param_d[3] = localAngle;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 発射直後は緩やかに外へ開き、その後さらに回転速度を上げる。
        const double t = (double)pShot->count;
        const double radius = 4.0 + t * 0.90;
        const double angle =
            pShot->param_d[0]
            + pShot->param_d[1] * (0.0060 * t + 0.000018 * t * t)
            + pShot->param_d[3] * 0.12;

        pShot->x = pEnemyShotSet->x + cos(angle) * radius;
        pShot->y = pEnemyShotSet->y + sin(angle) * radius;

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 追加の細い螺旋。
// 中盤以降、2体の中玉の間を縫うように出す。
// ------------------------------------------------------------
static void ShotSyncDiamond(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; ++i) {
            sEnemyShot* pShot = new sEnemyShot;

            const double a = pEnemyShotSet->muki + i * PI2 / 3.0;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = a;
            pShot->speed = 1.0;
            pShot->kind = img_enemyShotDiamond[pEnemyShotSet->kind];

            pShot->param_d[0] = a;
            pShot->param_d[1] = pEnemyShotSet->param_d[1];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double t = (double)pShot->count;
        const double radius = 12.0 + t * 1.20;
        const double angle =
            pShot->param_d[0]
            + pShot->param_d[1] * (0.010 + 0.000035 * t) * t;

        pShot->x = pEnemyShotSet->x + cos(angle) * radius;
        pShot->y = pEnemyShotSet->y + sin(angle) * radius;

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体
// 2体のボスを中央を軸にした楕円軌道で同期移動させる。
// ------------------------------------------------------------
void EnemyPat_TwoBoss_ChatGPT()
{
    static double phase;
    static int shotSide;

    if (count == 1) {
        enemy.x = 130.0;
        enemy.y = 90.0 + 30;
        enemy.x2 = 350.0;
        enemy.y2 = 90.0 + 30;
        enemy.maxHp = enemy.hp = 200;

        phase = 0.0;
        shotSide = 0;
    }

    // 2体が左右対称のまま、中央を基準に回り込む。
    phase += 0.018;

    const double centerX = 240.0;
    const double centerY = 82.0 + 30;
    const double orbitX = 112.0;
    const double orbitY = 42.0;

    enemy.x = centerX - cos(phase) * orbitX;
    enemy.y = centerY + sin(phase) * orbitY;
    enemy.x2 = centerX + cos(phase) * orbitX;
    enemy.y2 = centerY - sin(phase) * orbitY;

    // 序盤は10フレームごと、中盤以降は発射間隔を短くする。
    int interval = count < 420 ? 12 : 8;

    if (count % interval == 1) {
        const bool left = (shotSide == 0);
        const double bx = left ? enemy.x : enemy.x2;
        const double by = left ? enemy.y : enemy.y2;

        // 左は中央へ向かって時計回り、右は反時計回り。
        const double towardCenter =
            atan2(centerY - by, centerX - bx);
        const double rotateSign = left ? 1.0 : -1.0;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc =
            count < 420 ? ShotSyncOrbit : ShotSyncDiamond;
        pSet->x = bx;
        pSet->y = by + 8.0;
        pSet->muki = towardCenter;
        pSet->kind = left ? 3 : 5;
        pSet->param_d[1] = rotateSign;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        shotSide ^= 1;
    }

    // 終盤は2体の正面に大きめの弾を追加して、
    // 左右の螺旋だけでなく中央の交差点も危険にする。
    if (count >= 540 && count % 36 == 1) {
        for (int side = 0; side < 2; ++side) {
            const double bx = side == 0 ? enemy.x : enemy.x2;
            const double by = side == 0 ? enemy.y : enemy.y2;

            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSyncOrbit;
            pSet->x = bx;
            pSet->y = by + 6.0;
            pSet->muki =
                atan2(centerY - pSet->y, centerX - pSet->x);
            pSet->kind = side == 0 ? 1 : 8;
            pSet->param_d[1] = side == 0 ? 1.0 : -1.0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}