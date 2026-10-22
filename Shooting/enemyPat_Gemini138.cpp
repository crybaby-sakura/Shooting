#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <vector>

// ヘルパー関数：弾を追加するための簡略化関数
static void AddCustomEnemyShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind, int type, int param1 = 0, int param2 = 0) {
    sEnemyShot* pShot = new sEnemyShot;
    pShot->param_i[0] = type;   // 0:おもり, 1:糸, 2:攻撃弾
    pShot->param_i[1] = param1; // おもりのインデックス(0～4)など
    pShot->param_i[2] = param2; // 糸の長さ係数など
    pShot->param_i[3] = 0;      // 点滅エフェクト用タイマー

    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;

    // 双方向リストの末尾に追加
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 弾幕：振り子連鎖「ニュートン・クレイドル」
static void ShotNewtonCradle(sEnemyShotSet* pEnemyShotSet)
{
    // 初期化：5つの振り子を構成するおもりと糸を配置
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 5; i++) {
            // おもり (type 0、色は白)
            AddCustomEnemyShot(pEnemyShotSet, 240.0, 180.0, 0, 0, img_enemyShotLargeBall[6], 0, i);

            // 糸 (type 1、極小丸玉を9個連ねて表現)
            for (int j = 1; j < 10; j++) {
                AddCustomEnemyShot(pEnemyShotSet, 240.0, 80.0 + j * 10.0, 0, 0, img_enemyShotSmallBall[6], 1, i, j);
            }
        }
    }

    int t = pEnemyShotSet->count;
    int phase = t / 400;      // 480フレーム（約8秒）ごとにフェーズ移行
    int local_t = t % 400;

    double theta[5] = { 0, 0, 0, 0, 0 };
    bool is_impact = false;
    int impact_type = 0;      // 1:右衝突, 2:左衝突, 3:両端衝突, 4:右2個衝突, 5:左2個衝突

    // --- 振り子の角度計算と衝突判定 ---
    if (phase == 0) {
        // Phase 1: 左右交互に1個ずつスイング
        int cycle = local_t % 120;
        if (cycle < 60) {
            theta[4] = (DX_PI / 3.0) * sin(DX_PI * cycle / 60.0); // 右端がスイング
            if (cycle == 0 && t > 0) { is_impact = true; impact_type = 2; }
        }
        else {
            theta[0] = -(DX_PI / 3.0) * sin(DX_PI * (cycle - 60) / 60.0); // 左端がスイング
            if (cycle == 60) { is_impact = true; impact_type = 1; }
        }
    }
    else if (phase == 1) {
        // Phase 2: 両端から同時にスイング
        int cycle = local_t % 60;
        theta[0] = -(DX_PI / 3.0) * sin(DX_PI * cycle / 60.0);
        theta[4] = (DX_PI / 3.0) * sin(DX_PI * cycle / 60.0);
        if (cycle == 0 && t > 0) { is_impact = true; impact_type = 3; }
    }
    else {
        // Phase 3: 左右交互に2個ずつスイング
        int cycle = local_t % 120;
        if (cycle < 60) {
            theta[3] = (DX_PI / 3.0) * sin(DX_PI * cycle / 60.0);
            theta[4] = (DX_PI / 3.0) * sin(DX_PI * cycle / 60.0);
            if (cycle == 0 && t > 0) { is_impact = true; impact_type = 5; }
        }
        else {
            theta[0] = -(DX_PI / 3.0) * sin(DX_PI * (cycle - 60) / 60.0);
            theta[1] = -(DX_PI / 3.0) * sin(DX_PI * (cycle - 60) / 60.0);
            if (cycle == 60) { is_impact = true; impact_type = 4; }
        }
    }

    // --- 衝突時の攻撃弾生成とエフェクト発動 ---
    if (is_impact) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 衝撃波（中央にシアンの中玉リング弾）
        for (int i = 0; i < 12*3; i++) {
            AddCustomEnemyShot(pEnemyShotSet, 240.0, 180.0, DX_PI * 2.0 * i / 12.0/3, 3.0, img_enemyShotMediumBall[3], 2);
        }

        // 弾き飛ぶおもりから発射される自機狙い扇状弾（マゼンタの鱗弾）
        double fireX[5] = { 200.0, 220.0, 240.0, 260.0, 280.0 };
        std::vector<int> fireIndices;
        if (impact_type == 1) fireIndices.push_back(0); // 右衝突 -> 左端が飛ぶ
        else if (impact_type == 2) fireIndices.push_back(4);
        else if (impact_type == 3) { fireIndices.push_back(0); fireIndices.push_back(4); }
        else if (impact_type == 4) { fireIndices.push_back(0); fireIndices.push_back(1); }
        else if (impact_type == 5) { fireIndices.push_back(3); fireIndices.push_back(4); }

        for (size_t k = 0; k < fireIndices.size(); k++) {
            int idx = fireIndices[k];
            double aim = atan2(player.y - 180.0, player.x - fireX[idx]);
            for (int i = -2-5; i <= 2+5; i++) {
                double muki = aim + i * (DX_PI / 12.0);
                AddCustomEnemyShot(pEnemyShotSet, fireX[idx], 180.0, muki, 4.0, img_enemyShotScale[5], 2);
            }
        }
    }

    // --- 各弾の座標とエフェクトの更新 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int type = pShot->param_i[0];

        if (type == 0 || type == 1) { // おもり または 糸
            int idx = pShot->param_i[1];
            double origin_x = 240.0 + (idx - 2) * 20.0; // 支点のX座標
            double origin_y = 80.0;                     // 支点のY座標
            double len = (type == 0) ? 100.0 : pShot->param_i[2] * 10.0;

            pShot->x = origin_x + len * sin(theta[idx]);
            pShot->y = origin_y + len * cos(theta[idx]);

            // おもりのエネルギー伝達時の発光エフェクト
            if (type == 0) {
                if (is_impact) {
                    bool flash = false;
                    if ((impact_type == 1 || impact_type == 2) && idx >= 1 && idx <= 3) flash = true;
                    if (impact_type == 3 && idx >= 1 && idx <= 3) flash = true;
                    if ((impact_type == 4 || impact_type == 5) && idx == 2) flash = true;
                    if (flash) pShot->param_i[3] = 5; // 点滅タイマーを5フレームにセット
                }
                if (pShot->param_i[3] > 0) {
                    pShot->kind = img_enemyShotLargeBall[3]; // エネルギー通過中はシアン
                    pShot->param_i[3]--;
                }
                else {
                    pShot->kind = img_enemyShotLargeBall[6]; // 通常時は白
                }
            }
        }
        else if (type == 2) { // 攻撃弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_NewtonsCradle_Gemini()
{
    if (count == 1) {
        // 振り子の支点を固定するため、敵本体は画面上部に留める
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 開始から少し待って振り子をセット
    if (count == 30) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNewtonCradle;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}