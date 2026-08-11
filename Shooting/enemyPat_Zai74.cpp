// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------
// ヘルパー関数：弾幕セットの生成とリストへの追加
// ---------------------------------------------------------
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind)
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

    return pSet;
}

// ---------------------------------------------------------
// ヘルパー関数：個別の弾を生成して指定したセットのリストに追加
// ---------------------------------------------------------
static void AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}


// ---------------------------------------------------------
// 弾幕パターン①：タラコの登場と分裂
// ---------------------------------------------------------
static void ShotTarako(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // タラコ本体（オレンジの大玉）を下に発射
        AddShot(pSet, pSet->x, pSet->y, DX_PI / 2.0, 2.0, img_enemyShotLargeBall[8]);
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // y座標が200を超えたら分裂（大玉は1つしか生成していないのでこれに該当）
        if (pShot->param_i[0] == 0 && pShot->y >= 200.0) {
            pShot->param_i[0] = 1; // 分裂済みフラグ
            pShot->speed = 9999.0; // 画面外へ強制排出（メインルーチンの消去処理に任せる）

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            // 赤い粒（小玉）を100個ランダムに散開
            for (int i = 0; i < 100; i++) {
                // GetRand(628) は 0〜628 の 629種類。100.0で割ると 0.00〜6.28 (おおよそ2π) になる
                double angle = (double)GetRand(628) / 100.0;
                // GetRand(300) は 0〜300。100を足して100.0で割ると 1.0〜4.0 になる
                double spd = (100 + GetRand(300)) / 100.0;
                AddShot(pSet, pShot->x, pShot->y, angle, spd, img_enemyShotSmallBall[0]);
                pSet->pEnemyShotHead->prev->param_i[0] = 1;
            }
        }

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 弾幕パターン②：波打つ極細麺
// ---------------------------------------------------------
static void ShotNoodle(sEnemyShotSet* pSet)
{
    if (pSet->count <= 100 && pSet->count % 5 == 0) {
        if (pSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 左から右へ向かう麺（黄色い銃弾）
        for (int i = 0; i < 5; i++) {
            double baseY = 80.0 + i * 80.0 + 30;
            AddShot(pSet, -20.0, baseY, 0.0, 3.0, img_enemyShotBullet[1]);
            // 追加した直後の弾(pEnemyShotHead->prev)に、波打つ際の基準Y座標を記録
            pSet->pEnemyShotHead->prev->param_d[0] = baseY;
            pSet->pEnemyShotHead->prev->margin = 200;
        }

        // 右から左へ向かう麺（少しYをずらして交差させる）
        for (int i = 0; i < 5; i++) {
            double baseY = 120.0 + i * 80.0 + 30;
            AddShot(pSet, 500.0, baseY, DX_PI, 3.0, img_enemyShotBullet[1]);
            pSet->pEnemyShotHead->prev->param_d[0] = baseY;
            pSet->pEnemyShotHead->prev->margin = 200;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // X方向は等速直線運動
        pShot->x += pShot->speed * cos(pShot->muki);
        // Y方向はサイン波で上下に波打つ（振幅30ピクセル、周期はおおよそ63フレーム）
        pShot->y = pShot->param_d[0] + sin(pShot->count * 0.1) * 30.0;

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 弾幕パターン③：パラパラのトッピング
// ---------------------------------------------------------
static void ShotTopping(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 緑の小玉（シソなど）をパラパラ降らす
        for (int i = 0; i < 50; i++) {
            // GetRand(480) は 0〜480。画面幅いっぱいに散らす
            double spawnX = (double)GetRand(480);
            // GetRand(100) は 0〜100。マイナスを掛けて画面上に散らばし、出現タイミングをずらす
            double spawnY = -(double)GetRand(100);
            // GetRand(150) は 0〜150。50を足して100.0で割ると 0.5〜2.0 の速度になる
            double spd = (50 + GetRand(150)) / 100.0;
            AddShot(pSet, spawnX, spawnY, DX_PI / 2.0, spd, img_enemyShotSmallBall[2]);
            pSet->pEnemyShotHead->prev->margin = 200;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}


// ---------------------------------------------------------
// 敵本体のパターン
// ---------------------------------------------------------
void EnemyPat_TarakoSpaghetti_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    // ※お好みで敵をゆらさせたい場合は、ここにサンプルコードのような左右移動処理を追加してください

    const int T = 300;
    int countT = count % T;

    // タイミングに合わせて各弾幕セットを生成
    if (countT == 60) {
        CreateShotSet(ShotTarako, enemy.x, enemy.y, 0.0, 0);
    }
    if (countT == 100) {
        CreateShotSet(ShotNoodle, enemy.x, enemy.y, 0.0, 0);
    }
    if (countT == 200) {
        CreateShotSet(ShotTopping, enemy.x, enemy.y, 0.0, 0);
    }
}