// enemyPat_slither.cpp
// 弾幕：スリザースネーク

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 蛇の中心線。時間 t を与えると蛇の頭／胴体の位置を決める。
// 頭が通った軌跡を少しずつ遅れて小玉がたどることで、蛇を表現する。
static void GetSnakePos(int t, double& x, double& y, int kind)
{
	double tt = (double)t * 2.0;
	double a;
	double r;

	switch (kind)
	{
	case 0:
		a = 0.0105 * tt;
		r = 155.0 + 62.0 * sin(0.0065 * tt);
		x = 240.0 + r * cos(a) + 38.0 * sin(0.017 * tt);
		y = 238.0 + 0.72 * r * sin(a) + 26.0 * sin(0.011 * tt + 1.2) + 100;
		break;

	case 1:
		a = 0.0130 * tt;
		r = 135.0 + 78.0 * sin(0.0080 * tt);
		x = 240.0 + r * cos(a) + 52.0 * sin(0.021 * tt + 0.7);
		y = 238.0 + 0.65 * r * sin(a) + 32.0 * sin(0.014 * tt + 2.0);
		break;

	case 2:
		a = 0.0085 * tt;
		r = 175.0 + 48.0 * sin(0.0055 * tt + 1.0);
		x = 240.0 + r * cos(a) + 30.0 * sin(0.013 * tt + 1.8);
        y = 238.0 + 0.80 * r * sin(a) + 44.0 * sin(0.009 * tt + 0.3) - 100;
		break;
	}
}

static double GetSnakeAngle(int t, int kind)
{
    double x1, y1, x2, y2;
    GetSnakePos(t, x1, y1, kind);
    GetSnakePos(t + 1, x2, y2, kind);
    return atan2(y2 - y1, x2 - x1);
}

static void CreateSnake(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 頭：大玉で強調
        pEnemyShot = new sEnemyShot;
        pEnemyShot->kind = img_enemyShotLargeBall[5];
        pEnemyShot->speed = 0.0;
        pEnemyShot->param_i[0] = 0;
        pEnemyShot->margin = 240;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 頭の現在位置
    double hx, hy;
    GetSnakePos(pEnemyShotSet->count, hx, hy, pEnemyShotSet->kind);
    pEnemyShotSet->x = hx;
    pEnemyShotSet->y = hy;
    pEnemyShotSet->muki = GetSnakeAngle(pEnemyShotSet->count, pEnemyShotSet->kind);

    // 先頭の大玉を更新
    sEnemyShot* pHead = pEnemyShotSet->pEnemyShotHead->next;
    if (pHead != pEnemyShotSet->pEnemyShotHead) {
        pHead->x = hx;
        pHead->y = hy;
        pHead->muki = pEnemyShotSet->muki;
    }

    // 時間とともに尾を伸ばす。小玉の連鎖で胴体を表現。
    if (pEnemyShotSet->count % 4 == 0 && pEnemyShotSet->count < 620*999) {
        pEnemyShot = new sEnemyShot;
        pEnemyShot->kind = img_enemyShotScale[(pEnemyShotSet->count / 4) % 6];
        pEnemyShot->speed = 0.0;
        pEnemyShot->param_i[0] = pEnemyShotSet->count;

        double bx, by;
        GetSnakePos(pEnemyShot->param_i[0], bx, by, pEnemyShotSet->kind);
        pEnemyShot->x = bx;
        pEnemyShot->y = by;
        pEnemyShot->muki = GetSnakeAngle(pEnemyShot->param_i[0], pEnemyShotSet->kind);

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 既存の胴体は「生成時の時刻」を基準に軌跡上へ再配置する。
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot != pHead) {
            int trailTime = pShot->param_i[0];
            double bx, by;
            GetSnakePos(trailTime, bx, by, pEnemyShotSet->kind);
            pShot->x = bx;
            pShot->y = by;
            pShot->muki = GetSnakeAngle(trailTime, pEnemyShotSet->kind);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Slitherio_ChatGPT()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 65.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 蛇そのものを敵弾セットとして画面上に構築
    if (count == 1) for (int k = 0; k < 3; k++) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = CreateSnake;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 240.0;
        pEnemyShotSet->muki = -DX_PI / 2.0;
        pEnemyShotSet->kind = k;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 敵本体は蛇の頭を追うように上部で待機
    enemy.x = 240.0 + 55.0 * sin(0.01 * (double)count);
    enemy.y = 45.0 + 15.0 * sin(0.017 * (double)count + 1.0);
}