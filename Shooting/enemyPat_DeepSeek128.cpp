#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ニューラルネット弾幕 "Forward Propagation"
//   - 4層（入力5 / 隠れ4 / 隠れ3 / 出力2）のニューラルネットを画面に構成
//   - 大玉 = ニューロン、小玉 = シナプス信号、中玉 = 発火エフェクト
//   - 上から下へ順伝播し、最後に出力層が弾幕を放つ
//   - 敵本体の関数: EnemyPat_NeuralNetwork_DeepSeek()
// ============================================================

// 弾の役割（sEnemyShot::param_i[0] に格納）
enum {
    ROLE_BULLET = 0,   // 通常弾
    ROLE_NEURON_IN = 1,   // 入力層ニューロン
    ROLE_NEURON_H1 = 2,   // 隠れ層1ニューロン
    ROLE_NEURON_H2 = 3,   // 隠れ層2ニューロン
    ROLE_NEURON_OUT = 4,   // 出力層ニューロン
    ROLE_SIGNAL = 10,  // シナプス信号（小玉）
    ROLE_FIRE = 20,  // 発火/到達エフェクト（中玉）
};

// サイクル長（60fps想定）
static const int NN_CYCLE = 200;
// 信号が隣の層へ到達するまでに要するフレーム数
static const int NN_SIGNAL_FRAMES = 25;

// 各層のY座標
static const double NN_LAYER_Y[4] = { 80.0, 150.0, 220.0, 290.0 };
// 各層のニューロン数
static const int    NN_NUM[4] = { 5, 4, 3, 2 };

// 各層のニューロンX座標（未使用要素は 0）
static const double NN_LAYER_X[4][5] = {
    {  60.0, 150.0, 240.0, 330.0, 420.0 },
    {  90.0, 190.0, 290.0, 390.0,   0.0 },
    { 140.0, 240.0, 340.0,   0.0,   0.0 },
    { 190.0, 290.0,   0.0,   0.0,   0.0 },
};

// 層ごとのニューロン基本色（img_enemyShotLargeBall の添字）
//   0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
static const int NN_LAYER_COLOR[4] = { 3, 2, 1, 5 };

// 接続行列（1 = シナプス結合あり）
static const int NN_CONN1[5][4] = {
    {1,1,0,0},
    {1,0,1,0},
    {0,1,1,1},
    {0,0,1,1},
    {1,0,0,1},
};
static const int NN_CONN2[4][3] = {
    {1,1,0},
    {1,0,0},
    {0,1,1},
    {0,0,1},
};
static const int NN_CONN3[3][2] = {
    {1,1},
    {1,0},
    {0,1},
};

// ------------------------------------------------------------
// ヘルパー
// ------------------------------------------------------------
static void NN_AppendShot(sEnemyShotSet* pSet, sEnemyShot* p)
{
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// ニューロン（大玉）を1個生成
static void NN_SpawnNeuron(sEnemyShotSet* pSet, int layer, int idx)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = NN_LAYER_X[layer][idx];
    p->y = NN_LAYER_Y[layer];
    p->muki = 0.0;
    p->speed = 0.0;
    p->kind = img_enemyShotLargeBall[NN_LAYER_COLOR[layer]];
    p->count = 0;
    p->param_i[0] = ROLE_NEURON_IN + layer;   // 役割
    p->param_i[1] = idx;                      // 層内インデックス
    p->param_i[2] = 0;                        // 発火タイマー
    p->param_i[3] = NN_LAYER_COLOR[layer];    // 通常時の色
    p->param_d[0] = NN_LAYER_X[layer][idx];   // 基準X
    p->param_d[1] = NN_LAYER_Y[layer];        // 基準Y
    NN_AppendShot(pSet, p);
}

// シナプス信号（小玉）を source→target へ飛ばす
static void NN_SpawnSignal(sEnemyShotSet* pSet,
    double sx, double sy,
    double tx, double ty,
    int color)
{
    sEnemyShot* p = new sEnemyShot;
    double dx = tx - sx;
    double dy = ty - sy;
    double dist = sqrt(dx * dx + dy * dy);
    if (dist < 1.0) dist = 1.0;

    p->x = sx;
    p->y = sy;
    p->muki = atan2(dy, dx);
    p->speed = dist / (double)NN_SIGNAL_FRAMES;
    p->kind = img_enemyShotSmallBall[color];
    p->count = 0;
    p->param_i[0] = ROLE_SIGNAL;
    p->param_i[1] = NN_SIGNAL_FRAMES;   // 到着までのフレーム数
    p->param_d[0] = tx;                 // 目標X
    p->param_d[1] = ty;                 // 目標Y
    NN_AppendShot(pSet, p);
}

// 発火/到達エフェクト（中玉）をその場に発生
static void NN_SpawnFire(sEnemyShotSet* pSet, double x, double y, int color)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = 0.0;
    p->speed = 0.0;
    p->kind = img_enemyShotMediumBall[color];
    p->count = 0;
    p->param_i[0] = ROLE_FIRE;
    p->param_i[1] = 12;   // 寿命（フレーム）
    NN_AppendShot(pSet, p);
}

// 通常弾を1発生成
static void NN_SpawnBullet(sEnemyShotSet* pSet,
    double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->count = 0;
    p->param_i[0] = ROLE_BULLET;
    NN_AppendShot(pSet, p);
}

// ------------------------------------------------------------
// 出力層の発火：弾幕本体
// ------------------------------------------------------------
static void NN_FireAttack(sEnemyShotSet* pSet, int cycle)
{
    if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

    if (cycle % 2 == 0) {
        // 偶数サイクル：扇形弾
        for (int i = 0; i < NN_NUM[3]; i++) {
            double cx = NN_LAYER_X[3][i];
            double cy = NN_LAYER_Y[3];
            int color = (i == 0) ? 4 : 0;   // 青 / 赤
            for (int k = -4*6; k <= 4*6; k++) {
                double a = DX_PI / 2.0 + k * 0.10;
                NN_SpawnBullet(pSet, cx, cy, a, 1.8,
                    img_enemyShotMediumBall[color]);
            }
            // 中央に速い菱形弾
            NN_SpawnBullet(pSet, cx, cy, DX_PI / 2.0, 3.5,
                img_enemyShotDiamond[color]);
        }
    }
    else {
        // 奇数サイクル：全方位弾
        const int n = 14*3;
        for (int i = 0; i < NN_NUM[3]; i++) {
            double cx = NN_LAYER_X[3][i];
            double cy = NN_LAYER_Y[3];
            int color = (i == 0) ? 0 : 5;   // 赤 / マゼンタ
            for (int k = 0; k < n; k++) {
                double a = (k + i * 0.5) * DX_PI * 2.0 / (double)n;
                NN_SpawnBullet(pSet, cx, cy, a, 1.5,
                    img_enemyShotMediumBall[color]);
            }
            // 中央に遅い大玉
            NN_SpawnBullet(pSet, cx, cy, DX_PI / 2.0, 0.8,
                img_enemyShotLargeBall[color]);
        }
    }
}

// ------------------------------------------------------------
// 層の発火：ニューロンを橙に点滅させ、次層へ信号を流す
// ------------------------------------------------------------
static void NN_FireLayer(sEnemyShotSet* pSet, int layer, int cycle)
{
    // この層のニューロンに発火タイマーをセット
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == ROLE_NEURON_IN + layer) {
            p->param_i[2] = 15;   // 15フレーム橙になる
        }
        p = p->next;
    }

    if (layer == 0) {
        for (int i = 0; i < NN_NUM[0]; i++)
            for (int j = 0; j < NN_NUM[1]; j++)
                if (NN_CONN1[i][j])
                    NN_SpawnSignal(pSet,
                        NN_LAYER_X[0][i], NN_LAYER_Y[0],
                        NN_LAYER_X[1][j], NN_LAYER_Y[1], 6);   // 白
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    else if (layer == 1) {
        for (int i = 0; i < NN_NUM[1]; i++)
            for (int j = 0; j < NN_NUM[2]; j++)
                if (NN_CONN2[i][j])
                    NN_SpawnSignal(pSet,
                        NN_LAYER_X[1][i], NN_LAYER_Y[1],
                        NN_LAYER_X[2][j], NN_LAYER_Y[2], 6);
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    else if (layer == 2) {
        for (int i = 0; i < NN_NUM[2]; i++)
            for (int j = 0; j < NN_NUM[3]; j++)
                if (NN_CONN3[i][j])
                    NN_SpawnSignal(pSet,
                        NN_LAYER_X[2][i], NN_LAYER_Y[2],
                        NN_LAYER_X[3][j], NN_LAYER_Y[3], 6);
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    else {
        // 出力層：弾幕
        NN_FireAttack(pSet, cycle);
    }
}

// ------------------------------------------------------------
// 全弾の更新
//   ・ニューロン：基準位置の周りで微小振動、発火中は橙
//   ・信号：目標へ移動、到達したら消滅（画面外送りでメインに削除してもらう）
//   ・発火エフェクト：寿命まで留まる
//   ・通常弾：等速直進
// ------------------------------------------------------------
static void NN_Update(sEnemyShotSet* pSet)
{
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next;
        int role = p->param_i[0];

        if (role >= ROLE_NEURON_IN && role <= ROLE_NEURON_OUT) {
            // ニューロン：微振動
            double bx = p->param_d[0];
            double by = p->param_d[1];
            p->x = bx + sin(p->count * 0.13) * 1.5;
            p->y = by + cos(p->count * 0.17) * 1.5;

            if (p->param_i[2] > 0) {
                p->param_i[2]--;
                p->kind = img_enemyShotLargeBall[8];              // 発火中：橙
            }
            else {
                p->kind = img_enemyShotLargeBall[p->param_i[3]];  // 通常色
            }
        }
        else if (role == ROLE_SIGNAL) {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);

            if (p->count >= p->param_i[1]) {
                // 目標ニューロンに到達 → 到達エフェクト（白の中玉）を残して消滅
                NN_SpawnFire(pSet, p->param_d[0], p->param_d[1], 6);
                p->x = 10000.0;
                p->y = 10000.0;
            }
        }
        else if (role == ROLE_FIRE) {
            if (p->count >= p->param_i[1]) {
                p->x = 10000.0;
                p->y = 10000.0;
            }
        }
        else {
            // 通常弾
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }

        p = pNext;
    }
}

// ------------------------------------------------------------
// ニューラルネット弾幕本体（ショットセットのpatternFunc）
// ------------------------------------------------------------
static void ShotNeuralNet(sEnemyShotSet* pSet)
{
    int t = pSet->count % NN_CYCLE;
    int cycle = pSet->count / NN_CYCLE;

    // 初回：全層のニューロンを配置
    if (pSet->count == 0) {
        for (int layer = 0; layer < 4; layer++)
            for (int i = 0; i < NN_NUM[layer]; i++)
                NN_SpawnNeuron(pSet, layer, i);
    }

    // サイクル開始：予告音＋敵から入力層への刺激（白い小玉）
    if (t == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        NN_SpawnBullet(pSet, enemy.x, enemy.y + 20.0, DX_PI / 2.0, 3.0,
            img_enemyShotSmallBall[6]);
    }

    // 順伝播スケジュール（信号が届いた直後に次の層が発火する）
    if (t == 30) NN_FireLayer(pSet, 0, cycle);   // 入力層発火
    if (t == 60) NN_FireLayer(pSet, 1, cycle);   // 隠れ層1発火
    if (t == 90) NN_FireLayer(pSet, 2, cycle);   // 隠れ層2発火
    if (t == 120) NN_FireLayer(pSet, 3, cycle);   // 出力層発火（弾幕）

    // 全弾更新
    NN_Update(pSet);
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_NeuralNetwork_DeepSeek()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.8 * (double)muki;
        if (enemy.x > 420.0) muki = -1;
        if (enemy.x < 60.0) muki = 1;
    }

    // ニューラルネットのショットセットを1回だけ生成
    if (count == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotNeuralNet;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}