// enemyPat_Ebbinghaus.cpp
// エビングハウス錯視をモチーフにした弾幕パターン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：エビングハウス・イリュージョンリング
static void ShotEbbinghaus(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int phase = pEnemyShotSet->count;

    if (phase == 0) {
        // 導入：予告音 + 中サイズ弾（中央の円役）を展開
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 中サイズ弾を5個展開（視覚的に大きい中央円）
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = i * (DX_PI * 2.0 / 5.0) + (pEnemyShotSet->muki * 0.5);
            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * 80.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * 60.0;
            pEnemyShot->muki = angle + DX_PI / 2.0; // 初期向き
            pEnemyShot->speed = 0.8; // ゆっくり移動
            pEnemyShot->kind = img_enemyShotMediumBall[2]; // 中玉、黄色系
            pEnemyShot->param_i[0] = i; // 識別用
            pEnemyShot->param_d[0] = angle; // 元の角度保持
            pEnemyShot->margin = 100;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    else if (phase == 30) {
        // 錯視誘発：各中サイズ弾の周りに極小弾を密集配置
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pCenter = pEnemyShotSet->pEnemyShotHead->next;
        while (pCenter != pEnemyShotSet->pEnemyShotHead) {
            if (pCenter->kind == img_enemyShotMediumBall[2]) {
                double centerAngle = pCenter->param_d[0];
                // 各中サイズ弾周りに14個の極小弾（小さい周囲円）
                for (int j = 0; j < 14; j++) {
                    pEnemyShot = new sEnemyShot;
                    double offsetAngle = j * (DX_PI * 2.0 / 14.0);
                    pEnemyShot->x = pCenter->x + cos(offsetAngle) * 28.0; // 密集
                    pEnemyShot->y = pCenter->y + sin(offsetAngle) * 28.0;
                    pEnemyShot->muki = offsetAngle + centerAngle;
                    pEnemyShot->speed = 1.2;
                    pEnemyShot->kind = img_enemyShotSmallBall[4]; // 小玉、青系
                    pEnemyShot->param_i[0] = pCenter->param_i[0]; // 親のID
                    pEnemyShot->margin = 100;

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
            pCenter = pCenter->next;
        }
    }
    else if (phase == 80) {
        // 外側リング（大きい周囲円で錯覚を強調）
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = i * (DX_PI * 2.0 / 24.0);
            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * 140.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * 110.0;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.6;
            pEnemyShot->kind = img_enemyShotSmallBall[1]; // 小玉で薄く
            pEnemyShot->param_i[0] = -1; // 外側フラグ
            pEnemyShot->margin = 100;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    else if (phase >= 110 && phase % 4 == 0) {
        // 崩し：全弾を放射・移動開始
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->speed < 2.0) {
                pShot->speed = 2.5 + (GetRand(80) / 100.0);
                // プレイヤー方向へ加速
                double dx = player.x - pShot->x;
                double dy = player.y - pShot->y;
                double len = sqrt(dx * dx + dy * dy);
                if (len > 0.1) {
                    pShot->muki = atan2(dy, dx) + (GetRand(40) - 20) * 0.01;
                }
            }
            pShot = pShot->next;
        }
    }

    // 全弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 中サイズ弾はゆっくり回転
        if (pShot->kind == img_enemyShotMediumBall[2] && phase < 100) {
            pShot->muki += 0.015;
            double baseAngle = pShot->param_d[0];
            pShot->x = pEnemyShotSet->x + cos(baseAngle + phase * 0.008) * 75.0;
            pShot->y = pEnemyShotSet->y + sin(baseAngle + phase * 0.008) * 55.0;
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Ebbinghaus_Grok()
{
    static int muki = 1;
    static int shot_count = 0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右ゆっくり移動
        enemy.x += 1.1 * (double)muki;
        if (count % 140 == 70) muki *= -1;
    }

    // 定期的にパターン発動
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotEbbinghaus;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}