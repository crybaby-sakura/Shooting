// enemyPat_nn.cpp
#include <cmath>
#include "gv.h"

// DxLibの音声を安全に再生するヘルパー
static void PlayShotSound(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) {
        StopSoundMem(soundHandle);
    }
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

// 弾生成の共通ヘルパー関数
static void AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind, int typeFlag = 0)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    // typeFlag = 0: 通常移動弾, 1: ニューロン弾(静止), 2: シナプス弾(静止)
    pShot->param_i[0] = typeFlag;

    // 双方向循環リストの末尾に追加
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 2つのノード間にシナプス（エッジ）を生成するヘルパー関数
static void AddSynapse(sEnemyShotSet* pSet, double x1, double y1, double x2, double y2, int kind)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    double dist = std::sqrt(dx * dx + dy * dy);
    double angle = std::atan2(dy, dx);

    double step = 16.0; // シナプスを構成する弾の配置間隔
    int numSteps = (int)(dist / step);

    // ノード本体（中玉）に重ならないよう、1から始めて numSteps-1 まで配置
    for (int i = 1; i < numSteps; ++i) {
        double sx = x1 + i * step * std::cos(angle);
        double sy = y1 + i * step * std::sin(angle);

        // 速度0、typeFlag=2(シナプス弾)として配置
        AddShot(pSet, sx, sy, angle, 0.0, kind, 2);
    }
}

// 弾幕処理関数：順伝播マルチレイヤー（シナプス結合あり）
static void ShotForwardPropagation(sEnemyShotSet* pEnemyShotSet)
{
    // 各層のノード座標定義 (画面サイズ 480x480)
    static const int INPUT_COUNT = 3;
    static const double inputPos[3][2] = {
        { 140.0, 100.0 }, { 240.0, 100.0 }, { 340.0, 100.0 }
    };

    static const int HIDDEN_COUNT = 4;
    static const double hiddenPos[4][2] = {
        { 90.0, 210.0 }, { 190.0, 225.0 }, { 290.0, 225.0 }, { 390.0, 210.0 }
    };

    static const int OUTPUT_COUNT = 2;
    static const double outputPos[2][2] = {
        { 180.0, 340.0 }, { 300.0, 340.0 }
    };

    const int t = pEnemyShotSet->count;

    // -------------------------------------------------------------
    // Phase 1: ニューロン（ノード）の設置
    // -------------------------------------------------------------
    if (t == 0) {
        PlayShotSound(sound_enemyCharge);

        // 入力層（青色の中玉）
        for (int i = 0; i < INPUT_COUNT; ++i)
            AddShot(pEnemyShotSet, inputPos[i][0], inputPos[i][1], 0.0, 0.0, img_enemyShotMediumBall[4], 1);

        // 隠れ層（黄色の中玉）
        for (int i = 0; i < HIDDEN_COUNT; ++i)
            AddShot(pEnemyShotSet, hiddenPos[i][0], hiddenPos[i][1], 0.0, 0.0, img_enemyShotMediumBall[1], 1);

        // 出力層（赤色の中玉）
        for (int i = 0; i < OUTPUT_COUNT; ++i)
            AddShot(pEnemyShotSet, outputPos[i][0], outputPos[i][1], 0.0, 0.0, img_enemyShotMediumBall[0], 1);
    }

    // -------------------------------------------------------------
    // Phase 2: シナプス（結合エッジ）の描画展開
    // -------------------------------------------------------------
    if (t == 30) {
        PlayShotSound(sound_enemyShot_light);

        // 入力層 -> 隠れ層 への青色シナプス（銃弾）
        for (int i = 0; i < INPUT_COUNT; ++i) {
            for (int j = 0; j < HIDDEN_COUNT; ++j) {
                AddSynapse(pEnemyShotSet, inputPos[i][0], inputPos[i][1], hiddenPos[j][0], hiddenPos[j][1], img_enemyShotBullet[4]);
            }
        }

        // 隠れ層 -> 出力層 への黄色シナプス（銃弾）
        for (int j = 0; j < HIDDEN_COUNT; ++j) {
            for (int k = 0; k < OUTPUT_COUNT; ++k) {
                AddSynapse(pEnemyShotSet, hiddenPos[j][0], hiddenPos[j][1], outputPos[k][0], outputPos[k][1], img_enemyShotBullet[1]);
            }
        }
    }

    // -------------------------------------------------------------
    // Phase 3: 順伝播 第1層（シナプス上を信号が走る）
    // -------------------------------------------------------------
    if (t == 70) {
        PlayShotSound(sound_enemyShot_medium);

        for (int i = 0; i < INPUT_COUNT; ++i) {
            for (int j = 0; j < HIDDEN_COUNT; ++j) {
                double angle = std::atan2(hiddenPos[j][1] - inputPos[i][1], hiddenPos[j][0] - inputPos[i][0]);
                // 白色の菱形弾（シグナル）がシナプスの上を這うように高速移動
                AddShot(pEnemyShotSet, inputPos[i][0], inputPos[i][1], angle, 6.0, img_enemyShotDiamond[6], 0);
            }
        }
    }

    // -------------------------------------------------------------
    // Phase 4: 順伝播 第2層（隠れ層から出力層へ伝播）
    // -------------------------------------------------------------
    if (t == 100) {
        PlayShotSound(sound_enemyShot_medium);

        for (int j = 0; j < HIDDEN_COUNT; ++j) {
            for (int k = 0; k < OUTPUT_COUNT; ++k) {
                double angle = std::atan2(outputPos[k][1] - hiddenPos[j][1], outputPos[k][0] - hiddenPos[j][0]);
                // 白色の菱形弾（シグナル）がシナプスの上を移動
                AddShot(pEnemyShotSet, hiddenPos[j][0], hiddenPos[j][1], angle, 6.0, img_enemyShotDiamond[6], 0);
            }
        }
    }

    // -------------------------------------------------------------
    // Phase 5: 出力層の発火（自機狙いの攻撃）
    // -------------------------------------------------------------
    if (t == 135) {
        PlayShotSound(sound_enemyShot_heavy);

        for (int k = 0; k < OUTPUT_COUNT; ++k) {
            double targetAngle = std::atan2(player.y - outputPos[k][1], player.x - outputPos[k][0]);

            // 出力ノードから自機へ向けて5Wayの赤小弾を一斉射撃
            for (int way = -2; way <= 2; ++way) {
                double spreadAngle = targetAngle + (way * 0.12);
                // GetRand(50) は 0〜50の51種類の整数
                double speed = 2.8 + (GetRand(50) / 100.0);
                AddShot(pEnemyShotSet, outputPos[k][0], outputPos[k][1], spreadAngle, speed, img_enemyShotSmallBall[0], 0);
            }
        }
    }

    // -------------------------------------------------------------
    // Phase 6: ネットワーク崩壊（ニューロンとシナプスが弾け飛ぶ）
    // -------------------------------------------------------------
    if (t == 190) {
        PlayShotSound(sound_enemyShot_extreme);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // ニューロン弾 または シナプス弾 ならば、制限を解除してランダムに散らす
            if (pShot->param_i[0] == 1 || pShot->param_i[0] == 2) {
                pShot->muki = (GetRand(360) / 180.0) * DX_PI;
                pShot->speed = 3.0 + (GetRand(300) / 100.0)-1;
                pShot->param_i[0] = 0; // 通常弾に戻し、以後は移動させる
            }
            pShot = pShot->next;
        }
    }

    // -------------------------------------------------------------
    // 毎フレームの全弾移動更新
    // -------------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) { // 通常の移動弾(0)のみ動かす
            pShot->x += pShot->speed * std::cos(pShot->muki);
            pShot->y += pShot->speed * std::sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン関数
void EnemyPat_NeuralNetwork_Gemini()
{
    static int moveDir;

    // 初期化 (1フレーム目)
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        moveDir = 1;
    }
    else {
        // 敵本体のゆらぎ移動
        enemy.x += 0.5 * (double)moveDir;
        if (count % 160 == 80) {
            moveDir *= -1;
        }
    }

    // 240フレームごとに1サイクルとして「ニューラルネットワーク弾幕」を展開
    if (count % 240 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotForwardPropagation;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // センチネルノード（双方向循環リストの頭）の作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // enemyShotSetHead の末尾に追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}