// enemyPat_Tmp.cpp
// バーバーズ・ポール弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン：バーバーズ・ポール
static void ShotBarbersPole(sEnemyShotSet* pSet)
{
    // --- 定数 ------------------------------------------------
    const double VERT_SPEED_PER_FRAME = 80.0 / 60.0;          // 縦方向速度 (80px/秒, 60fps)
    const double WAVE_NUMBER = 2.0 * DX_PI / 120.0;   // 波数（1周期120px）
    const int    SPAWN_INTERVAL = 8;                     // 弾を生成する間隔 [frame]

    // --- 初期化 (最初のフレームのみ) -------------------------
    if (pSet->count == 0) {
        // param_d[0] : 回転位相
        // param_d[1] : 現在の振幅
        // param_d[2] : 基礎振幅（拡大演出に使用）
        // param_d[3] : 回転速度 [rad/frame]
        // param_d[4] : 経過時間カウンタ
        // param_d[5] : 振幅変動の周波数
        // param_d[6] : 色反転タイマー
        // param_i[0] : 色反転フラグ (0:通常, 1:反転)
        pSet->param_d[0] = 0.0;
        pSet->param_d[1] = 20.0;
        pSet->param_d[2] = 20.0;
        pSet->param_d[3] = 0.03;                          // 約1.8度/フレーム
        pSet->param_d[4] = 0.0;
        pSet->param_d[5] = 2.0 * DX_PI / 180.0;           // 3秒周期の振幅変動
        pSet->param_d[6] = 0.0;
        pSet->param_i[0] = 0;

        // 予告音（充電音）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --- 状態の更新 ------------------------------------------
    // 回転位相
    pSet->param_d[0] += pSet->param_d[3];

    // 経過時間 (弾幕持続時間の管理)
    double time = pSet->param_d[4];
    pSet->param_d[4] += 1.0;

    // 振幅の制御：最初は細く、300フレームかけて太く、その後は変調
    double baseAmp;
    if (time < 300.0) {
        // 拡大期 (0～5秒)
        baseAmp = 20.0 + (70.0 - 20.0) * time / 300.0;
    }
    else {
        // 変調期 (5秒以降) 振幅が正弦波的に変動
        baseAmp = 70.0 + 20.0 * sin((time - 300.0) * pSet->param_d[5]);
    }
    pSet->param_d[2] = baseAmp;
    double amplitude = baseAmp;

    // 色反転：5秒ごとに赤⇔青の配置を入れ替え
    pSet->param_d[6] += 1.0;
    if (pSet->param_d[6] >= 300.0) {
        pSet->param_d[6] = 0.0;
        pSet->param_i[0] = 1 - pSet->param_i[0];  // トグル
    }

    // --- 弾の生成 --------------------------------------------
    if (pSet->count % SPAWN_INTERVAL == 0) {
        for (int lane = 0; lane < 3; ++lane) {
            for (int dir = -1; dir <= 1; dir += 2) {   // dir = -1(下) / +1(上)
                sEnemyShot* pShot = new sEnemyShot;

                // レーンと反転状態から色を決定
                int colorIdx;
                if (pSet->param_i[0] == 0) { // 通常：赤・白・青
                    switch (lane) {
                    case 0: colorIdx = 0; break; // 赤
                    case 1: colorIdx = 6; break; // 白
                    case 2: colorIdx = 4; break; // 青
                    }
                }
                else {                      // 反転：青・白・赤
                    switch (lane) {
                    case 0: colorIdx = 4; break; // 青
                    case 1: colorIdx = 6; break; // 白
                    case 2: colorIdx = 0; break; // 赤
                    }
                }

                // 画像とサイズ（白のみ中玉、それ以外は小玉）
                int kind = (lane == 1)
                    ? img_enemyShotMediumBall[colorIdx]
                    : img_enemyShotSmallBall[colorIdx];

                pShot->kind = kind;
                pShot->x = enemy.x;
                pShot->y = enemy.y + 100;
                pShot->muki = 0.0;
                pShot->speed = 0.0;

                // 弾固有パラメータ（上昇/下降とレーン番号を記憶）
                pShot->param_d[0] = (double)dir;
                pShot->param_d[1] = (double)lane;

                // リンクリストに追加
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // --- 全弾の位置更新（正弦波パスに沿って移動） ------------
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double dir = pShot->param_d[0];       // ±1
        int    lane = (int)pShot->param_d[1];  // 0,1,2
        double lanePhase = lane * 2.0 * DX_PI / 3.0;  // 120度ずつの位相オフセット
        double phase = pSet->param_d[0];         // 共通回転位相

        // 縦移動
        pShot->y += dir * VERT_SPEED_PER_FRAME;

        // 横位置は正弦波で決定
        double relY = pShot->y - enemy.y - 100;
        double wave = amplitude * sin(WAVE_NUMBER * relY + phase + lanePhase);
        pShot->x = enemy.x + wave;

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_SignPole_DeepSeek()
{
    static bool initialized = false;

    if (count == 1) {
        // 初期配置：画面中央上部に固定
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        initialized = false;
    }

    if (!initialized) {
        // 弾幕セットを１つだけ作成し、持続させる
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = ShotBarbersPole;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // ヘッド（ダミーノード）の作成
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 全体のリストに接続
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        initialized = true;
    }

    // 敵本体は移動しない（固定砲台）
}