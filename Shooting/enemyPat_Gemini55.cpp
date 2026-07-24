// enemyPat_KaleidoPhoenix.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ====================================================================
//  内部ヘルパー関数
// ====================================================================

// 弾を生成してShotSetに繋ぐ
static sEnemyShot* CreateEnemyShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind) {
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;

    // リスト末尾に挿入
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;

    return pShot;
}

// 1セット分の弾幕を登録する
// param_i0, param_d0 は回転方向や角度などを渡すための汎用パラメータ
static void RegisterEnemyShotSet(void (*func)(sEnemyShotSet*), int param_i0 = 0, double param_d0 = 0.0) {
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = enemy.x; // 登録時の敵座標
    pSet->y = enemy.y;
    pSet->param_i[0] = param_i0;
    pSet->param_d[0] = param_d0;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}


// ====================================================================
//  各弾幕レイヤー（1セット分の振る舞い）
// ====================================================================

// ① 弾幕：万華鏡の輪（1フレームで5発だけ撃ち、あとは移動のみ）
static void ShotKaleidoPhoenix_Ring(sEnemyShotSet* pSet) {
    // 登録された最初の1フレームだけ発射する
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int dir = pSet->param_i[0];          // 1(右回転) or -1(左回転)
        double base_angle = pSet->param_d[0]; // メインから渡された基準角度

        int ways = 5;
        for (int i = 0; i < ways; i++) {
            double angle = base_angle + i * (DX_PI * 2.0 / ways);
            int kind = (i % 2 == 0) ? img_enemyShotLargeBall[3] : img_enemyShotMediumBall[4]; // シアン・青

            sEnemyShot* pShot = CreateEnemyShot(pSet, pSet->x, pSet->y, angle, 1.0, kind);
            pShot->param_i[0] = dir; // 弾自身に回転方向を記憶させる
        }
    }

    // 弾の移動処理（撃ち切った後も、弾が消えるまで毎回呼ばれる）
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->count < 150) {
            pShot->muki += 0.003 * pShot->param_i[0];
            pShot->speed += 0.01;
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// ② 弾幕：朱色の翼（60フレームかけて1回羽ばたく）
static void ShotKaleidoPhoenix_Wing(sEnemyShotSet* pSet) {
    int c = pSet->count;
    int dir = pSet->param_i[0]; // 1(右翼) or -1(左翼)

    // 60フレームの間だけ、4フレーム間隔で弾を撃ち出す
    if (c % 4 == 0 && c <= 60) {
        if (c % 8 == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        double base_angle = DX_PI / 2.0;
        double spread_angle = base_angle - dir * (DX_PI / 2.0) * (c / 40.0);

        for (int i = 0; i < 4; i++) {
            double speed = 1.5 + i * 0.6 + (GetRand(20) - 10) / 100.0;
            int color = (i % 2 == 0) ? 8 : 0; // 橙・赤

            // 本体から常に撃ち出すため、現在のenemy.x/yを使う
            sEnemyShot* pShot = CreateEnemyShot(pSet, enemy.x, enemy.y, spread_angle, speed, img_enemyShotScale[color]);
            pShot->param_i[0] = dir;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->count < 30) {
            pShot->speed -= 0.03;
            if (pShot->speed < 0.5) pShot->speed = 0.5;
        }
        else {
            pShot->speed += 0.02;
            pShot->muki += 0.003 * pShot->param_i[0]; // 外側に開くように曲がる
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// ③ 弾幕：黄金レーザー（150フレームかけて予告〜発射までを行う）
static void ShotKaleidoPhoenix_Laser(sEnemyShotSet* pSet) {
    int c = pSet->count;

    // 予告音
    if (c == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    // 発射音
    if (c == 120) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 予告線（60〜119フレームの間パラパラと細いレーザーを撃つ）
    if (c >= 60 && c < 120 && c % 10 == 0) {
        for (int angle_idx = -1; angle_idx <= 1; angle_idx++) {
            double base_angle = DX_PI / 2.0 + angle_idx * 0.6;
            CreateEnemyShot(pSet, enemy.x, enemy.y, base_angle, 25.0, img_enemyShotLaser[1]); // 1(黄)
        }
    }

    // 極太レーザー本射（120〜150フレームの間だけ発射）
    if (c >= 120 && c <= 150) {
        for (int angle_idx = -1; angle_idx <= 1; angle_idx++) {
            double base_angle = DX_PI / 2.0 + angle_idx * 0.6;
            for (int w = -3; w <= 3; w++) { // 横に太く並べる
                double offset_x = cos(base_angle + DX_PI / 2.0) * (w * 4.0);
                double offset_y = sin(base_angle + DX_PI / 2.0) * (w * 4.0);
                CreateEnemyShot(pSet, enemy.x + offset_x, enemy.y + offset_y, base_angle, 15.0, img_enemyShotLaser[1]);
            }
        }
    }

    // 弾の移動処理（レーザーなので直進のみ）
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// ====================================================================
//  敵本体のパターン（コントローラー）
// ====================================================================

void EnemyPat_ThumbnailFriendly_Gemini()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // ボス本体はゆっくりと左右・上下に揺れる
    enemy.x = 240.0 + 20.0 * sin(count / 120.0);
    enemy.y = 80.0 + 10.0 * cos(count / 60.0);

    // --- 各弾幕セットを周期的に呼び出し（発射命令） ---

    // ① 黄金レーザー (300フレーム周期)
    // カウントの区切りがいいタイミングで登録
    if (count % 300 == 150) {
        RegisterEnemyShotSet(ShotKaleidoPhoenix_Laser);
    }

    // ② 朱色の翼 (120フレーム周期)
    // 左右の羽ばたきを1セットとして登録
    if (count % 120 == 10) {
        RegisterEnemyShotSet(ShotKaleidoPhoenix_Wing, 1); // 右翼用
        RegisterEnemyShotSet(ShotKaleidoPhoenix_Wing, -1); // 左翼用
    }

    // ③ 万華鏡の輪 (10フレーム周期)
    // 細かい間隔で短いShotSetを大量に登録し、渦巻きを作る
    if (count % 15 == 0) {
        double angle_offset = count * 0.015; // 時間経過で発射角をずらす
        RegisterEnemyShotSet(ShotKaleidoPhoenix_Ring, 1, angle_offset); // 右回転用
        RegisterEnemyShotSet(ShotKaleidoPhoenix_Ring, -1, -angle_offset); // 左回転用
    }
}