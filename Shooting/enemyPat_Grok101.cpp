// enemyPat_Tmp.cpp
// ファランクスをモチーフにした密集方陣弾幕
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：密集方陣（ファランクス・ウォール）
// 小型弾で硬い盾壁を作り、ゆっくり前進＋左右揺れ。最前列から定期的に中型弾の「槍」を突き出す。
static void ShotPhalanx(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ：方陣生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 方陣の中心を shotSet の位置に設定
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;          // center_x
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y + 20.0;   // center_y

        const int cols = 13;
        const int rows = 6;
        const double hx = 13.0;   // 横間隔
        const double hy = 11.0;   // 縦間隔
        const double start_x = -(cols - 1) * hx * 0.5;
        const double start_y = -(rows - 1) * hy * 0.5;

        // 色：3 = シアン（盾らしい色）
        // 弾種：小玉
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->param_d[0] + start_x + c * hx;
                pEnemyShot->y = pEnemyShotSet->param_d[1] + start_y + r * hy;
                pEnemyShot->muki = DX_PI / 2.0;  // 下向き（参考値、実際の移動は上書き）
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[3];  // シアン小玉
                pEnemyShot->param_i[0] = r;                     // 行番号（0=後方、rows-1=最前列）
                pEnemyShot->param_i[1] = 1;                     // 1 = 方陣弾（移動を上書きする）
                pEnemyShot->margin = 240;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレーム：方陣の移動（共通の速度で剛体移動）
    // 左右にゆっくり揺れる + 下方向に一定速度
    double t = (double)pEnemyShotSet->count;
    double vx = 1.7 * cos(t * 0.032);   // 揺れの振幅と周期
    double vy = 1.45;                   // 前進速度

    // 中心も更新（槍の発射位置計算用）
    pEnemyShotSet->param_d[0] += vx;
    pEnemyShotSet->param_d[1] += vy;

    // 槍の突き出し（約1.6秒ごと）
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 100 == 40) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 現在の最前列の y を概算（中心 + 半分の高さ）
        const int rows = 6;
        const double hy = 11.0;
        double front_y = pEnemyShotSet->param_d[1] + (rows - 1) * hy * 0.5;
        double center_x = pEnemyShotSet->param_d[0];

        // 5本の槍を横に並べて高速で下へ
        for (int i = -2; i <= 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = center_x + i * 18.0;
            pEnemyShot->y = front_y;
            pEnemyShot->muki = DX_PI / 2.0;
            pEnemyShot->speed = 4.8;
            pEnemyShot->kind = img_enemyShotMediumBall[0];  // 赤中玉（槍）
            pEnemyShot->param_i[0] = -1;
            pEnemyShot->param_i[1] = 0;  // 0 = 通常弾（自身の speed/muki で移動）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾の位置更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[1] == 1) {
            // 方陣弾：共通速度で移動（相対位置を保つ）
            pShot->x += vx;
            pShot->y += vy;
        }
        else {
            // 槍などの通常弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Phalanx_Grok()
{
    static int muki;
    static int phase;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
    }
    else {
        // 敵自身はゆっくり左右に移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
    }

    // 一定間隔で新しい方陣を生成（前の方陣が下に進んだ頃に次を出す）
    if (count % 70 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPhalanx;
        pEnemyShotSet->x = enemy.x + ((count + 70) % 210 / 70 - 1) * 100.0;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = phase++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}