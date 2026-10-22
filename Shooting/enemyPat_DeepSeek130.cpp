#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ピッチドロップ実験モチーフの弾幕
//  ・短レーザー2本で漏斗（ガラス）を形成
//  ・漏斗先端の中玉を核に、小弾が収束して溜まる
//  ・溜め完了で大玉に置換、超低速で落下
//  ・着地で跳ね、周囲に弾を撒く
//  敵本体の関数：void EnemyPat_PitchDrop_DeepSeek()
// ============================================================

// ---------- ヘルパー ----------
static sEnemyShot* AddShot(sEnemyShotSet* pSet,
    double x, double y,
    double muki, double speed,
    int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x; p->y = y;
    p->muki = muki; p->speed = speed;
    p->kind = kind;
    p->count = 0;
    p->margin = 20.0;
    for (int i = 0; i < 16; i++) { p->param_i[i] = 0; p->param_d[i] = 0.0; }
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

static void AddSet(sEnemyShotSet::PatternFunc func,
    double x, double y, double muki)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x; pSet->y = y; pSet->muki = muki;
    pSet->kind = 0;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// 弾を全移動（速度0の弾はそのまま）
static void MoveAll(sEnemyShotSet* pSet)
{
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  パターン1：漏斗（短レーザー2本でV字）
// ============================================================
static void ShotFunnel(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 左辺（＼）: 上左 → 先端 へ向かう向き
        AddShot(pSet,
            pSet->x - 30.0, pSet->y - 30.0,
            DX_PI * 0.25, 0.0,
            img_enemyShotLaser[6]); // 白
        // 右辺（／）: 上右 → 先端 へ向かう向き
        AddShot(pSet,
            pSet->x + 30.0, pSet->y - 30.0,
            DX_PI * 0.75, 0.0,
            img_enemyShotLaser[6]);
    }
    // 落下が終わったら漏斗を片付ける（画面外へ飛ばして自動消去に任せる）
    if (pSet->count == 330) {
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            p->x = -1000.0; p->y = -1000.0; p->speed = 0.0;
            p = p->next;
        }
    }
    MoveAll(pSet);
}

// ============================================================
//  パターン2：ピッチのしずく（溜め→落下→着地）
// ============================================================
static void ShotPitchDrop(sEnemyShotSet* pSet)
{
    const int    ACCUM_END = 100;   // 溜め完了フレーム
    const double FALL_SPEED = 1.2;   // 超低速
    const double LAND_Y = 460.0; // 着底位置

    // ---- 生成 ----
    if (pSet->count == 0) {
        // 溜まりの核（中玉・黒）
        sEnemyShot* p = AddShot(pSet, pSet->x, pSet->y,
            0.0, 0.0,
            img_enemyShotMediumBall[7]);
        p->param_i[0] = 1; // 核フラグ
    }
    // 収束する小弾を10Fごとに左右交互に放出
    if (pSet->count > 0 && pSet->count < ACCUM_END && pSet->count % 10 == 0) {
        int idx = pSet->count / 10;
        double side = (idx % 2 == 0) ? -1.0 : 1.0;
        double sx = pSet->x + side * 30.0;
        double sy = pSet->y - 50.0;
        double ang = atan2(pSet->y - sy, pSet->x - sx);
        sEnemyShot* p = AddShot(pSet, sx, sy, ang, 3.0,
            img_enemyShotSmallBall[7]); // 黒
        p->param_i[1] = 1; // 収束中フラグ
    }

    // 溜め完了 → 核と溜まりを消し、大玉に置き換える
    if (pSet->count == ACCUM_END) {
        sEnemyShot* q = pSet->pEnemyShotHead->next;
        while (q != pSet->pEnemyShotHead) {
            if (q->param_i[0] == 1 || q->param_i[1] >= 1) {
                q->x = -1000.0; q->y = -1000.0;
                q->speed = 0.0;
                q->param_i[0] = 0;
                q->param_i[1] = 0;
            }
            q = q->next;
        }
        sEnemyShot* big = AddShot(pSet, pSet->x, pSet->y,
            DX_PI / 2.0, FALL_SPEED,
            img_enemyShotLargeBall[7]); // 黒
        big->param_i[2] = 1; // 落下中フラグ

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // ---- 移動 ----
    MoveAll(pSet);

    // ---- 特殊判定 ----
    // イテレーション中に新規弾を追加するので、事前に末尾を保存して暴走を防ぐ
    sEnemyShot* stop = pSet->pEnemyShotHead->prev;
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* nextP = p->next;

        // 収束小弾が核に到達したら停止
        if (p->param_i[1] == 1) {
            double dx = pSet->x - p->x;
            double dy = pSet->y - p->y;
            if (dx * dx + dy * dy < 30.0) {
                p->x = pSet->x + (GetRand(6) - 3);
                p->y = pSet->y + (GetRand(6) - 3);
                p->speed = 0.0;
                p->param_i[1] = 2; // 停止済み
            }
        }

        // 大玉の着底 → 跳ね
        if (p->param_i[2] == 1 && p->y >= LAND_Y) {
            double cx = p->x;
            double cy = p->y;
            p->x = -1000.0; p->y = -1000.0;
            p->speed = 0.0;
            p->param_i[2] = 0;

            // 全方向へ小弾12個
            for (int i = 0; i < 12*5; i++) {
                double a = (DX_PI * 2.0 / 12.0/5) * i + DX_PI * 0.5;
                AddShot(pSet,
                    cx + cos(a) * 6.0, cy + sin(a) * 6.0,
                    a, 3.5,
                    img_enemyShotSmallBall[7]); // 黒
            }
            // 上方へ中弾3個
            for (int i = -1-5; i <= 1+5; i++) {
                double a = -DX_PI * 0.5 + i * 0.30;
                AddShot(pSet, cx, cy, a, 2.0,
                    img_enemyShotMediumBall[7]); // 黒
            }

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        if (p == stop) break;
        p = nextP;
    }
}

// ============================================================
//  パターン3：溜め中の妨害（自機狙い3-way）
// ============================================================
static void ShotAimed(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = -1; i <= 1; i++) {
            AddShot(pSet, pSet->x, pSet->y,
                pSet->muki + i * 0.18, 2.2,
                img_enemyShotSmallBall[0]); // 赤
        }
    }
    MoveAll(pSet);
}

// ============================================================
//  敵本体
// ============================================================
void EnemyPat_PitchDrop_DeepSeek()
{
    static int cycleTimer;
    static int cycleCount;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        cycleTimer = 0;
        cycleCount = 0;
    }

    // ボスはゆっくり揺れる（漏斗の真上あたり）
    enemy.x = 240.0 + 15.0 * sin(count * 0.015);

    // 漏斗の先端（このパターンでは固定。装置として扱う）
    const double FUNNEL_TIP_X = 240.0;
    const double FUNNEL_TIP_Y = 150.0;

    const int CYCLE_LEN = 320;
    const int ACCUM_END = 100;

    cycleTimer++;

    // ---- サイクル開始：漏斗としずくを起動 ----
    if (cycleTimer == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        AddSet(ShotFunnel, FUNNEL_TIP_X, FUNNEL_TIP_Y, 0.0);
        AddSet(ShotPitchDrop, FUNNEL_TIP_X, FUNNEL_TIP_Y, 0.0);
    }

    // ---- 溜め中は自機狙い3-wayで妨害 ----
    if (cycleTimer >= 1 && cycleTimer <= ACCUM_END) {
        if (cycleTimer % 10 == 0) {
            double ang = atan2(player.y - enemy.y, player.x - enemy.x);
            AddSet(ShotAimed, enemy.x + GetRand(480) - 240, enemy.y + 10.0, ang);
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // ---- サイクル終了 ----
    if (cycleTimer >= CYCLE_LEN) {
        cycleTimer = 0;
        cycleCount++;
    }
}