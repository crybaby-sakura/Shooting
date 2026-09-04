// enemyPat_Tmp.cpp
//
// ボスがフラッシュ暗算を出題する敵パターン。
//   ・答えは「現在HP-30 ～ 現在HP-10」からランダムに決定
//   ・その答えを複数の数に分解し、7セグを"敵弾"で描いて一つずつフラッシュ表示
//   ・既定時間内にプレイヤーが ボスHP == 答え にできたか判定
//       成功 : 青・中玉・薄めの「◯」全方位弾（画面中央から放射状に等速拡散）
//       失敗 : 赤・中玉・濃いめの「✕」形。画面対角線2本の上に初期配置し、
//              画面中央→自機方向へ若干の乱数を加えて全弾射出。
//
// ★ 7セグ表示・残り時間ゲージは DrawBox を使わず「速度0の小玉(敵弾)」を
//    格子状に並べて描画する。表示専用の弾セットを1つ常駐させ、毎フレーム
//    中身を作り直して内容を更新する。
//
// ※ count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//    画面外弾の消去はメインルーチン側で行う仕様（本ファイルでは行わない）。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾の色インデックス:  0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙

// #define ADD_ANCHOR 1

// 画面サイズ
static const double SCREEN_W = 480.0;
static const double SCREEN_H = 480.0;

// =====================================================================
//  表示用: 弾セットへ小玉を1個追加（速度0でその場に留まる）
// =====================================================================
static void AddDot(sEnemyShotSet* pSet, double x, double y, int colorIdx)
{
    sEnemyShot* s = new sEnemyShot;
    s->x = x;
    s->y = y;
    s->muki = 0.0;
    s->speed = 0.0;                              // 移動しない＝表示専用
    s->kind = img_enemyShotSmallBall[colorIdx]; // 小玉(2.5x2.5)
    s->prev = pSet->pEnemyShotHead->prev;
    s->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = s;
    pSet->pEnemyShotHead->prev = s;
}

// 2点間に n 個の小玉を等間隔で並べる（セグメント1本ぶん）
static void AddSegLine(sEnemyShotSet* pSet, double x1, double y1,
    double x2, double y2, int n, int colorIdx)
{
    for (int i = 0; i < n; i++) {
        const double tt = (n == 1) ? 0.0 : (double)i / (n - 1);
        AddDot(pSet, x1 + (x2 - x1) * tt, y1 + (y2 - y1) * tt, colorIdx);
    }
}

// 7セグ1桁を (ox,oy)左上, セル幅W・高さH で描く
static void DrawDigitShots(sEnemyShotSet* pSet, double ox, double oy,
    double W, double H, int digit, int colorIdx)
{
    if (digit < 0 || digit > 9) return;
    // a,b,c,d,e,f,g の点灯パターン
    static const int seg[10][7] = {
        {1,1,1,1,1,1,0}, // 0
        {0,1,1,0,0,0,0}, // 1
        {1,1,0,1,1,0,1}, // 2
        {1,1,1,1,0,0,1}, // 3
        {0,1,1,0,0,1,1}, // 4
        {1,0,1,1,0,1,1}, // 5
        {1,0,1,1,1,1,1}, // 6
        {1,1,1,0,0,0,0}, // 7
        {1,1,1,1,1,1,1}, // 8
        {1,1,1,1,0,1,1}, // 9
    };
    const int* s = seg[digit];
    const double hy = oy + H / 2.0;
    const int nH = 7; // 横セグメントの弾数（3倍サイズに合わせ増量）
    const int nV = 7; // 縦セグメントの弾数
    if (s[0]) AddSegLine(pSet, ox, oy, ox + W, oy, nH, colorIdx); // a 上横
    if (s[1]) AddSegLine(pSet, ox + W, oy, ox + W, hy, nV, colorIdx); // b 右上縦
    if (s[2]) AddSegLine(pSet, ox + W, hy, ox + W, oy + H, nV, colorIdx); // c 右下縦
    if (s[3]) AddSegLine(pSet, ox, oy + H, ox + W, oy + H, nH, colorIdx); // d 下横
    if (s[4]) AddSegLine(pSet, ox, hy, ox, oy + H, nV, colorIdx); // e 左下縦
    if (s[5]) AddSegLine(pSet, ox, oy, ox, hy, nV, colorIdx); // f 左上縦
    if (s[6]) AddSegLine(pSet, ox, hy, ox + W, hy, nH, colorIdx); // g 中横
}

// 数値を (cx,cy) 中央基準で7セグ描画（サイズは従来の3倍）
static void DrawNumberShots(sEnemyShotSet* pSet, double cx, double cy,
    int number, int colorIdx)
{
    if (number < 0) number = 0;
    int digits[8], n = 0;
    if (number == 0) {
        digits[n++] = 0;
    }
    else {
        int tmp = number;
        while (tmp > 0 && n < 8) { digits[n++] = tmp % 10; tmp /= 10; }
        for (int i = 0; i < n / 2; i++) { int a = digits[i]; digits[i] = digits[n - 1 - i]; digits[n - 1 - i] = a; }
    }
    const double W = 42.0, H = 72.0, gap = 30.0; // 3倍(14,24,10 -> 42,72,30)
    const double totalW = n * W + (n - 1) * gap;
    const double startX = cx - totalW / 2.0;
    const double y = cy - H / 2.0;
    for (int i = 0; i < n; i++)
        DrawDigitShots(pSet, startX + i * (W + gap), y, W, H, digits[i], colorIdx);
}

// 残り時間ゲージを小玉の一列で描く（ratio: 0..100 の残量、サイズは従来の2倍）
static void DrawBarShots(sEnemyShotSet* pSet, double cx, double cy,
    int ratio, int colorIdx)
{
    const int total = 24;        // 2倍(12 -> 24)
    const double barW = 240.0;   // 2倍(120 -> 240)
    const double startX = cx - barW / 2.0;
    int lit = ratio * total / 100;
    if (lit < 0) lit = 0;
    if (lit > total) lit = total;
    for (int i = 0; i < total; i++) {
        const double x = startX + barW * i / (total - 1);
        AddDot(pSet, x, cy, (i < lit) ? colorIdx : 7);
    }
}

// =====================================================================
//  表示専用弾セットのパターン関数
//    param_i[0]=mode(0:消灯 1:数字 2:ゲージ), [1]=数値, [2]=残量%, [3]=色
//    param_d[0],[1]=表示中心X,Y   param_d[2],[3]=アンカー(常駐弾)X,Y
// =====================================================================
static void ShotDisplay(sEnemyShotSet* pSet)
{
    // 自分が抱える表示弾を一旦すべて破棄
    sEnemyShot* head = pSet->pEnemyShotHead;
    sEnemyShot* p = head->next;
    while (p != head) { sEnemyShot* nx = p->next; delete p; p = nx; }
    head->next = head;
    head->prev = head;

    const int mode = pSet->param_i[0];
    const int color = pSet->param_i[3];
    const double cx = pSet->param_d[0];
    const double cy = pSet->param_d[1];

#if ADD_ANCHOR
    AddDot(pSet, pSet->param_d[2], pSet->param_d[3], 7); // 空セット防止のアンカー
#endif

    if (mode == 1)      DrawNumberShots(pSet, cx, cy, pSet->param_i[1], color);
    else if (mode == 2) DrawBarShots(pSet, cx, cy, pSet->param_i[2], color);
}

// =====================================================================
//  成功時弾幕： 青・中玉・薄めの「◯」全方位弾
//    画面中央から放射状に等速拡散（弾数2倍）
// =====================================================================
static void ShotCircleSpread(sEnemyShotSet* pSet)
{
    sEnemyShot* s;
    const int NUM = 32; // 2倍(16 -> 32)、密度は薄め

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < NUM; i++) {
            s = new sEnemyShot;
            s->x = SCREEN_W / 2.0; // 画面中央から
            s->y = SCREEN_H / 2.0;
            s->muki = 2.0 * DX_PI * i / NUM; // 全方位
            s->speed = 2.2;
            s->kind = img_enemyShotMediumBall[4]; // 青・中玉
            s->prev = pSet->pEnemyShotHead->prev;
            s->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = s;
            pSet->pEnemyShotHead->prev = s;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// =====================================================================
//  失敗時弾幕： 赤・中玉・濃いめの「✕」
//    画面の対角線2本の上に初期弾を配置し、画面中央→自機方向に
//    若干の乱数を加えて全弾射出。
// =====================================================================
static void ShotXAimed(sEnemyShotSet* pSet)
{
    sEnemyShot* s;
    const int PER = 21 * 5; // 各対角線あたりの弾数（濃いめ）

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 画面中央 → 自機方向（基準の向き）
        const double aim = atan2(player.y - SCREEN_H / 2.0,
            player.x - SCREEN_W / 2.0);
        const double sp = 2.6;

        for (int line = 0; line < 2; line++) {
            for (int i = 0; i < PER; i++) {
                if (line == 1 && i == PER / 2) continue; // 中心の重複を回避

                s = new sEnemyShot;
                const double u = (double)i / (PER - 1); // 0..1
                double px, py;
                if (line == 0) {           // 左上→右下 の対角線
                    px = SCREEN_W * u;
                    py = SCREEN_H * u;
                }
                else {                   // 右上→左下 の対角線
                    px = SCREEN_W * (1.0 - u);
                    py = SCREEN_H * u;
                }
                s->x = px;
                s->y = py;
                // 中心→自機方向に ±約8度の乱数を加える（GetRand(16)=0..16）
                s->muki = aim + (GetRand(16) - 8) / 180.0 * DX_PI;
                s->speed = sp;
                s->kind = img_enemyShotMediumBall[0]; // 赤・中玉
                s->prev = pSet->pEnemyShotHead->prev;
                s->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = s;
                pSet->pEnemyShotHead->prev = s;
            }
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// =====================================================================
//  弾セットを1つ生成してリストへ登録し、そのポインタを返す
// =====================================================================
static sEnemyShotSet* SpawnShotSet(sEnemyShotSet::PatternFunc func,
    double x, double y, double muki)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = 0;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// =====================================================================
//  敵本体パターン
// =====================================================================
void EnemyPat_FlashMentalArithmetic()
{
    enum { PH_GEN = 0, PH_FLASH, PH_ANSWER, PH_JUDGE };

    static int muki;                 // 左右移動方向
    static int phase;                // 現在フェーズ
    static int phaseStart;           // フェーズ開始時の count
    static int answer;               // 今回の答え
    static int nums[8];              // フラッシュ表示する数列
    static int terms;                // 数列の項数
    static int onFrames;             // 今回の7セグ点灯フレーム数
    static sEnemyShotSet* pDisp;     // 表示専用弾セット

    // ---- 初期化・移動 ----
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = PH_GEN;
        phaseStart = count;

        pDisp = SpawnShotSet(ShotDisplay, enemy.x, enemy.y, 0.0);
        pDisp->alive = 99999;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
    }

    // 表示セットの共通パラメータ（毎フレーム更新）
    pDisp->param_d[2] = enemy.x;        // アンカー弾（ボス中心）
    pDisp->param_d[3] = enemy.y;
    pDisp->param_i[0] = 0;              // 既定は消灯
    pDisp->param_d[0] = enemy.x;        // 表示中心X
    pDisp->param_d[1] = enemy.y + 90.0; // 表示中心Y（3倍サイズぶん下げる）

    const int t = count - phaseStart;

    switch (phase) {

        // ------------------------------------------------------------
    case PH_GEN: { // 出題内容を生成
        // 答え = 現在HP-30 ～ 現在HP-10  （GetRand(20)=0..20 の21通り）
        answer = enemy.hp - 10 - GetRand(20);
        if (answer < 1) answer = 1;

        // 答えを terms 個の数に分解（各項 >= 1, 合計 == answer）
        terms = 4;
        if (terms > answer) terms = answer;
        if (terms < 1)      terms = 1;

        int rem = answer;
        for (int i = 0; i < terms - 1; i++) {
            const int slots = terms - i;
            int maxv = rem - (slots - 1);
            if (maxv < 1) maxv = 1;
            const int base = rem / slots;
            int v = base + GetRand(base) - base / 2;
            if (v < 1)    v = 1;
            if (v > maxv) v = maxv;
            nums[i] = v;
            rem -= v;
        }
        nums[terms - 1] = rem;

        {
            double hp = (double)enemy.hp;
            if (hp < 0.0)   hp = 0.0;
            if (hp > 200.0) hp = 200.0;
            const double factor = 0.5 + (3.0 - 0.5) * (hp / 200.0);
            onFrames = (int)(34.0 * factor + 0.5);
            if (onFrames < 1) onFrames = 1;
        }

        phase = PH_FLASH;
        phaseStart = count;
        break;
    }

               // ------------------------------------------------------------
    case PH_FLASH: { // フラッシュ暗算表示（1つずつ）
        const int ON = onFrames, OFF = 14;
        const int cycle = ON + OFF;
        const int idx = t / cycle;
        const int inCyc = t % cycle;

        if (idx < terms) {
            if (inCyc == 0) {
                if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            }
            if (inCyc < ON) {
                pDisp->param_i[0] = 1;         // mode=数字
                pDisp->param_i[1] = nums[idx];
                pDisp->param_i[3] = 2;         // 緑
            }
        }
        else {
            phase = PH_ANSWER;
            phaseStart = count;
        }
        break;
    }

                 // ------------------------------------------------------------
    case PH_ANSWER: { // 解答時間（この間にHPを答えに合わせる）
        const int LIMIT = 240;
        int remain = LIMIT - t;
        if (remain < 0) remain = 0;

        pDisp->param_i[0] = 2;                    // mode=ゲージ
        pDisp->param_i[2] = remain * 100 / LIMIT; // 残量%
        pDisp->param_i[3] = 1;                    // 黄
        pDisp->param_d[1] = enemy.y + 70.0;       // ゲージ位置

        if (remain <= 0) {
            phase = PH_JUDGE;
            phaseStart = count;
        }
        break;
    }

                  // ------------------------------------------------------------
    case PH_JUDGE: { // 判定して弾幕発射
        const bool success = (enemy.hp == answer);

        if (success) {
            SpawnShotSet(ShotCircleSpread, SCREEN_W / 2.0, SCREEN_H / 2.0, 0.0);
        }
        else {
            SpawnShotSet(ShotXAimed, SCREEN_W / 2.0, SCREEN_H / 2.0, 0.0);
        }

        phase = PH_GEN;
        phaseStart = count;
        break;
    }

    } // switch
}
