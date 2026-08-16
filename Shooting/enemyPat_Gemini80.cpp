// enemyPat_NorthWindAndSun.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 北風フェーズの弾幕パターン（約6秒間）
static void ShotNorthWind(sEnemyShotSet* pSet)
{
    // フェーズ中（0～359フレーム）のみ弾を追加生成
    if (pSet->count < 360) {
        // 1. 上から降ってくる風弾（押し流し効果を演出）
        if (pSet->count % 4 == 0) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = GetRand(480);
            pShot->y = 0.0;
            // 真下より±10度ランダムにブレさせる
            pShot->muki = DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
            pShot->speed = (300 + GetRand(200)) / 100.0; // 速度3.0 ～ 5.0

            // 色は白(6)とシアン(3)の風を交ぜる。形は流線形の中楕円弾
            pShot->kind = (GetRand(1) == 0) ? img_enemyShotMediumOval[6] : img_enemyShotMediumOval[3];

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // 2. ボスからの自機狙いWay弾（氷の針）
        if (pSet->count % 45 == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            double baseMuki = atan2(player.y - pSet->y, player.x - pSet->x);
            // 5Way
            for (int i = 0; i < 5; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = baseMuki + (i - 2) * (15.0 / 180.0 * DX_PI); // 15度間隔
                pShot->speed = 4.5;
                pShot->kind = img_enemyShotDiamond[4]; // 青(4)の菱形弾

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 生成した全弾の座標更新（フェーズ終了後も画面に残る限り実行される）
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// 太陽フェーズの弾幕パターン（約6秒間）
static void ShotSun(sEnemyShotSet* pSet)
{
    // フェーズ開始時にチャージ音
    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // フェーズ中（0～359フレーム）のみ弾を追加生成
    if (pSet->count < 360) {
        // 1. ボスからの全方位リング弾（太陽フレア）
        if (pSet->count % 60 == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            double baseMuki = (GetRand(360) / 180.0) * DX_PI; // ランダムな基準角
            int way = 16;
            for (int i = 0; i < way; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = baseMuki + i * (DX_PI * 2.0 / way);
                pShot->speed = 1.2; // 遅め
                // 橙(8)と赤(0)の大玉を交ぜる
                pShot->kind = (i % 2 == 0) ? img_enemyShotLargeBall[8] : img_enemyShotLargeBall[0];

                // param_i[0] を種類判別に使用 (0:大玉, 1:熱波)
                pShot->param_i[0] = 0;

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }

        // 2. 画面下部からせり上がる熱波弾（陽炎）
        if (pSet->count % 6 == 0) {
            // 1フレームに複数個出す
            for (int i = 0; i < 3; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = GetRand(480);
                pShot->y = 480.0; // 画面下端から発生
                pShot->muki = -DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI; // 真上より±10度ブレ
                pShot->speed = (100 + GetRand(150)) / 100.0; // 速度1.0 ～ 2.5
                pShot->kind = (GetRand(1) == 0) ? img_enemyShotMediumBall[8] : img_enemyShotMediumBall[0]; // 橙か赤

                // param_i[0] を種類判別に使用 (0:大玉, 1:熱波)
                pShot->param_i[0] = 1;

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 生成した全弾の座標更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 熱波弾(param_i[0] == 1)の場合、ボスの足元の安全地帯（y=160付近）より上に行ったら消す
        if (pShot->param_i[0] == 1) {
            if (pShot->y < 160.0) {
                // メインルーチンの画面外消去に回収させるため、遠くへ飛ばす
                pShot->x = -1000.0;
                pShot->y = -1000.0;
            }
        }

        pShot = pShot->next;
    }
}


// 敵本体のパターン（北風と太陽）
void EnemyPat_NorthWindAndSun_Gemini()
{
    // 初期化
    if (count == 1) {
        // 画面上部中央に固定
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // ボス用体力
    }

    // フェーズの周期管理 (1周期720フレーム = 約12秒)
    // 最初のフレーム(count==1)を0として扱う
    int phaseCount = (count - 1) % 720;

    // 北風フェーズ開始 (周期の0フレーム目)
    if (phaseCount == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotNorthWind;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 太陽フェーズ開始 (周期の360フレーム目)
    if (phaseCount == 360) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSun;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}