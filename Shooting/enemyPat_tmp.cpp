#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕:ニュートンのゆりかご「慣性の伝達」
// ------------------------------------------------------------
//  構成:
//    おもり ... 中玉。両端の玉(列番号0と4)が交互に振り子運動する
//    糸 ..... 小玉(白)をアンカー〜おもり間に6分割で並べた点線
//
//  動作(240フレーム周期でループ):
//    0〜 60F:右の玉が最下点→右へ減速しながら上昇
//     60〜120F:右の玉が加速しながら下降(折り返し点でプレイヤー狙い弾)
//    120F:最下点で衝突!衝撃波が発生し、左の玉が同じ角度まで押し出される
//    120〜240F:今度は左の玉が同じ運動をして戻ってくる(慣性の伝達)
//
//  難易度展開(グローバルcountで判定):
//    Phase1(〜600F):衝撃波+狙い弾
//    Phase2(600〜1200F):衝突時に列中央から狙い3Way追加
//    Phase3(1200F〜):衝撃波が12Way化、狙い5Way化、
//                     最下点のおもりから左右へ中玉追加
//
//  弾の役割は sEnemyShot::param_i[0] で管理:
//    0:自由飛翔弾(等速直線移動)  1:おもり  2:糸
//    param_i[1]:おもり/糸のグループ番号(玉の列番号 0〜4)
//    param_i[2]:糸の分割番号
// ============================================================

static constexpr double CRADLE_LENGTH = 100.0;        // 糸の長さ
static constexpr double CRADLE_SPACING = 16.0;         // おもり同士の間隔
static constexpr double CRADLE_THETA_MAX = DX_PI / 3.0;  // 振り子の最大角度
static constexpr int    CRADLE_QUARTER = 60/2;           // 1/4周期(フレーム)
static constexpr int    CRADLE_CYCLE = CRADLE_QUARTER * 4; // 240F
static constexpr int    CRADLE_STRINGS = 6;            // 糸の分割数

// 弾を生成してリストに登録するヘルパ
static sEnemyShot* CradleCreateShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind, int role)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = role;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// ゆりかご本体
static void CradleNewton(sEnemyShotSet* pEnemyShotSet)
{
    // アンカーは敵(本体)の位置に追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    const double ax = pEnemyShotSet->x;
    const double ay = pEnemyShotSet->y;

    const int t = pEnemyShotSet->count % CRADLE_CYCLE;

    // ---- 振り子の角度計算 ----
    // 上昇は正弦(減速)、下降は余弦(加速)で釣り針のような natural な動きにする
    double thR = 0.0, thL = 0.0;
    if (t < CRADLE_QUARTER * 2) {
        double s = (t < CRADLE_QUARTER)
            ? sin(DX_PI / 2.0 * t / CRADLE_QUARTER)
            : cos(DX_PI / 2.0 * (t - CRADLE_QUARTER) / CRADLE_QUARTER);
        thR = CRADLE_THETA_MAX * s;
    }
    else {
        double u = t - CRADLE_QUARTER * 2;
        double s = (u < CRADLE_QUARTER)
            ? sin(DX_PI / 2.0 * u / CRADLE_QUARTER)
            : cos(DX_PI / 2.0 * (u - CRADLE_QUARTER) / CRADLE_QUARTER);
        thL = -CRADLE_THETA_MAX * s;
    }

    // おもり5個の位置(0=左端 〜 4=右端)
    double ballX[5], ballY[5];
    for (int i = 0; i < 5; i++) {
        ballX[i] = ax + (i - 2) * CRADLE_SPACING;
        ballY[i] = ay + CRADLE_LENGTH;
    }
    ballX[4] += CRADLE_LENGTH * sin(thR);
    ballY[4] = ay + CRADLE_LENGTH * cos(thR);
    ballX[0] += CRADLE_LENGTH * sin(thL);
    ballY[0] = ay + CRADLE_LENGTH * cos(thL);

    // ---- 初回のみ:おもりと糸を配置 ----
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 5; i++) {
            // おもり(中玉・橙)
            sEnemyShot* pBall = CradleCreateShot(pEnemyShotSet, ballX[i], ballY[i],
                0.0, 0.0, img_enemyShotMediumBall[8], 1);
            pBall->param_i[1] = i;
            // 糸(小玉・白の点線)
            for (int j = 1; j < CRADLE_STRINGS; j++) {
                sEnemyShot* pStr = CradleCreateShot(pEnemyShotSet, ballX[i], ballY[i],
                    0.0, 0.0, img_enemyShotSmallBall[6], 2);
                pStr->param_i[1] = i;
                pStr->param_i[2] = j;
            }
        }
    }

    // ---- 衝突イベント(最下点で玉がぶつかる瞬間) ----
    if (t == CRADLE_QUARTER * 2 || (t == 0 && pEnemyShotSet->count > 0)) {
        // どちら側の玉が衝突したか(t==120:右 / t==0:左)
        int side = (t == 0) ? -1 : 1;
        double cx = ax + side * CRADLE_SPACING * 1.5;   // 接触点
        double cy = ay + CRADLE_LENGTH;

        // 衝撃波:接触点から全周放射(ランダム回転で毎回変化)
        int nWave = (count > 800) ? 12*2 : 10*2;
        double base = GetRand(360) / 180.0 * DX_PI;
        for (int k = 0; k < nWave; k++) {
            double m = base + DX_PI * 2.0 * k / nWave;
            CradleCreateShot(pEnemyShotSet, cx, cy, m, 2.5,
                img_enemyShotSmallBall[3], 0);   // シアンの小玉
        }

        if (count > 400) {
            // 列の中央からプレイヤー狙いの拡散(マゼンタの鱗弾)
            int nAim = (count > 800) ? 5*2+1 : 3*2+1;
            double aim = atan2(player.y - cy, player.x - ax);
            for (int k = 0; k < nAim; k++) {
                double m = aim + ((nAim - 1) / 2.0 - k) * 0.22;
                CradleCreateShot(pEnemyShotSet, ax, cy, m, 3.4,
                    img_enemyShotScale[5], 0);
            }
        }

        if (count > 800) {
            // 最下点に来たばかりのおもりから左右へ中玉(クッション役)
            double bx = ax + side * 2 * CRADLE_SPACING;
            CradleCreateShot(pEnemyShotSet, bx, cy, DX_PI, 2.0, img_enemyShotMediumBall[0], 0);
            CradleCreateShot(pEnemyShotSet, bx, cy, 0.0, 2.0, img_enemyShotMediumBall[0], 0);
        }

        // 衝突音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // ---- 振り上げきわ(おもりの折り返し点):予告音+狙い弾 ----
    if (t == CRADLE_QUARTER || t == CRADLE_QUARTER * 3) {
        int bi = (t == CRADLE_QUARTER) ? 4 : 0;
        double aim = atan2(player.y - ballY[bi], player.x - ballX[bi]);
        CradleCreateShot(pEnemyShotSet, ballX[bi], ballY[bi], aim, 3.0*2,
            img_enemyShotSmallBall[1], 0);   // 黄の小玉

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // ---- 各弾の更新 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case 0: // 自由飛翔弾:等速直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        case 1: // おもり:振り子位置に直接配置
        {
            int i = pShot->param_i[1];
            pShot->x = ballX[i];
            pShot->y = ballY[i];
            // 振れている玉は赤で強調し、主従をはっきりさせる
            pShot->kind = ((t < CRADLE_QUARTER * 2 && i == 4) ||
                (t >= CRADLE_QUARTER * 2 && i == 0))
                ? img_enemyShotMediumBall[0] : img_enemyShotMediumBall[8];
            break;
        }
        case 2: // 糸:アンカー〜おもり間を分割した点に直接配置
        {
            int i = pShot->param_i[1];
            double r = pShot->param_i[2] / (double)CRADLE_STRINGS;
            pShot->x = ax + (ballX[i] - ax) * r;
            pShot->y = ay + (ballY[i] - ay) * r;
            break;
        }
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Tmp()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 150.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }
    // アンカー(敵本体)ごとゆっくり揺らして、吊り下げ感を出す
    enemy.x = 240.0 + 14.0 * sin(count * 0.015);

    if (count == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = CradleNewton;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}