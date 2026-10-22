#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 焼き栗の弾を追加する共通処理
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet,
    double x, double y,
    double muki, double speed,
    int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// 焼き栗が割れて、殻の破片と火の粉が一斉に飛び散る
static void BurstChestnut(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
{
    const double baseMuki = pEnemyShot->muki;

    // 同じ栗の集団では爆発音を一度だけ鳴らす
    if (pEnemyShotSet->param_i[0] == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        pEnemyShotSet->param_i[0] = 1;
    }

    // 栗本体を橙色の小玉へ変化させ、破片の一つとして再利用する
    pEnemyShot->kind = img_enemyShotSmallBall[8];
    pEnemyShot->speed = 2.8;
    pEnemyShot->muki = baseMuki + DX_PI;
    pEnemyShot->param_i[0] = 1;

    // 焼けた栗の火の粉
    for (int i = 0; i < 14; i++) {
        double angle = DX_PI * 2.0 * (double)i / 14.0;
        angle += (GetRand(20) - 10) / 100.0;
        double speed = 1.9 + GetRand(80) / 100.0;
        int kind = (i & 1) ? img_enemyShotSmallBall[0] : img_enemyShotSmallBall[1];
        AddShot(pEnemyShotSet,
            pEnemyShot->x,
            pEnemyShot->y,
            angle,
            speed,
            kind);
    }

    // 殻が左右へ裂けるように、細長い菱形弾を大きく開いて飛ばす
    for (int side = -1; side <= 1; side += 2) {
        double angle = baseMuki + side * (0.95 + GetRand(20) / 100.0);
        sEnemyShot* pShard = AddShot(pEnemyShotSet,
            pEnemyShot->x,
            pEnemyShot->y,
            angle,
            3.6,
            img_enemyShotDiamond[8]);
        pShard->param_i[1] = 1;
    }

    // 栗の割れ目から勢いよく飛ぶ大きめの破片
    for (int side = -1; side <= 1; side += 2) {
        double angle = baseMuki + side * 0.42;
        sEnemyShot* pShell = AddShot(pEnemyShotSet,
            pEnemyShot->x,
            pEnemyShot->y,
            angle,
            2.7,
            img_enemyShotMediumOval[8]);
        pShell->param_i[0] = 2; // 中楕円弾だが「未爆発の栗」ではない
    }
}

// 栗そのものを落とし、止まりかけたところで爆ぜさせる
static void ShotRoastedChestnut(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 一列に積み上がった焼き栗をイメージした、少しずつずれる扇状配置
        const int chestnutNum = 7+4;
        for (int i = 0; i < chestnutNum; i++) {
            double center = (chestnutNum - 1) * 0.5;
            double xOffset = (i - center) * 42.0;
            double angle = DX_PI * 0.5 + (i - center) * 0.055;
            double speed = 1.8 + GetRand(40) / 100.0;

            sEnemyShot* pChestnut = AddShot(pEnemyShotSet,
                pEnemyShotSet->x + xOffset,
                pEnemyShotSet->y + 18.0,
                angle,
                speed,
                img_enemyShotMediumOval[8]);
            pChestnut->param_i[0] = 0;
            // 外側ほど早め、中央ほど遅めに爆ぜることで時間差を作る
            int offsetIndex = i - (int)center;
            if (offsetIndex < 0) offsetIndex = -offsetIndex;
            pChestnut->param_i[1] = 58 + offsetIndex * 9;
            pChestnut->param_i[2] = i & 1;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        // 栗本体：落下してから一瞬だけ減速し、その後に爆ぜる
        if (pShot->kind == img_enemyShotMediumOval[8] && pShot->param_i[0] == 0) {
            int burstCount = pShot->param_i[1];

            if (pShot->count < burstCount - 12) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (pShot->count < burstCount) {
                // 焼けて膨らみ、弾ける直前にほぼ停止
                pShot->speed *= 0.88;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                BurstChestnut(pEnemyShotSet, pShot);
            }
        }
        // 爆ぜた後の弾は通常移動
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 殻の菱形弾だけ、わずかに向きを変えて飛び散る感じを強める
            if (pShot->param_i[1] == 1 && pShot->count < 18) {
                pShot->muki += (pShot->muki < 0.0 ? -0.002 : 0.002);
            }
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_RoastChestnut_ChatGPT()
{
    static int muki;
    static int wave;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        wave = 0;
    }
    else {
        // 焼き栗を鍋の中で転がすように、ボスを左右へ往復させる
        enemy.x += 0.85 * (double)muki;
        if (enemy.x < 75.0) muki = 1;
        if (enemy.x > 405.0) muki = -1;
    }

    // 一定間隔で新しい栗の列を投下
    if (count % 150 == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRoastedChestnut;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 22.0;
        pEnemyShotSet->muki = DX_PI * 0.5;
        pEnemyShotSet->kind = wave++;
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}