// EnemyPat_BeerSpray_Gemini.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =========================================================
// フェーズ1：シェイキング・ボトル（気泡エフェクト用）
// =========================================================
static void ShotShakingBubbles(sEnemyShotSet* pEnemyShotSet)
{
    // 常にボスの位置に追従
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // 60フレーム間、毎フレーム小玉を真上に向けて放出
    if (pEnemyShotSet->count < 60) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x + GetRand(40) - 20;
        pEnemyShot->y = pEnemyShotSet->y - 10 + GetRand(20) - 10;
        pEnemyShot->muki = -DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI; // 上方向
        pEnemyShot->speed = (150 + GetRand(150)) / 100.0;
        pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白い小玉（気泡）

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// =========================================================
// フェーズ2：プラグ・ポップ（王冠をイメージした高速弾）
// =========================================================
static void ShotPlugPop(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 自機狙いから少しだけ扇状にばらけさせる
            pEnemyShot->muki = pEnemyShotSet->muki + (i - 2) * 0.04;
            pEnemyShot->speed = 8.0; // 超高速
            pEnemyShot->kind = img_enemyShotDiamond[6]; // 白い菱形弾（王冠の代用）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// =========================================================
// フェーズ3：ファイン・フォーム（大量の泡ばら撒き）
// =========================================================
static void ShotFineFoam(sEnemyShotSet* pEnemyShotSet)
{
    // 常にボスの位置に追従
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // 200フレームの間、高頻度でばら撒く
    if (pEnemyShotSet->count < 200 && pEnemyShotSet->count % 2 == 0) {
        if (pEnemyShotSet->count % 10 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 一度に数発生成して圧倒的な弾幕に
        for (int i = 0; i < 4; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(20) - 10;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;
            // 下方向広範囲（140度程度）
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(140) - 70) / 180.0 * DX_PI;
            // 速度に大きな揺らぎ
            pEnemyShot->speed = (150 + GetRand(350)) / 100.0;

            int color = (GetRand(3) == 0) ? 6 : 1; // 1/4で黄色、3/4で白
            if (GetRand(1) == 0) {
                pEnemyShot->kind = img_enemyShotSmallBall[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[color];
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// =========================================================
// フェーズ4：ドロップ・レイン（重力落下する大玉）
// =========================================================
static void ShotDropRain(sEnemyShotSet* pEnemyShotSet)
{
    // 常にボスの位置に追従
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // ファイン・フォームと並行して、少し間隔をあけて発射
    if (pEnemyShotSet->count < 200 && pEnemyShotSet->count % 15 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x + GetRand(100) - 50; // 発射位置を広めにとる
        pEnemyShot->y = pEnemyShotSet->y - 20;
        // 上方向に打ち上げる
        pEnemyShot->muki = -DX_PI / 2.0 + (GetRand(80) - 40) / 180.0 * DX_PI;
        pEnemyShot->speed = (200 + GetRand(200)) / 100.0;
        pEnemyShot->kind = img_enemyShotLargeBall[1]; // 黄色の大玉

        // param_d[0] に Vx, param_d[1] に Vy を保存
        pEnemyShot->param_d[0] = pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->param_d[1] = pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 弾の移動（重力加算）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 重力加速（毎フレーム下方向の速度を加算）
        pShot->param_d[1] += 0.04;

        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];

        // 描画用に muki と speed を更新
        pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
        pShot->speed = sqrt(pow(pShot->param_d[0], 2) + pow(pShot->param_d[1], 2));

        pShot = pShot->next;
    }
}

// =========================================================
// 敵本体のパターン（歓喜のビア・シャワー）
// =========================================================
void EnemyPat_BeerSpray_Gemini()
{
    static double init_x, init_y;
    int cycle = count % 400; // 400フレーム周期の弾幕

    // 初回のみのステータス初期化
    if (count == 1) {
        enemy.maxHp = enemy.hp = 200;
        enemy.x = 240.0;
        enemy.y = 100.0;
        init_x = enemy.x;
        init_y = enemy.y;
    }

    // 周期開始時の基準位置を記憶
    if (cycle == 1) {
        init_x = enemy.x;
        init_y = enemy.y;
    }

    // ---------------------------------------------------------
    // フェーズ1：シェイキング・ボトル (cycle 1 ~ 60)
    // ---------------------------------------------------------
    if (cycle > 0 && cycle <= 60) {
        if (cycle == 1) {
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            // 気泡エフェクト用弾幕セット生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotShakingBubbles;
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
        // ボトルを振るように激しく振動
        enemy.x = init_x + (GetRand(6) - 3);
        enemy.y = init_y + (GetRand(6) - 3);
    }
    else {
        // 振動が終わったら元の位置基準に戻る
        enemy.x = init_x;
        enemy.y = init_y;
    }

    // ---------------------------------------------------------
    // フェーズ2：プラグ・ポップ (cycle 60)
    // ---------------------------------------------------------
    if (cycle == 60) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPlugPop;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x); // 自機狙い

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ---------------------------------------------------------
    // フェーズ3 & 4：ファイン・フォーム & ドロップ・レイン (cycle 80)
    // ---------------------------------------------------------
    if (cycle == 80) {
        // 1. ファイン・フォーム（泡ばら撒き）
        sEnemyShotSet* pSetFoam = new sEnemyShotSet;
        pSetFoam->count = 0;
        pSetFoam->patternFunc = ShotFineFoam;
        pSetFoam->x = enemy.x;
        pSetFoam->y = enemy.y;

        pSetFoam->pEnemyShotHead = new sEnemyShot;
        pSetFoam->pEnemyShotHead->prev = pSetFoam->pEnemyShotHead;
        pSetFoam->pEnemyShotHead->next = pSetFoam->pEnemyShotHead;

        pSetFoam->prev = enemyShotSetHead.prev;
        pSetFoam->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetFoam;
        enemyShotSetHead.prev = pSetFoam;

        // 2. ドロップ・レイン（大玉重力落下）
        sEnemyShotSet* pSetRain = new sEnemyShotSet;
        pSetRain->count = 0;
        pSetRain->patternFunc = ShotDropRain;
        pSetRain->x = enemy.x;
        pSetRain->y = enemy.y;

        pSetRain->pEnemyShotHead = new sEnemyShot;
        pSetRain->pEnemyShotHead->prev = pSetRain->pEnemyShotHead;
        pSetRain->pEnemyShotHead->next = pSetRain->pEnemyShotHead;

        pSetRain->prev = enemyShotSetHead.prev;
        pSetRain->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetRain;
        enemyShotSetHead.prev = pSetRain;
    }

    // 噴出中は左右にゆっくり移動して広範囲にばら撒く (cycle 80 ~ 280)
    if (cycle >= 80 && cycle <= 280) {
        enemy.x = 240.0 + 100.0 * sin((cycle - 80) * DX_PI / 100.0);
    }
}