#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：ニューラルネットワーク
//
// 画面上に「入力層→隠れ層1→隠れ層2→出力層」の4層を固定配置し、
// ニューロン専用素材は無いため既存の敵弾を流用して表現する。
//   ・ニューロン本体      … 大玉弾（層ごとに色分け、常時明滅）
//   ・シナプス（配線）    … 短レーザー弾を複数個並べて線状に配置（静止）
//   ・信号の伝播          … 小玉弾が配線に沿ってニューロン間を移動
//   ・ニューロンの発火    … 菱形弾の放射状バースト
//   ・出力層への到達      … 中楕円弾によるリングバースト
//
// 入力層の全ニューロンが一定周期で一斉発火し、信号が小玉弾として
// 隠れ層へ伝播→発火→さらに次の層へ…という順伝播をそのまま弾幕として表現する。
// あるノードへ複数方向から信号が届いても、そのノードからの「次層への発射」は
// 1サイクルにつき1回だけに制限し、無制限に枝分かれして弾数が爆発しないようにしている
// （受信を示す発火バーストそのものは、届くたびに毎回出す）。
// ============================================================

// ------------------------------------------------------------
// 汎用ヘルパー：新しい sEnemyShotSet / sEnemyShot をリストに追加する
// ------------------------------------------------------------
static sEnemyShotSet* NewEnemyShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind; // このパターンでは「色番号」の受け渡しに流用する

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

static sEnemyShot* NewEnemyShotInSet(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ------------------------------------------------------------
// ネットワーク構造の定義（画面 480x480 を想定した固定レイアウト）
// ------------------------------------------------------------
static const int npLayerCount = 4;
static const int npLayerSize[npLayerCount] = { 4, 4, 3, 2 }; // 入力4・隠れ4・隠れ3・出力2

static const double npNodeX[npLayerCount][4] = {
    { 100.0, 180.0, 260.0, 340.0 },
    { 110.0, 190.0, 270.0, 350.0 },
    { 140.0, 220.0, 300.0,   0.0 },
    { 170.0, 270.0,   0.0,   0.0 },
};
static const double npNodeY[npLayerCount] = { 90.0-50, 190.0-50, 290.0-50, 390.0-50 };

// 層ごとの色（0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙）
static const int npLayerColor[npLayerCount] = { 3, 6, 6, 5 }; // 入力=シアン、隠れ=白、出力=マゼンタ

static double NpNodeX(int layer, int index) { return npNodeX[layer][index]; }
static double NpNodeY(int layer) { return npNodeY[layer]; }

// 各ノードが最後に「次層への発射」をトリガーしたフレーム（多重発火の抑制用）
static int npLastFireFrame[npLayerCount][4];

// ------------------------------------------------------------
// ニューロン本体：常駐する大玉弾。層ごとに位相をずらしてゆっくり明滅させる
// ------------------------------------------------------------
static void ShotNodeBlink(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int layer = 0; layer < npLayerCount; layer++) {
            for (int i = 0; i < npLayerSize[layer]; i++) {
                sEnemyShot* pShot = NewEnemyShotInSet(pSet);
                pShot->x = NpNodeX(layer, i);
                pShot->y = NpNodeY(layer);
                pShot->muki = -DX_PI / 2.0;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotLargeBall[npLayerColor[layer]];
                pShot->param_i[0] = layer;
                pShot->param_i[1] = i;
            }
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 層・ノードごとに位相をずらして呼吸するように明滅させる（本来色⇔白）
        int phase = (count + pShot->param_i[0] * 15 + pShot->param_i[1] * 7) % 60;
        int color = (phase < 30) ? npLayerColor[pShot->param_i[0]] : 6;
        pShot->kind = img_enemyShotLargeBall[color];
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// シナプス（配線）：短レーザー弾を線状に並べて静止させる
// ------------------------------------------------------------
static void ShotSynapseLine(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int layer = 0; layer + 1 < npLayerCount; layer++) {
            for (int i = 0; i < npLayerSize[layer]; i++) {
                double x1 = NpNodeX(layer, i);
                double y1 = NpNodeY(layer);
                for (int j = 0; j < npLayerSize[layer + 1]; j++) {
                    double x2 = NpNodeX(layer + 1, j);
                    double y2 = NpNodeY(layer + 1);
                    double dx = x2 - x1;
                    double dy = y2 - y1;
                    double dist = sqrt(dx * dx + dy * dy);
                    double angle = atan2(dy, dx);

                    int segCount = (int)(dist / 46.0);
                    if (segCount < 1) segCount = 1;

                    for (int s = 0; s < segCount; s++) {
                        double t = (s + 1.0) / (segCount + 1.0);
                        sEnemyShot* pShot = NewEnemyShotInSet(pSet);
                        pShot->x = x1 + dx * t;
                        pShot->y = y1 + dy * t;
                        pShot->muki = angle; // 配線の向きに画像を合わせる
                        pShot->speed = 0.0;
                        pShot->kind = img_enemyShotLaser[0]; // 赤：シナプス配線
                    }
                }
            }
        }
    }
    // 静止したまま画面に留まるので、毎フレームの移動処理は不要
}

// ------------------------------------------------------------
// 中間層の発火バースト（菱形弾の放射）
// ------------------------------------------------------------
static void ShotFireBurstMove(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int burstCount = 16/2;
        for (int i = 0; i < burstCount; i++) {
            sEnemyShot* pShot = NewEnemyShotInSet(pSet);
            double angle = DX_PI * 2.0 * i / burstCount + count * 0.005;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = 2.5-1;
            pShot->kind = img_enemyShotDiamond[pSet->kind];
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

static void SpawnFireBurst(double x, double y, int color)
{
    NewEnemyShotSet(ShotFireBurstMove, x, y, 0.0, color);
}

// ------------------------------------------------------------
// 出力層への到達リング（中楕円弾の全方位バースト）
// ------------------------------------------------------------
static void ShotOutputRingMove(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int ringCount = 32/2;
        for (int i = 0; i < ringCount; i++) {
            sEnemyShot* pShot = NewEnemyShotInSet(pSet);
            double angle = DX_PI * 2.0 * i / ringCount + count * 0.005;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = 3.0-1;
            pShot->kind = img_enemyShotMediumOval[pSet->kind];
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

static void SpawnOutputRing(double x, double y, int color)
{
    NewEnemyShotSet(ShotOutputRingMove, x, y, 0.0, color);
}

// ------------------------------------------------------------
// 信号の伝播（小玉弾）。ShotSignalPulse と SpawnPulseWave は相互参照するため前方宣言する
// ------------------------------------------------------------
static void ShotSignalPulse(sEnemyShotSet* pSet);

static void SpawnPulseWave(double fromX, double fromY, int fromLayer, int toLayer)
{
    int color = npLayerColor[fromLayer];
    sEnemyShotSet* pSet = NewEnemyShotSet(ShotSignalPulse, fromX, fromY, 0.0, color);

    // 全結合を表現するため、発火元から次層の全ノードへ1本ずつ信号を送る
    for (int j = 0; j < npLayerSize[toLayer]; j++) {
        double tx = NpNodeX(toLayer, j);
        double ty = NpNodeY(toLayer);

        sEnemyShot* pShot = NewEnemyShotInSet(pSet);
        pShot->x = fromX;
        pShot->y = fromY;
        pShot->muki = atan2(ty - fromY, tx - fromX);
        pShot->speed = 3.0;
        pShot->kind = img_enemyShotMediumOval[color];
        pShot->param_i[0] = toLayer; // 到達先の層
        pShot->param_i[1] = j;       // 到達先のノード番号
        pShot->param_d[0] = tx;
        pShot->param_d[1] = ty;
    }
}

static void ShotSignalPulse(sEnemyShotSet* pSet)
{
    // このセットの弾は SpawnPulseWave 側で直接生成するため、ここでの初期生成は行わない

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        double targetX = pShot->param_d[0];
        double targetY = pShot->param_d[1];
        double dx = targetX - pShot->x;
        double dy = targetY - pShot->y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist <= pShot->speed) {
            // 目的地のニューロンに到達：受信エフェクトを出し、必要なら次層へ伝播させる
            int targetLayer = pShot->param_i[0];
            int targetIndex = pShot->param_i[1];

            if (targetLayer + 1 < npLayerCount) {
                SpawnFireBurst(targetX, targetY, 1); // 黄色い菱形弾で発火演出

                // 同一サイクル内でこのノードがまだ次層へ発射していなければ伝播させる
                // （複数方向から同じノードへ信号が届いても、発射は1回に集約する）
                if (count - npLastFireFrame[targetLayer][targetIndex] > 20) {
                    npLastFireFrame[targetLayer][targetIndex] = count;
                    SpawnPulseWave(targetX, targetY, targetLayer, targetLayer + 1);
                }
            }
            else {
                SpawnOutputRing(targetX, targetY, 8); // 出力層到達：橙のリング
            }

            // 画面外へ逃がし、メインルーチンの画面外削除に処理を委ねる
            pShot->x = -9999.0;
            pShot->y = -9999.0;
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_NeuralNetwork_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        // ボス本体はネットワーク図とは独立した固定座標に描かれるだけの存在とする

        for (int l = 0; l < npLayerCount; l++) {
            for (int i = 0; i < 4; i++) {
                npLastFireFrame[l][i] = -10000;
            }
        }

        // ニューロン（常駐で明滅する大玉）とシナプス配線（静止した短レーザー）を1度だけ生成
        NewEnemyShotSet(ShotNodeBlink, 0.0, 0.0, 0.0, 0);
        NewEnemyShotSet(ShotSynapseLine, 0.0, 0.0, 0.0, 0);
    }

    const int cycleLength = 200;

    if (count % cycleLength == 1) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (count % cycleLength == 31) {
        // 入力層の全ニューロンを一斉発火させ、隠れ層1へ信号を送り出す
        for (int i = 0; i < npLayerSize[0]; i++) {
            SpawnPulseWave(NpNodeX(0, i), NpNodeY(0), 0, 1);
        }
    }
}