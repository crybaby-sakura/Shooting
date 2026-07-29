// enemyPat_Tmp.cpp
// 正十二面体モチーフ弾幕「十二面体結界」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {

    constexpr double PI = DX_PI;

    // ------------------------------------------------------------
    // 辺を小玉で描画
    // ------------------------------------------------------------
    static void AddEdgeBalls(
        sEnemyShotSet* set,
        double x1, double y1,
        double x2, double y2,
        int color,
        int div)
    {
        for (int i = 0; i <= div; ++i) {
            double t = (double)i / (double)div;

            sEnemyShot* s = new sEnemyShot;
            s->x = x1 + (x2 - x1) * t;
            s->y = y1 + (y2 - y1) * t;

            double dx = x2 - x1;
            double dy = y2 - y1;
            double len = sqrt(dx * dx + dy * dy);

            s->muki = atan2(dy, dx);
            s->speed = 0.18;

            // 辺は小玉
            s->kind = img_enemyShotSmallBall[color % COL_VAR];
            s->margin = 200;

            // 辺上を往復するための情報
            s->param_d[0] = x1;
            s->param_d[1] = y1;
            s->param_d[2] = dx;
            s->param_d[3] = dy;
            s->param_d[4] = len;
            s->param_d[5] = t * len;

            s->prev = set->pEnemyShotHead->prev;
            s->next = set->pEnemyShotHead;
            set->pEnemyShotHead->prev->next = s;
            set->pEnemyShotHead->prev = s;
        }
    }

    // ------------------------------------------------------------
    // 頂点を中玉で配置
    // ------------------------------------------------------------
    static void AddVertex(
        sEnemyShotSet* set,
        double x, double y,
        int color)
    {
        sEnemyShot* s = new sEnemyShot;
        s->x = x;
        s->y = y;
        s->muki = 0.0;
        s->speed = 0.0;

        // 頂点は中玉
        s->kind = img_enemyShotMediumBall[color % COL_VAR];
        s->margin = 200;

        s->prev = set->pEnemyShotHead->prev;
        s->next = set->pEnemyShotHead;
        set->pEnemyShotHead->prev->next = s;
        set->pEnemyShotHead->prev = s;
    }

    // ------------------------------------------------------------
    // 正十二面体風ワイヤーフレーム生成
    // ------------------------------------------------------------
    static void BuildDodecahedron(sEnemyShotSet* set)
    {
        const double cx = set->x;
        const double cy = set->y;

        // 前後の五角形リング
        double px[3][5];
        double py[3][5];

        const double radius[3] = { 28.0 * 5, 46.0 * 5, 28.0 * 5 };
        const double offsetY[3] = { -22.0 * 5, 0.0 * 5, 22.0 * 5 };
        const double rot[3] = { 0.0, PI / 5.0, PI / 10.0 };

        for (int k = 0; k < 3; ++k) {
            for (int i = 0; i < 5; ++i) {
                double ang = rot[k] + 2.0 * PI * i / 5.0;
                px[k][i] = cx + radius[k] * cos(ang);
                py[k][i] = cy + offsetY[k] + radius[k] * sin(ang) * 0.55;
            }
        }

        // 頂点配置
        for (int k = 0; k < 3; ++k) {
            for (int i = 0; i < 5; ++i) {
                AddVertex(set, px[k][i], py[k][i], 6); // 白
            }
        }

        // 各五角形の辺
        for (int k = 0; k < 3; ++k) {
            for (int i = 0; i < 5; ++i) {
                int j = (i + 1) % 5;
                AddEdgeBalls(
                    set,
                    px[k][i], py[k][i],
                    px[k][j], py[k][j],
                    4, // 青
                    7);
            }
        }

        // 前面リング → 中央リング
        for (int i = 0; i < 5; ++i) {
            AddEdgeBalls(
                set,
                px[0][i], py[0][i],
                px[1][i], py[1][i],
                3, // シアン
                6);

            AddEdgeBalls(
                set,
                px[0][i], py[0][i],
                px[1][(i + 4) % 5], py[1][(i + 4) % 5],
                3,
                6);
        }

        // 中央リング → 背面リング
        for (int i = 0; i < 5; ++i) {
            AddEdgeBalls(
                set,
                px[1][i], py[1][i],
                px[2][i], py[2][i],
                2, // 緑
                6);

            AddEdgeBalls(
                set,
                px[1][i], py[1][i],
                px[2][(i + 1) % 5], py[2][(i + 1) % 5],
                2,
                6);
        }
    }

    // ------------------------------------------------------------
    // 弾セット更新
    // ------------------------------------------------------------
    static void ShotDodecahedron(sEnemyShotSet* set)
    {
        // 初回生成
        if (set->count == 0) {
            if (CheckSoundMem(sound_enemyCharge))
                StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            BuildDodecahedron(set);

            set->param_d[0] = 0.0;
            set->param_d[1] = 0.0;
        }

        set->param_d[1] += 1.0;

        // 辺に沿って弾を流す
        sEnemyShot* s = set->pEnemyShotHead->next;
        while (s != set->pEnemyShotHead) {

            // speed == 0 の弾は頂点
            if (s->speed > 0.0 && s->param_d[4] > 0.0) {
                double len = s->param_d[4];

                s->param_d[5] += s->speed;

                // 往復運動
                double p = fmod(s->param_d[5], len * 2.0);
                if (p > len) p = len * 2.0 - p;

                double t = p / len;

                s->x = s->param_d[0] + s->param_d[2] * t + set->param_d[0];
                s->y = s->param_d[1] + s->param_d[3] * t + set->param_d[1];
            }
            else {
                s->y += 1.0;
            }

            s = s->next;
        }
    }

} // namespace

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_Dodecahedron_ChatGPT()
{
    static int dir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 72.0;
        enemy.maxHp = enemy.hp = 200;
        dir = 1;
    }
    else {
        // ゆっくり左右移動
        enemy.x += 0.55 * dir;

        if (enemy.x < 120.0) dir = 1;
        if (enemy.x > 360.0) dir = -1;
    }

    // 240フレームごとに新しい十二面体結界を展開
    if (count % 240 == 1) {
        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = ShotDodecahedron;

        set->x = enemy.x;
        set->y = enemy.y + 26.0;

        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }
}