// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 残像の輪
// ============================================================
static void ShotAfterimage(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 徐々に外側へ広がる
        double r = pShot->param_d[0] + pShot->speed * pShot->count;

        pShot->x = pEnemyShotSet->x + cos(pShot->muki) * r;
        pShot->y = pEnemyShotSet->y + sin(pShot->muki) * r;

        pShot = pShot->next;
    }
}

// ============================================================
// 転位軌跡
// ============================================================
static void ShotWarpLine(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // param_d[0] : 線分上の位置
        // param_d[1] : 線分の始点X
        // param_d[2] : 線分の始点Y
        // param_d[3] : 線分の長さ方向

        double t = pShot->param_d[0] + pShot->speed * 0.0025;

        // 軌跡の終端まで到達したら、再び始点側へ戻す
        if (t > 1.0)
            t -= 1.0;

        pShot->param_d[0] = t;

        double x0 = pShot->param_d[1];
        double y0 = pShot->param_d[2];
        double dx = cos(pShot->param_d[3]);
        double dy = sin(pShot->param_d[3]);
        double len = pShot->param_d[4];

        pShot->x = x0 + dx * len * t;
        pShot->y = y0 + dy * len * t;

        pShot = pShot->next;
    }
}

// ============================================================
// 残像の輪を生成
// ============================================================
static void CreateAfterimage(double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;

    pSet->count = 0;
    pSet->patternFunc = ShotAfterimage;
    pSet->x = x;
    pSet->y = y;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    // 16個の中玉で残像の輪を作る
    for (int i = 0; i < 16; i++) {
        sEnemyShot* pShot = new sEnemyShot;

        double angle = DX_PI * 2.0 * i / 16.0;

        pShot->muki = angle;
        pShot->speed = 0.55;
        pShot->kind = img_enemyShotMediumBall[3];

        // 輪の初期半径
        pShot->param_d[0] = 8.0;

        pShot->x = x + cos(angle) * pShot->param_d[0];
        pShot->y = y + sin(angle) * pShot->param_d[0];

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ============================================================
// 転位した軌跡を生成
// ============================================================
static void CreateWarpLine(
    double x0, double y0,
    double x1, double y1)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;

    pSet->count = 0;
    pSet->patternFunc = ShotWarpLine;
    pSet->x = x0;
    pSet->y = y0;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    double dx = x1 - x0;
    double dy = y1 - y0;
    double len = sqrt(dx * dx + dy * dy);
    double angle = atan2(dy, dx);

    // 軌跡上に並ぶ菱形弾
    int num = (int)(len / 24.0);

    if (num < 4)
        num = 4;

    if (num > 24)
        num = 24;

    for (int i = 0; i < num; i++) {
        sEnemyShot* pShot = new sEnemyShot;

        double t = (double)i / (double)(num - 1);

        pShot->param_d[0] = t;
        pShot->param_d[1] = x0;
        pShot->param_d[2] = y0;
        pShot->param_d[3] = angle;
        pShot->param_d[4] = len;

        pShot->speed = 0.35;

        // 白い菱形弾
        pShot->kind = img_enemyShotDiamond[6];

        pShot->x = x0 + dx * t;
        pShot->y = y0 + dy * t;
        pShot->muki = angle;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_Warp_ChatGPT()
{
    // 8つの転位地点
    static const double posX[8] = {
        120.0, 240.0, 360.0, 400.0,
        360.0, 240.0, 120.0, 80.0
    };

    static const double posY[8] = {
        90.0, 60.0, 90.0, 200.0,
        320.0, 400.0, 320.0, 200.0
    };

    static int warpIndex;
    static int warpDirection;
    static double oldX;
    static double oldY;

    // --------------------------------------------------------
    // 初期化
    // --------------------------------------------------------
    if (count == 1) {
        enemy.maxHp = enemy.hp = 200;

        warpIndex = 0;
        warpDirection = 1;

        enemy.x = posX[0];
        enemy.y = posY[0];

        oldX = enemy.x;
        oldY = enemy.y;
    }

    // --------------------------------------------------------
    // 45フレームごとに瞬間移動
    // --------------------------------------------------------
    if (count > 1 && count % 45 == 1) {
        double nextX;
        double nextY;

        int nextIndex = warpIndex + warpDirection;

        // 端まで行ったら折り返す
        if (nextIndex >= 7) {
            nextIndex = 7;
            warpDirection = -1;
        }
        else if (nextIndex <= 0) {
            nextIndex = 0;
            warpDirection = 1;
        }

        nextX = posX[nextIndex];
        nextY = posY[nextIndex];

        // 瞬間移動前の場所に残像を残す
        CreateAfterimage(oldX, oldY);

        // 移動前と移動後を結ぶ軌跡を生成
        CreateWarpLine(oldX, oldY, nextX, nextY);

        // 瞬間移動
        enemy.x = nextX;
        enemy.y = nextY;

        oldX = nextX;
        oldY = nextY;

        warpIndex = nextIndex;

        // 転位音
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);

        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
}