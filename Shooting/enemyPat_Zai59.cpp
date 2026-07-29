// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：三三七拍子（高難易度版）
static void ShotSanSanNana_Hard(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        int kind = pEnemyShotSet->kind;
        int seq = pEnemyShotSet->param_i[0]; // 3連撃の何発目か (0, 1, 2)

        // 種類に応じた効果音の再生（重複防止のため一旦ストップ）
        if (kind == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        else if (kind == 1) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        else if (kind == 2) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // ----- 3拍 (ドン・ドン・ドン) -----
        if (kind == 0) {
            // 赤い大玉を2発、ランダムな角度で発射
            for (int i = 0; i < 2*3; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // ±5度のランダムな角度ズレ（GetRand(10)は0〜10を返すので、-5〜+5になる）
                double offset = (GetRand(10) - 5) / 180.0 * DX_PI*3;
                pEnemyShot->muki = pEnemyShotSet->muki + offset;
                pEnemyShot->speed = 2.0; // 少し速度アップ
                pEnemyShot->kind = img_enemyShotLargeBall[0]; // 0:赤、大玉

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        // ----- 3拍 (カツ・カツ・カツ) -----
        else if (kind == 1) {
            // 青い小玉を5発、ランダムな角度でばら撒く
            for (int i = 0; i < 5*3; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // ±15度のランダムな角度ズレ（GetRand(30)は0〜30を返すので、-15〜+15になる）
                double offset = (GetRand(30) - 15) / 180.0 * DX_PI*3;
                pEnemyShot->muki = pEnemyShotSet->muki + offset;
                pEnemyShot->speed = 5.5; // 高速
                pEnemyShot->kind = img_enemyShotSmallBall[4]; // 4:青、小玉

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        // ----- 7拍目 (ドンドンドン！) -----
        else if (kind == 2) {
            // 黄色い菱形弾を11方向に扇状発射
            // 連射するたびに全体のベース角度を5度ずつずらして交差させる
            double baseRotOffset = seq * 5.0 / 180.0 * DX_PI/2;

            for (int i = 0; i < 11*5; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->kind = img_enemyShotDiamond[1]; // 1:黄、菱形弾
                pEnemyShot->speed = 3.5;
                // 10度刻み（PI/18）で11発（-50度〜+50度）
                pEnemyShot->muki = pEnemyShotSet->muki + baseRotOffset + (i - 5*5-1) * (DX_PI / 18.0)/2;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 全弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_337Beat_Zai()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 1ループを240フレームとし、1拍=30フレームで構成
    int rhythm = (count - 1) % 240;
    int kind = -1;
    int seq = 0; // 連撃のインデックス (0, 1, 2)

    // 3拍 (ドン・ドン・ドン)
    if (rhythm == 0 || rhythm == 30 || rhythm == 60) {
        kind = 0;
        seq = rhythm / 30;
    }
    // 3拍 (カツ・カツ・カツ)
    else if (rhythm == 90 || rhythm == 120 || rhythm == 150) {
        kind = 1;
        seq = (rhythm - 90) / 30;
    }
    // 7拍目 (タン・タン・タン)
    else if (rhythm == 180 || rhythm == 190 || rhythm == 200) {
        kind = 2;
        seq = (rhythm - 180) / 10;
    }

    // 拍子のタイミングで弾幕セットを生成
    if (kind >= 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSanSanNana_Hard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = kind;
        pEnemyShotSet->param_i[0] = seq; // 3連撃の何発目かをパラメータとして渡す

        // 循環リストのダミーノード生成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルの弾幕セットリストに接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}