// enemyPat_tmp.cpp
//
// 弾幕「境界面の潜航（ヒルベルト・カーブ）」
//
// 【概要】
//   画面全体にヒルベルト曲線を「一筆書き」していく弾幕。
//   ・先頭弾（中玉）がヒルベルト曲線に沿って這っていく
//   ・通過した地点に小玉を置いていく（寿命で消える＝通過した場所は再び安全になる）
//   ・プレイヤーはフラクタルの規則性を読み、「次にどこが埋まるか」を先読みして逃げる
//
// 【フェイス構成】（c = (count-1) % 2600）
//   フェイス1 (c=    0〜 899)：8x8 の曲線をゆっくり一筆書き
//   フェイス2 (c=  900〜1699)：16x16 の曲線を両端から2本で挟み撃ち
//   フェイス3 (c=1700〜2599)：8x8 の曲線を高速往復（ピンポン）でなぞる
//
// 【仕様上の注意】
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメントはメイン側で行われる
//   ・画面外の弾の消去もメイン側で行われる
//     → 弾を消したい時は画面外 (x = y = -9999) へ移動し、メインに消してもらう

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ヒルベルト曲線ユーティリティ
// ============================================================

// 象限の回転・反転（部分正方形の一辺 s を渡す）
static void HilbertRot(int s, int* x, int* y, int rx, int ry)
{
    if (ry == 0) {
        if (rx == 1) {
            *x = s - 1 - *x;
            *y = s - 1 - *y;
        }
        int t = *x; // x, y を入れ替え
        *x = *y;
        *y = t;
    }
}

// 曲線上インデックス d (0 〜 grid*grid-1) → セル座標 (x, y)
static void HilbertD2XY(int grid, int d, int* x, int* y)
{
    int rx, ry, s, t = d;
    *x = *y = 0;
    for (s = 1; s < grid; s *= 2) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);
        HilbertRot(s, x, y, rx, ry);
        *x += s * rx;
        *y += s * ry;
        t /= 4;
    }
}

// 曲線上の位置 idx（小数可）→ 画面座標。隣接セル中心間を線形補間する
static void HilbertPos(int grid, double idx, double* px, double* py)
{
    int n = grid * grid;
    if (idx < 0.0)          idx = 0.0;
    if (idx > (double)(n - 1)) idx = (double)(n - 1);

    int d = (int)idx;
    double f = idx - (double)d;
    int x0, y0, x1, y1;
    HilbertD2XY(grid, d, &x0, &y0);
    if (d + 1 < n) HilbertD2XY(grid, d + 1, &x1, &y1);
    else { x1 = x0; y1 = y0; }

    double cs = 480.0 / (double)grid; // 1セルの大きさ
    *px = ((double)x0 + 0.5 + (double)(x1 - x0) * f) * cs;
    *py = ((double)y0 + 0.5 + (double)(y1 - y0) * f) * cs;
}

// 0→(n-1)→0 を折り返す（ピンポンモード用）
static double HilbertFold(int n, double h)
{
    double m = (double)(n - 1);
    double r = fmod(h, 2.0 * m);
    if (r < 0.0) r += 2.0 * m;
    return (r <= m) ? r : (2.0 * m - r);
}

// ============================================================
//  弾の生成補助
// ============================================================

// 弾を1つ生成してセットの弾リストの末尾に繋ぐ
static void LinkBullet(sEnemyShotSet* pSet, double x, double y, int img, int life, int isHead)
{
    sEnemyShot* s = new sEnemyShot;
    s->x = x;
    s->y = y;
    s->muki = 0.0;
    s->speed = 0.0;      // 蛇の弾は静止弾（位置は生成時に確定）
    s->kind = img;
    s->param_i[0] = life;   // 寿命（フレーム）
    s->param_i[1] = isHead; // 1:先頭弾
    s->param_i[2] = 0;      // 消滅フラグ

    s->prev = pSet->pEnemyShotHead->prev;
    s->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = s;
    pSet->pEnemyShotHead->prev = s;
}

// ============================================================
//  弾パターン：ヒルベルトの蛇
// ============================================================
//  sEnemyShotSet 側のパラメータ
//    param_i[0] : grid        曲線の分割数（8 または 16）
//    param_i[1] : dir         +1:曲線先頭から / -1:曲線終端から
//    param_i[2] : mode        0:一方向で周回 / 1:折り返し往復（ピンポン）
//    param_i[3] : duration    先頭弾が這い続けるフレーム数
//    param_i[4] : maxLaps     一方向モードの最大周回数
//    param_i[5] : color       弾の色
//    param_i[6] : ballType    0:小玉を置く / 1:中玉を置く
//    param_i[7] : (実行時) 最後に置いた弾の通過距離インデックス
//    param_i[8] : (実行時) 完了した周回数
//    param_i[9] : (実行時) 先頭弾が生存中か
//    param_d[0] : speed       進行速度（セル/フレーム）
//    param_d[1] : (実行時) 先頭の通過距離 h（セル）
//    param_d[2] : spacing     弾を置く間隔（セル）
//    param_d[3] : trailLife   置いた弾の寿命（フレーム）
static void ShotHilbertSnake(sEnemyShotSet* p)
{
    int grid = p->param_i[0];
    int dir = p->param_i[1];
    int mode = p->param_i[2];
    int duration = p->param_i[3];
    int maxLaps = p->param_i[4];
    int color = p->param_i[5];
    int ballType = p->param_i[6];
    double speed = p->param_d[0];
    double spacing = p->param_d[2];
    double life = p->param_d[3];
    int n = grid * grid;
    int depositImg = (ballType == 1) ? img_enemyShotMediumBall[color]
        : img_enemyShotSmallBall[color];

    if (p->count == 0) {
        // 初期化＋先頭弾（中玉）生成
        p->param_d[1] = 0.0;
        p->param_i[7] = 0;      // 最初の1個は先頭弾と重なるので置かない
        p->param_i[8] = 0;
        p->param_i[9] = 1;

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double posIdx = (dir > 0) ? 0.0 : (double)(n - 1);
        double hx, hy;
        HilbertPos(grid, posIdx, &hx, &hy);
        LinkBullet(p, hx, hy, img_enemyShotLargeBall[color], 0, 1);
    }

    if (p->param_i[9]) {
        // ---- 先頭を進める（h は通過距離）----
        double h = p->param_d[1] + speed;

        if (mode == 0 && h >= (double)(n - 1)) {
            // 曲線の終端に到達
            if (p->param_i[8] + 1 < maxLaps && p->count < duration) {
                // 周回数が残っていれば曲線先頭に戻って描き直す
                h -= (double)(n - 1);
                p->param_i[8]++;
                p->param_i[7] = -1;
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
            else {
                h = (double)(n - 1);
                p->param_i[9] = 0; // 先頭弾を消す（置き弾は寿命で自然に消える）
            }
        }
        if (p->count >= duration) {
            p->param_i[9] = 0; // 活動時間の上限でも先頭弾を消す
        }
        p->param_d[1] = h;

        // ---- 通過した地点に弾を置く ----
        int didx = (int)(h / spacing);
        if (didx > p->param_i[7]) {
            p->param_i[7] = didx;
            double base = (double)didx * spacing; // 等間隔に置く
            double posIdx = (mode == 1) ? HilbertFold(n, base)
                : ((dir > 0) ? base : (double)(n - 1) - base);
            double bx, by;
            HilbertPos(grid, posIdx, &bx, &by);
            LinkBullet(p, bx, by, depositImg, (int)life, 0);
        }
    }

    // ---- 全弾の更新 ----
    sEnemyShot* s = p->pEnemyShotHead->next;
    while (s != p->pEnemyShotHead) {
        if (s->param_i[2] == 0) {
            if (s->param_i[1] == 1) {
                // 先頭弾：曲線上を移動し続ける
                if (p->param_i[9]) {
                    double posIdx = (mode == 1) ? HilbertFold(n, p->param_d[1])
                        : ((dir > 0) ? p->param_d[1] : (double)(n - 1) - p->param_d[1]);
                    HilbertPos(grid, posIdx, &s->x, &s->y);
                }
                else {
                    s->param_i[2] = 1;
                    s->x = -9999.0; s->y = -9999.0; // メインルーチンに消してもらう
                }
            }
            else {
                // 置き弾：寿命が来たら画面外へ移動して消滅させる
                if (s->count >= s->param_i[0]) {
                    s->param_i[2] = 1;
                    s->x = -9999.0; s->y = -9999.0;
                }
            }
        }
        s = s->next;
    }
}

// ============================================================
//  弾パターン：自機狙い（フェイス中の牽制用）
// ============================================================
//    param_i[0] : num      同時発射数
//    param_i[1] : color    弾の色
//    param_d[0] : speed    弾速
//    param_d[1] : spread   扇の広さ（ラジアン）
static void ShotAimed(sEnemyShotSet* p)
{
    if (p->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int num = p->param_i[0];
        for (int i = 0; i < num; i++) {
            sEnemyShot* s = new sEnemyShot;
            s->x = p->x;
            s->y = p->y;
            double off = (num > 1) ? ((double)i / (double)(num - 1) - 0.5) : 0.0;
            s->muki = p->muki + off * p->param_d[1];
            s->speed = p->param_d[0];
            s->kind = img_enemyShotMediumBall[p->param_i[1]];

            s->prev = p->pEnemyShotHead->prev;
            s->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = s;
            p->pEnemyShotHead->prev = s;
        }
    }

    sEnemyShot* s = p->pEnemyShotHead->next;
    while (s != p->pEnemyShotHead) {
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
        s = s->next;
    }
}

// ============================================================
//  セット生成ヘルパ
// ============================================================
static void CreateHilbertSet(int grid, int dir, int mode, int duration, int maxLaps,
    double speed, double spacing, int trailLife, int color, int ballType)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = ShotHilbertSnake;
    p->x = enemy.x;
    p->y = enemy.y;
    p->muki = 0.0;
    p->kind = color;
    p->param_i[0] = grid;
    p->param_i[1] = dir;
    p->param_i[2] = mode;
    p->param_i[3] = duration;
    p->param_i[4] = maxLaps;
    p->param_i[5] = color;
    p->param_i[6] = ballType;
    p->param_d[0] = speed;
    p->param_d[2] = spacing;
    p->param_d[3] = (double)trailLife;

    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;

    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
}

static void CreateAimedSet(int num, double speed, double spreadRad, int color)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = ShotAimed;
    p->x = enemy.x;
    p->y = enemy.y + 12.0;
    p->muki = atan2(player.y - p->y, player.x - p->x);
    p->kind = color;
    p->param_i[0] = num;
    p->param_i[1] = color;
    p->param_d[0] = speed;
    p->param_d[1] = spreadRad;

    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;

    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_HilbertCurve_Zai()
{
    // フェイスサイクル（フレーム）
    const int CYCLE = 2600 - 300;
    const int PHASE2_START = 900 - 100;
    const int PHASE3_START = 1700 - 200;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 300; // 200で固定
    }

    // サイクル内経過フレーム
    int c = (count - 1) % CYCLE;

    // ボスは画面上部をゆらゆら漂う
    enemy.x = 240.0 + 70.0 * sin(c * 0.004) + 15.0 * sin(c * 0.013);
    enemy.y = 60.0 + 12.0 * sin(c * 0.009);

    // ---- フェイス開始の予告音 ----
    if (c == 20 || c == PHASE2_START - 60 || c == PHASE3_START - 60) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ---- フェイス1：8x8 の曲線を一筆書き ----
    // ゆっくり這う蛇。尾（置き弾の残る範囲）は約24セル分。
    if (c == 60) {
        CreateHilbertSet(8, +1, 0, 99999, 1, 0.11, 0.5/4, 220*4, 6, 0);
    }

    // ---- フェイス2：16x16 の曲線を両端から挟み撃ち ----
    // 白い蛇が曲線先頭から、シアンの蛇が終端から進み、中盤で鉢合わせする。
    if (c == PHASE2_START + 60) {
        CreateHilbertSet(16, +1, 0, 600, 99, 0.22, 0.8/3, 160*4, 6, 0);
    }
    if (c == PHASE2_START + 90) {
        CreateHilbertSet(16, -1, 0, 570, 99, 0.22, 0.8/3, 160*4, 3, 0);
    }

    // ---- フェイス3：8x8 の曲線を高速往復 ----
    // 完成した曲線上を折り返しながら高速でなぞり続ける。
    if (c == PHASE3_START + 60) {
        CreateHilbertSet(8, +1, 1, 620, 99, 0.15, 0.45/6, 130*4, 6, 0);
    }

    // ---- 自機狙い（牽制）----
    if (c < PHASE2_START) {
        // フェイス1：単発・ゆっくり
        if (c >= 120 && c % 30 == 5) CreateAimedSet(1, 2.5, 0.0, 0);
    }
    else if (c < PHASE3_START) {
        // フェイス2：3-way
        if (c % 30 == 5) CreateAimedSet(3, 2.6, 0.35, 0);
    }
    else {
        // フェイス3：3-way・高速・高頻度
        if (c % 20 == 5) CreateAimedSet(3, 3.0, 0.5, 0);
    }
}