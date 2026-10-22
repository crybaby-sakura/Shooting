#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕:深層伝播
// ------------------------------------------------------------
//  大玉(青)    = ニューロン(ノード):格子状に展開して滞留
//  小玉(白)    = シナプス信号:ニューロンの列の間を流れる
//  短レーザー(黄)    = 出力層の発火:全方向放射
//  短レーザー(シアン) = 逆伝播パルス:ニューロンから上下に放たれる
//
//  ※大玉は当たり判定が大きい(20x20)ため、そのままでは
//   「ニューロンの陰に隠れる」立ち回りが難しい場合は
//   中玉(7.0x7.0)に置き換えて判定を緩くすると調整可能。
// ============================================================

// 弾の役割タイプ(param_i[0] に格納)
#define SHOT_NEURON   0   // ニューロン
#define SHOT_SIGNAL   1   // シナプス信号
#define SHOT_FIRE     2   // 発火して放たれたレーザー

// フェーズ
#define PHASE_BUILD    0   // ネットワーク構築
#define PHASE_FORWARD  1   // 順伝播
#define PHASE_BACKWARD 2   // 逆伝播

#define PHASE_BUILD_TIME    90
#define PHASE_FORWARD_TIME  300
#define PHASE_BACKWARD_TIME 240

// ニューロン格子の設定
#define GRID_COLS     3
#define GRID_ROWS     3
#define GRID_CENTER_X 240.0
#define GRID_CENTER_Y 220.0
#define GRID_SPAN_X   120.0
#define GRID_SPAN_Y   80.0

#define SIGNAL_SPEED  3.5
#define FIRE_RADIUS   26.0   // 信号がニューロンに「到達した」とみなす距離

// 弾を1つ生成してセットのリストに繋ぐ
static void AddShot(sEnemyShotSet* pSet, int type, int kind,
    double x, double y, double muki, double speed)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = type;
    p->margin = 80;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// ------------------------------------------------------------
//  パターン本体
//  set->param_i[0] : フェーズ
//  set->param_i[1] : フェーズ内タイマー
//  set->param_i[2] : 放射の回転オフセット(発火ごとに加算)
//  set->param_i[3] : 発火音のクールダウン
// ------------------------------------------------------------
static void ShotDeepPropagation(sEnemyShotSet* pSet)
{
    int phase = pSet->param_i[0];
    int timer = pSet->param_i[1];

    // ---------- 初期化:ニューロン(大玉)を敵位置に生成 ----------
    if (pSet->count == 0) {
        for (int row = 0; row < GRID_ROWS; row++) {
            for (int col = 0; col < GRID_COLS; col++) {
                sEnemyShot* p = new sEnemyShot;
                p->x = pSet->x;
                p->y = pSet->y;
                p->muki = 0.0;
                p->speed = 0.0;
                p->kind = img_enemyShotLargeBall[4];   // 大玉:青 = ニューロン
                p->param_i[0] = SHOT_NEURON;
                p->param_i[1] = col;                   // 列(0:入力層 2:出力層)
                p->param_i[2] = row;                   // 行
                p->param_i[3] = 0;                     // 発火クールダウン
                p->param_d[0] = GRID_CENTER_X + (col - 1) * GRID_SPAN_X;  // ホーム座標
                p->param_d[1] = GRID_CENTER_Y + (row - 1) * GRID_SPAN_Y;

                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
        }
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        phase = PHASE_BUILD;
        timer = 0;
    }

    // ---------- フェーズ進行 ----------
    timer++;

    switch (phase) {
    case PHASE_BUILD:
        // ニューロンがホーム座標へ展開するのを待つ
        if (timer >= PHASE_BUILD_TIME) {
            phase = PHASE_FORWARD;
            timer = 0;
        }
        break;

    case PHASE_FORWARD:
        // 順伝播:入力層(左列)のニューロンから信号(小玉)を右へ流す
        if (timer % 24 == 1) {
            sEnemyShot* pNeuron = pSet->pEnemyShotHead->next;
            while (pNeuron != pSet->pEnemyShotHead) {
                if (pNeuron->param_i[0] == SHOT_NEURON && pNeuron->param_i[1] == 0) {
                    // ニューロンの少し左から、その行の高さで右方向へ
                    AddShot(pSet, SHOT_SIGNAL, img_enemyShotSmallBall[6],
                        pNeuron->x - 55.0, pNeuron->y,
                        0.0, SIGNAL_SPEED);
                }
                pNeuron = pNeuron->next;
            }
        }
        if (timer >= PHASE_FORWARD_TIME) {
            phase = PHASE_BACKWARD;
            timer = 0;
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        break;

    case PHASE_BACKWARD:
        // 逆伝播:出力層(右列)から信号を左へ流す
        if (timer % 24 == 1) {
            sEnemyShot* pNeuron = pSet->pEnemyShotHead->next;
            while (pNeuron != pSet->pEnemyShotHead) {
                if (pNeuron->param_i[0] == SHOT_NEURON && pNeuron->param_i[1] == GRID_COLS - 1) {
                    AddShot(pSet, SHOT_SIGNAL, img_enemyShotSmallBall[6],
                        pNeuron->x + 55.0, pNeuron->y,
                        DX_PI, SIGNAL_SPEED);
                }
                pNeuron = pNeuron->next;
            }
        }
        if (timer >= PHASE_BACKWARD_TIME) {
            // 学習(重み更新):各ニューロンのホーム座標をシフトさせる
            // GetRand(40) は 0〜40 を返すので -20 することで -20〜+20 になる
            sEnemyShot* pNeuron = pSet->pEnemyShotHead->next;
            while (pNeuron != pSet->pEnemyShotHead) {
                if (pNeuron->param_i[0] == SHOT_NEURON) {
                    pNeuron->param_d[0] += GetRand(40) - 20;
                    pNeuron->param_d[1] += GetRand(40) - 20;
                    // 格子が崩れすぎないようクランプ
                    if (pNeuron->param_d[0] < GRID_CENTER_X - GRID_SPAN_X - 20.0)
                        pNeuron->param_d[0] = GRID_CENTER_X - GRID_SPAN_X - 20.0;
                    if (pNeuron->param_d[0] > GRID_CENTER_X + GRID_SPAN_X + 20.0)
                        pNeuron->param_d[0] = GRID_CENTER_X + GRID_SPAN_X + 20.0;
                    if (pNeuron->param_d[1] < GRID_CENTER_Y - GRID_SPAN_Y - 20.0)
                        pNeuron->param_d[1] = GRID_CENTER_Y - GRID_SPAN_Y - 20.0;
                    if (pNeuron->param_d[1] > GRID_CENTER_Y + GRID_SPAN_Y + 20.0)
                        pNeuron->param_d[1] = GRID_CENTER_Y + GRID_SPAN_Y + 20.0;
                }
                pNeuron = pNeuron->next;
            }
            phase = PHASE_FORWARD;
            timer = 0;
        }
        break;
    }

    pSet->param_i[0] = phase;
    pSet->param_i[1] = timer;

    // ---------- 発火判定:信号とニューロンの近接をチェック ----------
    sEnemyShot* pNeuron = pSet->pEnemyShotHead->next;
    while (pNeuron != pSet->pEnemyShotHead) {
        if (pNeuron->param_i[0] == SHOT_NEURON && pNeuron->param_i[3] == 0) {
            bool nearQ = false;
            sEnemyShot* pSig = pSet->pEnemyShotHead->next;
            while (pSig != pSet->pEnemyShotHead) {
                if (pSig->param_i[0] == SHOT_SIGNAL) {
                    double dx = pSig->x - pNeuron->x;
                    double dy = pSig->y - pNeuron->y;
                    if (dx * dx + dy * dy < FIRE_RADIUS * FIRE_RADIUS) {
                        nearQ = true;
                        break;
                    }
                }
                pSig = pSig->next;
            }

            if (nearQ) {
                if (phase == PHASE_FORWARD && pNeuron->param_i[1] == GRID_COLS - 1) {
                    // 出力層の発火:全方向へ黄レーザーを放射
                    for (int i = 0; i < 10; i++) {
                        double m = i * (2.0 * DX_PI / 10.0) + pSet->param_i[2] * 0.35;
                        AddShot(pSet, SHOT_FIRE, img_enemyShotLaser[1],
                            pNeuron->x, pNeuron->y, m, 3.0);
                    }
                    pSet->param_i[2]++;         // 次の発火は向きを変える
                    pNeuron->param_i[3] = 50;   // クールダウン
                    if (pSet->param_i[3] == 0) {
                        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                        pSet->param_i[3] = 30;
                    }
                }
                else if (phase == PHASE_BACKWARD) {
                    // 逆伝播パルス:信号が通過したニューロンから上下へシアンレーザー
                    AddShot(pSet, SHOT_FIRE, img_enemyShotLaser[3],
                        pNeuron->x, pNeuron->y, DX_PI / 2.0 + (GetRand(30) - 15) * DX_PI / 180.0, 2.0);
                    AddShot(pSet, SHOT_FIRE, img_enemyShotLaser[3],
                        pNeuron->x, pNeuron->y, -DX_PI / 2.0 + (GetRand(30) - 15) * DX_PI / 180.0, 2.0);
                    pNeuron->param_i[3] = 45;
                }
            }
        }
        pNeuron = pNeuron->next;
    }

    if (pSet->param_i[3] > 0) pSet->param_i[3]--;

    // ---------- 弾の移動 ----------
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        switch (pShot->param_i[0]) {
        case SHOT_NEURON:
        {
            // ホーム座標へ収束しつつ、微かに脈動させる(生きているネットワーク感)
            double bobX = sin((pSet->count + pShot->param_i[2] * 40) * 0.05) * 3.0;
            double bobY = cos((pSet->count + pShot->param_i[1] * 40) * 0.04) * 3.0;
            pShot->x += (pShot->param_d[0] + bobX - pShot->x) * 0.08;
            pShot->y += (pShot->param_d[1] + bobY - pShot->y) * 0.08;
            if (pShot->param_i[3] > 0) pShot->param_i[3]--;
            break;
        }
        case SHOT_SIGNAL:
        case SHOT_FIRE:
            // 信号・レーザーは等速直線移動。
            // 画面外に出ればメインルーチンが自動で消してくれる。
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }
        pShot = pNext;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_NeuralNetwork_Zai()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;

        // 弾幕セットは1つだけ生成し、以降は patternFunc 内で
        // フェーズ管理をすべて行う
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDeepPropagation;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_i[0] = 0;   // フェーズ
        pEnemyShotSet->param_i[1] = 0;   // フェーズ内タイマー
        pEnemyShotSet->param_i[2] = 0;   // 放射の回転オフセット
        pEnemyShotSet->param_i[3] = 0;   // 発火音クールダウン

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵は画面上部でゆっくり左右に揺れるだけ(弾幕の主役はネットワーク)
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 0) muki *= -1;
    }
}