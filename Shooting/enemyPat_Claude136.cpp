#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕「噴水(Fontaine)」
//
//  ボスが画面中央付近でノズルになり、水を真上へ噴き上げる。
//  水は放物線を描いて左右に広がり、雨のように降り注ぐ。
//  最後は水面に落ちて波紋が広がる。
//
//  ・噴出(常時)   : 首振りノズルの水柱(中玉)＋水の流れ(小玉)＋飛沫(鱗弾)
//  ・頂点で分裂   : 水柱の中玉は頂点で小玉5発に砕け、傘状に降り注ぐ
//  ・波紋         : 水面に楕円リングを3重で広げる(中楕円弾/銃弾/小玉)
//  ・大噴出       : 予告音のあと、短レーザーの水柱＋大玉の螺旋が噴き上がり、
//                   頂点で砕けて滝のように降る
//
//  1サイクル 480 フレーム。
//     0〜359 : 通常の噴水＋波紋
//   360〜419 : 水が引く(予告音、飛沫が少し出るだけ)
//   420      : 大噴出
//   440〜479 : 噴水再開
//
//  弾数の目安(常時): 通常時は概算で 700〜1000 発が画面内に存在、
//  大噴出のピークは概算 1500〜1800 発。
//  密度は下の定数と、各Spawn関数内の発射数で調整できる。
// ============================================================

// ---- 全体の定数 ----
static const int    FTN_START_COUNT = 60;      // 登場が終わって弾幕を開始するcount
static const int    FTN_CYCLE = 480+60;     // 1サイクルのフレーム数
static const int    FTN_CHARGE_FRAME = 360;     // サイクル内: 水が引いて予告音
static const int    FTN_BURST_FRAME = 420;     // サイクル内: 大噴出
static const int    FTN_RESUME_FRAME = 440;     // サイクル内: 噴水再開
static const double FTN_BOSS_X = 240.0;   // ボス位置の中心x
static const double FTN_BOSS_Y = 240.0-60;   // ボス位置y(ノズルの高さ)
static const double FTN_GRAVITY = 0.07;    // 水の重力加速度
static const double FTN_FALL_MAX = 3.6;     // 水の落下速度の上限
static const double FTN_SURFACE_Y = 400.0+20;   // 水面(波紋の中心)のy

// ---- 弾の色 ----
static const int FTN_CYAN = 3;
static const int FTN_BLUE = 4;
static const int FTN_WHITE = 6;

// ---- ShotSetの種類(pEnemyShotSet->kind) ----
enum {
    FTN_SET_JET,      // 噴出(毎フレーム生成)
    FTN_SET_RIPPLE,   // 波紋
    FTN_SET_BURST     // 大噴出
};

// ---- 弾の param_d[] の使い方 ----
enum {
    FTN_VX = 0,       // x方向速度
    FTN_VY = 1,       // y方向速度
    FTN_GRAV = 2,     // 重力加速度(下向きが正)
    FTN_VMAX = 3,     // 落下速度の上限
    FTN_HX = 4,       // 螺旋の中心x(螺旋弾のみ)
    FTN_HR = 5,       // 螺旋の半径(0なら螺旋にしない)
    FTN_HW = 6,       // 螺旋の角速度
    FTN_HP = 7        // 螺旋の初期位相
};

// ---- 弾の param_i[] の使い方 ----
enum {
    FTN_APEX = 0,     // 頂点に達したときの動作(FTN_APEX_xxx)
    FTN_FIXED = 1,    // 1なら muki を移動方向に追従させない(波紋用)
    FTN_DONE = 2      // 頂点の動作を実行済みなら1
};

enum {
    FTN_APEX_NONE,    // 何もしない
    FTN_APEX_CORE,    // 中玉 → 小玉5発に分裂
    FTN_APEX_LASER,   // 短レーザー → しぶきの小玉を左右に散らす
    FTN_APEX_BIG      // 大玉 → 中玉と小玉の輪に砕ける
};

// 効果音を鳴らす(重複再生の前に止める)
static void PlaySe(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// 弾を1発追加する。速度は vx, vy で管理し、muki と speed は毎フレーム更新される。
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, int kind,
    double vx, double vy, double gravity, double fallMax)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->kind = kind;
    pShot->speed = sqrt(vx * vx + vy * vy);
    pShot->muki = atan2(vy, vx);
    pShot->param_d[FTN_VX] = vx;
    pShot->param_d[FTN_VY] = vy;
    pShot->param_d[FTN_GRAV] = gravity;
    pShot->param_d[FTN_VMAX] = fallMax;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ------------------------------------------------------------
//  頂点での分裂
// ------------------------------------------------------------

// 水柱の中玉: 親を小玉に変化させ、さらに4発追加して計5発の傘にする
static void SplitCore(sEnemyShotSet* pSet, sEnemyShot* pParent)
{
    double baseVx = pParent->param_d[FTN_VX] * 0.5;
    for (int k = 0; k < 5; k++) {
        double vx = baseVx + (k - 2) * 0.85 + (GetRand(40) - 20) / 100.0;
        double vy = (GetRand(80) - 40) / 100.0;   // 頂点でぴょこっと跳ねる
        int color = (k % 2 == 0) ? FTN_CYAN : FTN_BLUE;
        if (k == 0) {
            pParent->kind = img_enemyShotSmallBall[color];
            pParent->param_d[FTN_VX] = vx;
            pParent->param_d[FTN_VY] = vy;
            pParent->param_i[FTN_APEX] = FTN_APEX_NONE;
        }
        else {
            AddShot(pSet, pParent->x, pParent->y, img_enemyShotSmallBall[color], vx, vy, FTN_GRAVITY, FTN_FALL_MAX);
        }
    }
}

// 短レーザーの水柱: レーザー本体は落下を続け、しぶきの小玉を左右に散らす
static void SplitLaser(sEnemyShotSet* pSet, sEnemyShot* pParent)
{
    for (int k = 0; k < 6; k++) {
        double side = (k % 2 == 0) ? 1.0 : -1.0;
        double vx = side * (0.6 + (k / 2) * 0.7 + GetRand(40) / 100.0);
        double vy = -0.6 + GetRand(60) / 100.0;
        int color = (k % 3 == 0) ? FTN_WHITE : FTN_CYAN;
        AddShot(pSet, pParent->x, pParent->y, img_enemyShotSmallBall[color], vx, vy, 0.10, 3.6);
    }
    pParent->param_i[FTN_APEX] = FTN_APEX_NONE;
}

// 大玉の螺旋: 親を中玉に変化させ、中玉10発の輪と小玉8発の内輪を放射する
static void SplitBig(sEnemyShotSet* pSet, sEnemyShot* pParent)
{
    double base = GetRand(35) * DX_PI / 180.0;
    for (int k = 0; k < 10; k++) {
        double ang = base + 2.0 * DX_PI * k / 10.0;
        double sp = 1.6 + GetRand(100) / 100.0;
        int color = (k % 2 == 0) ? FTN_CYAN : FTN_BLUE;
        AddShot(pSet, pParent->x, pParent->y, img_enemyShotMediumBall[color],
            sp * cos(ang), sp * sin(ang) - 0.3, 0.09, 3.8);
    }
    for (int k = 0; k < 8; k++) {
        double ang = base + DX_PI / 10.0 + 2.0 * DX_PI * k / 8.0;
        double sp = 0.8 + GetRand(60) / 100.0;
        AddShot(pSet, pParent->x, pParent->y, img_enemyShotSmallBall[FTN_WHITE],
            sp * cos(ang), sp * sin(ang) - 0.2, 0.09, 3.4);
    }
    // 親は中玉になって真下へ落ちる
    pParent->kind = img_enemyShotMediumBall[FTN_CYAN];
    pParent->param_d[FTN_HR] = 0.0;
    pParent->param_d[FTN_VX] = 0.0;
    pParent->param_d[FTN_VY] = 0.4;
    pParent->param_i[FTN_APEX] = FTN_APEX_NONE;
}

// ------------------------------------------------------------
//  全弾の移動(重力つきの放物運動＋muki追従)
// ------------------------------------------------------------
static void UpdateShots(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double oldX = pShot->x;
        double oldY = pShot->y;

        // 重力(落下速度には上限あり)
        double vy = pShot->param_d[FTN_VY] + pShot->param_d[FTN_GRAV];
        if (vy > pShot->param_d[FTN_VMAX]) vy = pShot->param_d[FTN_VMAX];
        pShot->param_d[FTN_VY] = vy;

        // 移動(螺旋弾は中心を流しつつ、横に sin で振る)
        if (pShot->param_d[FTN_HR] != 0.0) {
            pShot->param_d[FTN_HX] += pShot->param_d[FTN_VX];
            pShot->x = pShot->param_d[FTN_HX]
                + pShot->param_d[FTN_HR] * sin(pShot->param_d[FTN_HW] * pShot->count + pShot->param_d[FTN_HP]);
        }
        else {
            pShot->x += pShot->param_d[FTN_VX];
        }
        pShot->y += vy;

        // 実際の移動量から speed と muki を更新(波紋の弾は muki を固定)
        double dx = pShot->x - oldX;
        double dy = pShot->y - oldY;
        pShot->speed = sqrt(dx * dx + dy * dy);
        if (pShot->param_i[FTN_FIXED] == 0) {
            if (pShot->speed > 0.0001) pShot->muki = atan2(dy, dx);
        }
        else {
            // 波紋は水面より上に出すぎたら画面外へ飛ばし、消去はメインルーチンに任せる
            if (pShot->y < FTN_SURFACE_Y - 70.0) pShot->y = -1000.0;
        }

        // 頂点(上昇が止まった瞬間)での動作
        if (pShot->param_i[FTN_APEX] != FTN_APEX_NONE && pShot->param_i[FTN_DONE] == 0 && vy >= 0.0) {
            pShot->param_i[FTN_DONE] = 1;
            switch (pShot->param_i[FTN_APEX]) {
            case FTN_APEX_CORE:  SplitCore(pSet, pShot);  break;
            case FTN_APEX_LASER: SplitLaser(pSet, pShot); break;
            case FTN_APEX_BIG:   SplitBig(pSet, pShot);   break;
            }
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  噴出(1フレーム分)
//    pSet->param_d[0] : ノズルの首振り角(rad)
//    pSet->param_i[0] : 水柱の中玉を撃つか(1で左右対称に2発)
//    pSet->param_i[1] : 水の流れ(小玉)の発数
//    pSet->param_i[2] : 飛沫(鱗弾)を撃つか
//    pSet->param_i[3] : 色の交互用の値
// ------------------------------------------------------------
static void SpawnJet(sEnemyShotSet* pSet)
{
    double sweep = pSet->param_d[0];
    int doCore = pSet->param_i[0];
    int flowNum = pSet->param_i[1];
    int doSpray = pSet->param_i[2];
    int parity = pSet->param_i[3];

    // 水柱の芯(中玉): 首振り角で左右対称に2発。頂点で小玉に砕ける
    if (doCore) {
        for (int side = -1; side <= 1; side += 2) {
            double ang = -DX_PI / 2.0 + sweep * side + (GetRand(100) - 50) / 100.0 * 0.06;
            double sp = 4.5 + GetRand(90) / 100.0;
            sEnemyShot* pShot = AddShot(pSet, pSet->x + GetRand(6) - 3, pSet->y,
                img_enemyShotMediumBall[FTN_CYAN], sp * cos(ang), sp * sin(ang), FTN_GRAVITY, FTN_FALL_MAX);
            pShot->param_i[FTN_APEX] = FTN_APEX_CORE;
        }
    }

    // 水の流れ(小玉): 真上中心の扇状。首振りにも少し引っ張られる
    for (int j = 0; j < flowNum; j++) {
        double ang = -DX_PI / 2.0 + (GetRand(200) - 100) / 100.0 * 0.30 + sweep * 0.3;
        double sp = 3.6 + GetRand(180) / 100.0;
        int color = ((parity + j) % 2 == 0) ? FTN_CYAN : FTN_BLUE;
        AddShot(pSet, pSet->x + GetRand(8) - 4, pSet->y,
            img_enemyShotSmallBall[color], sp * cos(ang), sp * sin(ang), FTN_GRAVITY, FTN_FALL_MAX);
    }

    // 飛沫(白い鱗弾): 広い角度にばら撒き、霧のようにゆっくり落ちる
    if (doSpray) {
        double ang = -DX_PI / 2.0 + (GetRand(200) - 100) / 100.0 * 1.05;
        double sp = 2.8 + GetRand(300) / 100.0;
        AddShot(pSet, pSet->x, pSet->y,
            img_enemyShotScale[FTN_WHITE], sp * cos(ang), sp * sin(ang), FTN_GRAVITY, 2.6);
    }
}

// ------------------------------------------------------------
//  波紋: 楕円リングを1つ追加する
//    弾は楕円の中心から放射状に、横長の比率(sx:sy)で広がる。
//    muki は楕円の接線方向に固定する。
// ------------------------------------------------------------
static void AddRippleRing(sEnemyShotSet* pSet, int kind, int num, double rx0, double ry0, double sx, double sy)
{
    double offset = GetRand(359) * DX_PI / 180.0;
    for (int i = 0; i < num; i++) {
        double th = offset + 2.0 * DX_PI * i / num;
        double c = cos(th);
        double s = sin(th);
        sEnemyShot* pShot = AddShot(pSet, pSet->x + rx0 * c, pSet->y + ry0 * s, kind, sx * c, sy * s, 0.0, 999.0);
        pShot->param_i[FTN_FIXED] = 1;
        pShot->muki = atan2(sy * c, -sx * s);   // 楕円の接線方向
    }
}

// 波紋(3重): 外側から 中楕円弾(シアン) / 銃弾(白) / 小玉(青)
static void SpawnRipple(sEnemyShotSet* pSet)
{
    AddRippleRing(pSet, img_enemyShotMediumOval[FTN_CYAN], 22, 18.0, 5.0, 2.4, 0.62);
    AddRippleRing(pSet, img_enemyShotBullet[FTN_WHITE], 30, 12.0, 3.5, 1.75, 0.45);
    AddRippleRing(pSet, img_enemyShotSmallBall[FTN_BLUE], 16, 8.0, 2.4, 1.1, 0.28);
}

// ------------------------------------------------------------
//  大噴出: pSet->count に合わせて40フレームかけて撃ち出す
// ------------------------------------------------------------
static void SpawnBurst(sEnemyShotSet* pSet)
{
    int c = pSet->count;

    // 短レーザーの水柱: 2フレームごとに1本、真上へ。頂点でしぶきを散らす
    if (c < 40 && c % 2 == 0) {
        double vx = (GetRand(60) - 30) / 100.0;
        int color = ((c / 2) % 2 == 0) ? FTN_CYAN : FTN_WHITE;
        sEnemyShot* pShot = AddShot(pSet, pSet->x, pSet->y, img_enemyShotLaser[color], vx, -6.0, 0.10, 4.2);
        pShot->param_i[FTN_APEX] = FTN_APEX_LASER;
    }

    // 大玉の螺旋: 4フレームごとに3本の腕から。頂点で砕けて滝になる
    if (c < 40 && c % 4 == 0) {
        for (int arm = 0; arm < 3; arm++) {
            double phase = arm * 2.0 * DX_PI / 3.0 + c * 0.09;
            sEnemyShot* pShot = AddShot(pSet, pSet->x, pSet->y, img_enemyShotLargeBall[FTN_BLUE], 0.0, -5.0, 0.09, 4.0);
            pShot->param_d[FTN_HR] = 46.0;
            pShot->param_d[FTN_HW] = 0.11;
            pShot->param_d[FTN_HP] = phase;
            pShot->param_d[FTN_HX] = pSet->x - 46.0 * sin(phase);   // count=0 のとき x が pSet->x に一致する
            pShot->param_i[FTN_APEX] = FTN_APEX_BIG;
        }
    }
}

// ShotSetのパターン関数(種類ごとに発射し、全弾を動かす)
static void FountainShotFunc(sEnemyShotSet* pSet)
{
    switch (pSet->kind) {
    case FTN_SET_JET:
        if (pSet->count == 0) SpawnJet(pSet);
        break;
    case FTN_SET_RIPPLE:
        if (pSet->count == 0) SpawnRipple(pSet);
        break;
    case FTN_SET_BURST:
        SpawnBurst(pSet);
        break;
    }
    UpdateShots(pSet);
}

// ShotSetを生成して enemyShotSetHead の末尾に連結する
static sEnemyShotSet* NewShotSet(int setKind, double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = FountainShotFunc;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = -DX_PI / 2.0;
    pSet->kind = setKind;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Fountain_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = FTN_BOSS_X;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }

    // 登場: 上から中央付近へ滑らかに降りる
    if (count <= FTN_START_COUNT) {
        double r = (double)count / FTN_START_COUNT;
        r = r * r * (3.0 - 2.0 * r);
        enemy.y = 40.0 + (FTN_BOSS_Y - 40.0) * r;
    }
    else {
        enemy.y = FTN_BOSS_Y;
    }
    // 噴水の水面に浮かぶように、左右にゆったり漂う
    enemy.x = FTN_BOSS_X + 30.0 * sin(count * 2.0 * DX_PI / FTN_CYCLE);

    int t = count - FTN_START_COUNT;
    if (t < 0) return;
    int ph = t % FTN_CYCLE;   // サイクル内の位置

    double nozzleX = enemy.x;
    double nozzleY = enemy.y - 10.0;

    // ノズルの首振り: 振り幅はサイクルを通して徐々に大きくなる
    double amp = (24.0 + 26.0 * ph / FTN_CYCLE) * DX_PI / 180.0;
    double sweep = amp * sin(2.0 * DX_PI * ph / 120.0);

    // ---- 噴出 ----
    bool jetOn = (ph < FTN_CHARGE_FRAME);
    int doCore = 0, flowNum = 0, doSpray = 0;
    if (jetOn) {
        doCore = (t % 10 == 0) ? 1 : 0;      // 水柱は3フレームに1回(左右2発)
        flowNum = (t % 7 == 0) ? 2 : 0;     // 水の流れは平均1.5発/フレーム
        doSpray = (t % 7 == 1) ? 1 : 0;     // 飛沫は2フレームに1発
    }
    else if (ph < FTN_BURST_FRAME && ph % 6 == 0) {
        doSpray = 1;                        // 水が引く間は、しずくがぽたぽた出るだけ
    }
    if (doCore || flowNum > 0 || doSpray) {
        sEnemyShotSet* pSet = NewShotSet(FTN_SET_JET, nozzleX, nozzleY);
        pSet->param_d[0] = sweep;
        pSet->param_i[0] = doCore;
        pSet->param_i[1] = flowNum;
        pSet->param_i[2] = doSpray;
        pSet->param_i[3] = t;
        if (jetOn && t % 8 == 0) PlaySe(sound_enemyShot_light);
    }

    // ---- 波紋: 水面のランダムな位置に。自機の真上には出さない ----
    if (ph % 30 == 10 && (ph < 350 || ph >= 450)) {
        double cx = 50.0 + GetRand(380);
        if (fabs(player.x - cx) < 60.0 && player.y > FTN_SURFACE_Y - 40.0) {
            cx += (player.x < cx) ? 90.0 : -90.0;
            if (cx < 30.0) cx = 30.0;
            if (cx > 450.0) cx = 450.0;
        }
        NewShotSet(FTN_SET_RIPPLE, cx, FTN_SURFACE_Y);
        PlaySe(sound_enemyShot_medium);
    }

    // ---- 予告音 → 大噴出 ----
    if (ph == FTN_CHARGE_FRAME) {
        PlaySe(sound_enemyCharge);
    }
    if (ph == FTN_BURST_FRAME) {
        NewShotSet(FTN_SET_BURST, nozzleX, nozzleY);
        PlaySe(sound_enemyShot_extreme);
    }
}