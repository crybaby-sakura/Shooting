// enemyPat_sanshokuKushimai.cpp
//
// 三色串舞（さんしょくくしまい）
// 花見団子（緑・白・桃）をモチーフにした4フェーズ弾幕
//
//   Phase1 串出現   : 中心から放射状に5本の串が伸び、串上に内側から
//                     緑(よもぎ)→白(雪)→桃(桜／マゼンタで代用)の順で
//                     団子が結実する
//   Phase2 串ぐるぐる: 完成した車輪状の串が回転しながら、団子の色ごとに
//                     異なる接線弾を撒く（緑=反時計回り／桃=時計回り／
//                     白=直進）
//   Phase3 串抜き   : 串本体が外側へ弾け飛んで画面外へ消え、残った団子が
//                     色ごとに異なるバーストで弾ける
//                     （桃=3wayプレイヤー狙い／白=安置つき全方位リング／
//                       緑=接線方向を中心とした扇バースト）
//   Phase4 串刺し乱舞: 画面上から短い串が次々と降り注ぐ。間隔が徐々に
//                     短くなり加速しながら、通過時に団子が単発の狙い弾を
//                     放つ。次のサイクルへループする。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {

    // ---- 色（0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙） ----
    constexpr int COL_GREEN = 2; // よもぎ
    constexpr int COL_WHITE = 6; // 雪
    constexpr int COL_PINK = 5; // 桜（マゼンタで代用）
    constexpr int COL_STICK = 8; // 串（橙）

    // ---- サイクル構成（フレーム、1サイクル=780F=13秒@60fps） ----
    constexpr int T1 = 90;   // Phase1終了：串出現完了
    constexpr int T2 = 330;  // Phase2終了：回転完了→串抜きトリガー
    constexpr int T3 = 390;  // Phase3終了→Phase4開始：串刺し乱舞
    constexpr int CYCLE = 780;

    // ---- 車輪の幾何 ----
    constexpr int    NUM_KUSHI = 5;
    constexpr double CENTER_X = 240.0;
    constexpr double CENTER_Y = 170.0;
    constexpr double R_GREEN = 55.0;
    constexpr double R_WHITE = 95.0;
    constexpr double R_PINK = 135.0;
    constexpr double R_TIP = 150.0;
    constexpr double ROT_SPEED = DX_PI / 120.0; // Phase2の240Fでちょうど1回転

    // ---- 速度・間隔パラメータ ----
    constexpr double FLYOUT_ACCEL = 0.15; // 串抜き時の加速度
    constexpr int    SPOKE_FIRE_INTERVAL = 20; // Phase2中、団子が接線弾を撒く間隔
    constexpr double SPOKE_SHOT_SPEED = 1.6;
    constexpr double BURST_SHOT_SPEED = 2.2;

    constexpr int    RAIN_BASE_INTERVAL = 35; // Phase4開始時の串の降下間隔
    constexpr int    RAIN_MIN_INTERVAL = 3; // Phase4終盤の最短間隔
    constexpr double RAIN_FALL_SPEED = 3.0;
    constexpr double RAIN_TRIGGER_Y = 260.0; // このYを超えたら団子が発砲

    // ---- 弾の役割（param_i[0]） ----
    constexpr int ROLE_STICK = 0; // 車輪の串本体
    constexpr int ROLE_DANGO_GREEN = 1;
    constexpr int ROLE_DANGO_WHITE = 2;
    constexpr int ROLE_DANGO_PINK = 3;
    constexpr int ROLE_SPOKE_SHOT = 4; // Phase2で団子から撒かれる接線弾
    constexpr int ROLE_BURST = 5; // Phase3/4のバースト弾（直進）
    constexpr int ROLE_RAIN_STICK = 6; // Phase4の串本体
    constexpr int ROLE_RAIN_DANGO = 7; // Phase4の団子

    // ============================================================
    //  共通ヘルパー
    // ============================================================

    // 弾を1個生成し、リストの末尾（head の直前）に連結する
    static sEnemyShot* SpawnShot(sEnemyShotSet* pSet, int kind, double muki)
    {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->kind = kind;
        pShot->muki = muki;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;

        return pShot;
    }

    // 団子1個ぶんのPhase3バーストを生成する（色ごとに性質が異なる）
    static void SpawnDangoBurst(sEnemyShotSet* pSet, int role, double x, double y, double tangentAngle)
    {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        if (role == ROLE_DANGO_PINK) {
            // 桃：プレイヤー狙いの3way
            double aim = atan2(player.y - y, player.x - x);
            double spread[3] = { -0.16, 0.0, 0.16 };
            for (int i = 0; i < 3; i++) {
                double a = aim + spread[i];
                sEnemyShot* p = SpawnShot(pSet, img_enemyShotMediumOval[COL_PINK], a);
                p->param_i[0] = ROLE_BURST;
                p->param_d[0] = x;
                p->param_d[1] = y;
                p->param_d[2] = a;
            }
        }
        else if (role == ROLE_DANGO_WHITE) {
            // 白：全方位リング（1方向だけ隙間を開けて安置にする）
            constexpr int N = 16;
            for (int i = 0; i < N; i++) {
                if (i == 0) continue; // 安置用の隙間
                double a = 2.0 * DX_PI * i / N;
                sEnemyShot* p = SpawnShot(pSet, img_enemyShotSmallBall[COL_WHITE], a);
                p->param_i[0] = ROLE_BURST;
                p->param_d[0] = x;
                p->param_d[1] = y;
                p->param_d[2] = a;
            }
        }
        else if (role == ROLE_DANGO_GREEN) {
            // 緑：接線方向を中心とした扇（クレセント）バースト、約140度
            constexpr int N = 9;
            double center = tangentAngle + DX_PI / 2.0;
            double fanWidth = 140.0 / 180.0 * DX_PI;
            for (int i = 0; i < N; i++) {
                double a = center + (i - (N - 1) / 2.0) * (fanWidth / (N - 1));
                sEnemyShot* p = SpawnShot(pSet, img_enemyShotDiamond[COL_GREEN], a);
                p->param_i[0] = ROLE_BURST;
                p->param_d[0] = x;
                p->param_d[1] = y;
                p->param_d[2] = a;
            }
        }
    }

    // ============================================================
    //  Phase1〜3：車輪本体（串+団子）のパターン
    // ============================================================
    static void ShotKushiWheel(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            for (int k = 0; k < NUM_KUSHI; k++) {
                double baseAngle = -DX_PI / 2.0 + k * (2.0 * DX_PI / NUM_KUSHI);

                // 串本体：中心から外へ橙の棒状弾を並べる
                for (double r = 15.0; r <= R_TIP; r += 15.0) {
                    sEnemyShot* p = SpawnShot(pSet, img_enemyShotBullet[COL_STICK], baseAngle);
                    p->param_i[0] = ROLE_STICK;
                    p->param_d[0] = baseAngle;
                    p->param_d[1] = r;
                }

                // 団子3色：内側から緑→白→桃
                struct DangoDef { int role; double r; int kind; };
                DangoDef list[3] = {
                    { ROLE_DANGO_GREEN, R_GREEN, img_enemyShotMediumBall[COL_GREEN] },
                    { ROLE_DANGO_WHITE, R_WHITE, img_enemyShotMediumBall[COL_WHITE] },
                    { ROLE_DANGO_PINK,  R_PINK,  img_enemyShotMediumBall[COL_PINK]  },
                };
                for (int d = 0; d < 3; d++) {
                    sEnemyShot* p = SpawnShot(pSet, list[d].kind, baseAngle);
                    p->param_i[0] = list[d].role;
                    p->param_d[0] = baseAngle;
                    p->param_d[1] = list[d].r;
                }
            }
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            int t = pShot->count;
            int role = pShot->param_i[0];

            if (role == ROLE_STICK || role == ROLE_DANGO_GREEN ||
                role == ROLE_DANGO_WHITE || role == ROLE_DANGO_PINK) {

                double baseAngle = pShot->param_d[0];
                double targetR = pShot->param_d[1];
                double angle, radius;

                if (t < T1) {
                    // Phase1：中心から伸びていく（回転なし）
                    radius = targetR * (double)t / T1;
                    angle = baseAngle;
                }
                else if (t < T2) {
                    // Phase2：全長に達し、車輪ごと回転
                    radius = targetR;
                    angle = baseAngle + ROT_SPEED * (t - T1);
                }
                else {
                    // Phase3：串抜き。回転を保ったまま外側へ加速して画面外へ
                    angle = baseAngle + ROT_SPEED * (t - T1);
                    double dt = (double)(t - T2);
                    radius = targetR + FLYOUT_ACCEL * dt * dt;
                }

                pShot->x = CENTER_X + radius * cos(angle);
                pShot->y = CENTER_Y + radius * sin(angle);
                pShot->muki = angle;

                bool isDango = (role != ROLE_STICK);
                if (isDango) {
                    // Phase2中：一定間隔で接線／放射弾を撒く
                    if (t >= T1 && t < T2 && (t - T1) % SPOKE_FIRE_INTERVAL == 0) {
                        double fireMuki;
                        int col;
                        if (role == ROLE_DANGO_GREEN) { fireMuki = angle + DX_PI / 2.0; col = COL_GREEN; }
                        else if (role == ROLE_DANGO_PINK) { fireMuki = angle - DX_PI / 2.0; col = COL_PINK; }
                        else { fireMuki = angle;              col = COL_WHITE; }

                        sEnemyShot* pNew = SpawnShot(pSet, img_enemyShotScale[col], fireMuki);
                        pNew->param_i[0] = ROLE_SPOKE_SHOT;
                        pNew->param_d[0] = pShot->x;
                        pNew->param_d[1] = pShot->y;
                        pNew->param_d[2] = fireMuki;
                    }

                    // Phase2→3の切り替わりの瞬間に色別バーストを1回だけ発生させる
                    if (t == T2) {
                        SpawnDangoBurst(pSet, role, pShot->x, pShot->y, angle);
                    }
                }
            }
            else if (role == ROLE_SPOKE_SHOT || role == ROLE_BURST) {
                double spawnX = pShot->param_d[0];
                double spawnY = pShot->param_d[1];
                double muki = pShot->param_d[2];
                double speed = (role == ROLE_SPOKE_SHOT) ? SPOKE_SHOT_SPEED : BURST_SHOT_SPEED;

                pShot->x = spawnX + speed * (double)t * cos(muki);
                pShot->y = spawnY + speed * (double)t * sin(muki);
                pShot->muki = muki;
            }

            pShot = pShot->next;
        }
    }

    // ============================================================
    //  Phase4：串刺し乱舞（降り注ぐミニ串）
    // ============================================================
    static void ShotKushiRain(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            double spawnX = pSet->x;
            double spawnY = pSet->y;

            // 串本体：短い橙の棒4本
            for (int i = 0; i < 4; i++) {
                sEnemyShot* p = SpawnShot(pSet, img_enemyShotBullet[COL_STICK], DX_PI / 2.0);
                p->param_i[0] = ROLE_RAIN_STICK;
                p->param_d[0] = spawnX;
                p->param_d[1] = spawnY + i * 14.0;
            }

            // 団子3色：上から緑→白→桃（桃が先頭＝下側）
            struct DangoDef { int col; int kind; double offsetY; };
            DangoDef list[3] = {
                { COL_GREEN, img_enemyShotMediumBall[COL_GREEN], 0.0  },
                { COL_WHITE, img_enemyShotMediumBall[COL_WHITE], 20.0 },
                { COL_PINK,  img_enemyShotMediumBall[COL_PINK],  40.0 },
            };
            for (int d = 0; d < 3; d++) {
                sEnemyShot* p = SpawnShot(pSet, list[d].kind, DX_PI / 2.0);
                p->param_i[0] = ROLE_RAIN_DANGO;
                p->param_i[1] = list[d].col;
                p->param_i[2] = 0; // 発砲済みフラグ
                p->param_d[0] = spawnX;
                p->param_d[1] = spawnY + list[d].offsetY;
            }
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            int t = pShot->count;
            int role = pShot->param_i[0];

            if (role == ROLE_RAIN_STICK || role == ROLE_RAIN_DANGO) {
                double baseX = pShot->param_d[0];
                double baseY = pShot->param_d[1];
                double y = baseY + RAIN_FALL_SPEED * (double)t;
                double x = baseX + 8.0 * sin((double)t * 0.05); // 落下中の軽いゆれ

                pShot->x = x;
                pShot->y = y;

                if (role == ROLE_RAIN_DANGO && pShot->param_i[2] == 0 && y >= RAIN_TRIGGER_Y) {
                    pShot->param_i[2] = 1; // 二重発砲を防ぐ

                    int col = pShot->param_i[1];
                    int kind = (col == COL_GREEN) ? img_enemyShotDiamond[COL_GREEN]
                        : (col == COL_PINK) ? img_enemyShotMediumOval[COL_PINK]
                        : img_enemyShotSmallBall[COL_WHITE];
                    double aim = atan2(player.y - y, player.x - x);

                    sEnemyShot* pB = SpawnShot(pSet, kind, aim);
                    pB->param_i[0] = ROLE_BURST;
                    pB->param_d[0] = x;
                    pB->param_d[1] = y;
                    pB->param_d[2] = aim;
                }
            }
            else if (role == ROLE_BURST) {
                double spawnX = pShot->param_d[0];
                double spawnY = pShot->param_d[1];
                double muki = pShot->param_d[2];

                pShot->x = spawnX + BURST_SHOT_SPEED * (double)t * cos(muki);
                pShot->y = spawnY + BURST_SHOT_SPEED * (double)t * sin(muki);
                pShot->muki = muki;
            }

            pShot = pShot->next;
        }
    }

    // ============================================================
    //  ショットセット生成ヘルパー
    // ============================================================
    static void SpawnKushiWheel()
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotKushiWheel;
        pSet->x = CENTER_X;
        pSet->y = CENTER_Y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    static void SpawnRainKushi()
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotKushiRain;
        pSet->x = 40.0 + GetRand(400); // 画面内でランダムなX（480x480画面、余白40）
        pSet->y = -20.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

} // namespace

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_TricolorDango_Claude()
{
    static int nextRainAt = 0;

    int t = (count - 1) % CYCLE;

    if (count == 1) {
        enemy.maxHp = enemy.hp = 200;
    }

    // 中心に留まりつつ、軽く上下にゆれる（演出のみ、団子の座標計算には影響しない）
    enemy.x = CENTER_X;
    enemy.y = (CENTER_Y - 40.0) + 6.0 * sin((double)count * 0.02);

    if (t == 0) {
        // サイクル開始：車輪（串+団子）を生成
        SpawnKushiWheel();
        nextRainAt = T3;
    }

    // Phase4：串刺し乱舞。間隔を徐々に短くしながら降らせる
    if (t >= T3 && t < CYCLE && t == nextRainAt) {
        SpawnRainKushi();

        int elapsed = t - T3;
        int span = CYCLE - T3;
        int interval = RAIN_BASE_INTERVAL - (RAIN_BASE_INTERVAL - RAIN_MIN_INTERVAL) * elapsed / span;
        if (interval < RAIN_MIN_INTERVAL) interval = RAIN_MIN_INTERVAL;

        nextRainAt = t + interval;
    }
}