#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// スピログラフ弾幕
// ・小玉    : 内側の歯車の歯
// ・中玉    : 外側の歯車の歯
// ・菱形弾  : スピログラフの描線
// count / pEnemyShotSet->count / pEnemyShot->count の更新、
// 画面外弾の削除はメインルーチン側で行う。

static const double PI2 = DX_PI * 2.0;
static const int INNER_TEETH = 12 * 3;
static const int OUTER_TEETH = 20 * 3;
static const int TRACE_COUNT = 24 * 10;

static void AddShot(sEnemyShotSet* set, double x, double y, int kind, int role, double a, double b, double c)
{
    sEnemyShot* shot = new sEnemyShot;
    shot->x = x;
    shot->y = y;
    shot->muki = 0.0;
    shot->speed = 0.0;
    shot->kind = kind;
    shot->param_i[0] = role;
    shot->param_i[1] = 0;
    shot->param_i[2] = 0;
    shot->param_d[0] = a;
    shot->param_d[1] = b;
    shot->param_d[2] = c;

    shot->prev = set->pEnemyShotHead->prev;
    shot->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = shot;
    set->pEnemyShotHead->prev = shot;
}

static void ShotSpirograph(sEnemyShotSet* set)
{
    const double cx = set->x;
    const double cy = set->y;
    const double t = set->count * 0.035 * 5;

    // 内歯車: 小玉。少しだけ半径を揺らして「歯」を強調。
    // role 0
    // a = 初期位相, b = 歯の角速度補正, c = 歯数由来の局所位相
    // 外歯車: 中玉。内歯車と逆方向に回す。
    // role 1
    // 軌跡: 菱形弾。外歯車に対して内歯車が転がる形でスピログラフを描く。
    // role 2
    // role 3 は中心軸付近の補助円。

    sEnemyShot* shot = set->pEnemyShotHead->next;
    while (shot != set->pEnemyShotHead) {
        const int role = shot->param_i[0];
        const double phase = shot->param_d[0];
        const double r = shot->param_d[1] * 1.5;
        const double tooth = shot->param_d[2];

        if (role == 0) {
            const double a = phase + t * 0.75;
            const double toothWave = 5.0 * (0.5 + 0.5 * cos(tooth * a));
            const double rr = r + toothWave;
            shot->x = cx + rr * cos(a);
            shot->y = cy + rr * sin(a);
            shot->muki = a + DX_PI * 0.5;
        }
        else if (role == 1) {
            const double a = phase - t * 0.46;
            const double toothWave = 5.5 * (0.5 + 0.5 * cos(tooth * a));
            const double rr = r + toothWave;
            shot->x = cx + rr * cos(a);
            shot->y = cy + rr * sin(a);
            shot->muki = a + DX_PI * 0.5;
        }
        else if (role == 2) {
            // 外歯車と内歯車の比率をそのまま軌跡に反映。
            // R=145, r=55 として、内転円によるハイポトロコイド風の線を描く。
            const double u = t * 0.62 + phase;
            const double R = 145.0 * 1.5;
            const double rr = 55.0 * 1.5;
            const double k = (R - rr) / rr;
            const double q = 31.0;
            const double orbit = R - rr;
            const double x = orbit * cos(u) + rr * 0.78 * cos((1.0 + k) * u);
            const double y = orbit * sin(u) - rr * 0.78 * sin((1.0 + k) * u);
            // 複数本の位相違いを重ねることで一本の光線ではなく輪郭を作る。
            const double wobble = 2.5 * sin(q * u + phase);
            const double len = sqrt(x * x + y * y);
            const double nx = (len > 0.001) ? x / len : 1.0;
            const double ny = (len > 0.001) ? y / len : 0.0;
            shot->x = cx + x + nx * wobble;
            shot->y = cy + y + ny * wobble;
            shot->muki = atan2(y, x);
        }
        else {
            const double a = phase + t * 0.22;
            const double rr = r + 3.0 * sin(shot->param_d[2] * a);
            shot->x = cx + rr * cos(a);
            shot->y = cy + rr * sin(a);
            shot->muki = a;
        }

        shot = shot->next;
    }
}

void EnemyPat_Spirograph_ChatGPT()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 中央固定。ごく小さく左右に揺らして、歯車全体に生命感を出す。
    enemy.x = 240.0 + 38.0 * sin(count * 0.009);
    enemy.y = 215.0 + 16.0 * cos(count * 0.013);

    if (count == 1) {
        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = ShotSpirograph;
        set->x = enemy.x;
        set->y = enemy.y;
        set->muki = 0.0;
        set->kind = 0;

        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        // 内側の歯車。小玉で歯を表現。
        for (int i = 0; i < INNER_TEETH; i++) {
            const double a = PI2 * i / INNER_TEETH;
            AddShot(set,
                set->x + 78.0 * cos(a),
                set->y + 78.0 * sin(a),
                img_enemyShotSmallBall[3],
                0, a, 72.0, (double)INNER_TEETH);
        }

        // 外側の歯車。中玉で歯を表現。
        for (int i = 0; i < OUTER_TEETH; i++) {
            const double a = PI2 * i / OUTER_TEETH + DX_PI / OUTER_TEETH;
            AddShot(set,
                set->x + 150.0 * cos(a),
                set->y + 150.0 * sin(a),
                img_enemyShotMediumBall[4],
                1, a, 144.0, (double)OUTER_TEETH);
        }

        // スピログラフの描線。菱形弾を位相違いで重ねる。
        for (int i = 0; i < TRACE_COUNT; i++) {
            const double a = PI2 * i / TRACE_COUNT;
            AddShot(set,
                set->x,
                set->y,
                img_enemyShotDiamond[5],
                2, a, 0.0, 0.0);
        }

        // 中央寄りの補助輪。内外歯車の噛み合わせを視覚的に強調。
        for (int i = 0; i < 8; i++) {
            const double a = PI2 * i / 8.0 + DX_PI / 8.0;
            AddShot(set,
                set->x + 43.0 * cos(a),
                set->y + 43.0 * sin(a),
                img_enemyShotSmallBall[6],
                3, a, 43.0, 4.0);
        }

        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }

}