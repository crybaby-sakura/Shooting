//---------------------------------------------------------------
// enemyPat_Tmp.cpp
// 弾幕: 焼き栗「いがぐり開殻」
//
// 既存弾の組み合わせで焼き栗を表現する
//   大玉(橙)   = 焼いた栗の実
//   小玉(黒)   = いが(鬼板)のトゲ
//   鱗弾(黄)   = 開殻時にこぼれる中身
//   中楕円弾(黄) = 加熱中に殻の隙間から覗く実
//
// 流れ:
//   [形成]  栗の実+トゲを円形に静止配置(いがぐり)
//   [加熱]  実が脈動し、中身がゆっくりこぼれはじめる(予告)
//   [開殻]  実が2つに割れて飛び、トゲが波状で飛散、
//           中身が全方位に放出される
//---------------------------------------------------------------
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const double TOGE_RADIUS = 30.0;   // いが(トゲ)の配置半径

// 弾を生成してセットの弾リストへ追加
static void AddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind, int role)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = role;   // 0=栗の本体 1=トゲ 2=中身 3=殻の半分
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// 効果音再生(二重再生防止付き)
static void PlayShotSound(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

// ------------------------------------------------------------
// 弾幕本体: いがぐり開殻
// ------------------------------------------------------------
static void ShotChestnut(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    sEnemyShot* pNext;

    // ---- 形成フェーズ: いがぐりを作る ----
    if (pSet->count == 0) {
        PlayShotSound(sound_enemyCharge);   // 予告音

        int togeNum = (12 + (pSet->kind % 3) * 2)*2;   // 12/14/16個が巡回

        // 栗の実(大玉・橙)を中心に配置
        AddShot(pSet, pSet->x, pSet->y, 0.0, 0.0, img_enemyShotLargeBall[8], 0);

        // いがのトゲ(小玉・黒)を円形に静止配置
        for (int i = 0; i < togeNum; i++) {
            double angle = i / (double)togeNum * 2.0 * DX_PI;
            AddShot(pSet,
                pSet->x + cos(angle) * TOGE_RADIUS,
                pSet->y + sin(angle) * TOGE_RADIUS,
                0.0, 0.0, img_enemyShotSmallBall[7], 1);
            // 放射方向と発射遅延を記録(遅延で波状に飛び散らせる)
            sEnemyShot* pToge = pSet->pEnemyShotHead->prev;
            pToge->param_d[0] = angle;
            pToge->param_i[1] = i;
            pToge->margin = 80;
        }
    }

    // ---- 加熱開始(焼き始め) ----
    if (pSet->count == 120) {
        PlayShotSound(sound_enemyShot_light);
    }

    // ---- 加熱中: 実がこぼれはじめる(開殻の予告を兼ねる) ----
    if (pSet->count >= 120 && pSet->count < 200 && pSet->count % 4 == 0) {
        double a = GetRand(360) / 180.0 * DX_PI;
        AddShot(pSet, pSet->x, pSet->y, a, 0.8, img_enemyShotMediumOval[1], 2);
    }

    // ---- 開殻: パカッと割れる ----
    if (pSet->count == 200) {
        PlayShotSound(sound_enemyShot_heavy);

        double diff = pSet->param_d[0];   // 難易度倍率

        // 栗の本体(役割0)を削除
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pNext = pShot->next;
            if (pShot->param_i[0] == 0) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
            pShot = pNext;
        }

        // 殻が左右2つに割れてはじけ飛ぶ(大玉)
        AddShot(pSet, pSet->x - 8.0, pSet->y,
            -DX_PI / 2.0 - 0.5, 1.5 * diff, img_enemyShotLargeBall[8], 3);
        AddShot(pSet, pSet->x + 8.0, pSet->y,
            -DX_PI / 2.0 + 0.5, 1.5 * diff, img_enemyShotLargeBall[8], 3);

        // 中身(鱗弾・黄)が全方位にこぼれる(放射状なので避け道あり)
        for (int i = 0; i < 12*3; i++) for (int j = 0; j < 3; j++) {
            double a = i / 12.0/3 * 2.0 * DX_PI
                + (GetRand(20) - 10) / 180.0 * DX_PI;
            AddShot(pSet, pSet->x, pSet->y, a,
                (2.2 + GetRand(100) / 100.0) * diff + j,
                img_enemyShotScale[1], 2);
        }
    }

    // ---- 各弾の挙動 ----
    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pNext = pShot->next;   // ループ中に削除するため先に保存
        switch (pShot->param_i[0]) {
        case 0:     // 栗の本体: 加熱中に脈動(焼けている演出&開殻予告)
            if (pSet->count >= 120) {
                double pulse = sin((pSet->count - 120) * 0.25);
                pShot->x = pSet->x + pulse * 4.0;
                pShot->y = pSet->y - pulse * 4.0;
            }
            break;
        case 1:     // いがのトゲ: 予告→波状で飛散
            if (pShot->speed == 0.0) {
                if (pSet->count >= 200 + pShot->param_i[1]) {
                    // 飛び散る! 角度に±15°のランダム幅
                    pShot->speed = (3.5 + GetRand(150) / 100.0) * pSet->param_d[0]+1;
                    pShot->muki = pShot->param_d[0]
                        + (GetRand(30) - 15) / 180.0 * DX_PI;
                }
                else if (pSet->count >= 160) {
                    // 開殻直前の微振動(明滅予告)
                    pShot->x = pSet->x + cos(pShot->param_d[0]) * TOGE_RADIUS
                        + (GetRand(3) - 1);
                    pShot->y = pSet->y + sin(pShot->param_d[0]) * TOGE_RADIUS
                        + (GetRand(3) - 1);
                }
            }
            else {
                pShot->kind = img_enemyShotLaser[7];
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            break;
        default:    // 中身(2)・殻の半分(3): 等速直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }
        pShot = pNext;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Tmp()
{
    static int muki;
    static int shotCount;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;   // 200で固定
        muki = 1;
        shotCount = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // いがぐりを定期的に生成(発生位置は左・中央・右を巡回)
    if (count >= 30 && count % 180 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotChestnut;
        pEnemyShotSet->x = 120.0 + (shotCount % 3) * 120.0;
        pEnemyShotSet->y = 120.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shotCount++;
        // ループごとに弾速が上がる(難易度上昇) 上限1.5倍
        double diff = 1.0 + (pEnemyShotSet->kind % 5) * 0.125;
        pEnemyShotSet->param_d[0] = diff;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}