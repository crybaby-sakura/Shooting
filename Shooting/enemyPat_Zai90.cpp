// enemyPat_brownian.cpp
//
// 弾幕パターン：ブラウン・ダンス 〜花粉の踊り〜
// [コンセプト]
//   画面を顕微鏡のプレパラート(閉じた箱)に見立てる。
//     大玉(黄)   = 花粉(観察対象の微粒子) ×3
//     小玉(水色) = 水分子(媒質)
//   水分子が花粉に衝突するたびに花粉が入射方向へ僅かに押され、
//   減衰と速度上限の中で、花粉は分子の衝突だけでランダムウォークする。
//
// [サイクル構成: 1200フレーム(20秒)で一周]
//      0〜 300  低温(分子30個) 花粉はゆっくり漂う
//    300〜 600  中温(分子45個)
//    600〜 900  高温(分子60個)
//    900〜1080  沸騰(分子75個) 花粉が激しく飛び回る
//   1080        蒸発: 花粉は白い分子を弾いて画面上へ飛び去り、
//               箱の蓋が開いて全分子が画面外へ排気される(小休止)
//   1200〜      敵が新しい花粉を吐き出して再び加熱
//
// [仕様上の注意]
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//     画面外に出た弾の消去はメインルーチンが行うため、このファイル内では
//     カウントの加算も弾の delete も一切行わない。
//     (花粉の消去も「画面外へ飛ばしてメインルーチンに消してもらう」方式)
//   ・弾セットは敵出現時に1つだけ生成する。大玉も小玉も同じリストに入れ、
//     役割は param_i[0] で区別する。相互衝突を単一リスト内で完結させるため。
//   ・リストが空になるとセットを消される可能性があるため、
//     どのフレームでも必ず弾が1発以上残り続ける構成にしている。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ================= チューニング用定数 =================
static const double BALL_R = 22.0;  // 花粉(大玉)の反射判定半径
static const double MOL_R = 2.5;   // 分子(小玉)の半径
static const double DECAY = 0.985; // 花粉の速度減衰(媒質の抵抗)
static const double VMAX = 2.4;   // 花粉の速度上限(回避可能な速さに抑える)
static const double IMPULSE = 0.30;  // 分子1ヒットが花粉に与える衝撃係数
static const int    CYCLE = 1200;  // 1サイクルの長さ(フレーム)
static const int    EVAP_F = 1080;  // サイクル内での蒸発(蓋を開ける)タイミング

// 花粉が漂っていく目標位置(スロット)
static const double BIG_X[3] = { 240.0, 140.0, 340.0 };
static const double BIG_Y[3] = { 180.0, 250.0, 250.0 };

// ================= 生成ヘルパー =================

// リスト末尾に弾を1つ追加(役割は呼び出し側が param_i[0] に設定する)
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int img)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = img;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 分子(小玉)を生成
static sEnemyShot* SpawnMolecule(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int color)
{
    sEnemyShot* p = AddShot(pSet, x, y, muki, speed, img_enemyShotSmallBall[color]);
    p->param_i[0] = 1; // 1 = 分子(小玉)
    return p;
}

// ================= 弾幕本体 =================
// param の使い方:
//   param_i[0] : 0 = 花粉(大玉) / 1 = 分子(小玉)
//   param_i[1] : 花粉のみ。0 = 通常 / 1 = 蒸発中
//   param_d[0],[1] : 花粉のみ。速度 (vx, vy)
static void ShotBrownian(sEnemyShotSet* pSet)
{
    int f = pSet->count % CYCLE;

    // ---- 温度(フェーズ)の決定 ----
    // 分子の数と速さがそのまま「温度」。花粉の動きの激しさはここで決まる。
    int    targetN;
    double molSpeed;
    if (f < 300) { targetN = 30; molSpeed = 2.6; }
    else if (f < 600) { targetN = 45; molSpeed = 3.0; }
    else if (f < 900) { targetN = 60; molSpeed = 3.3; }
    else if (f < EVAP_F) { targetN = 75; molSpeed = 3.6; }
    else { targetN = 0;  molSpeed = 2.5; } // 排気中は補充しない

    // 蓋が開いている間は分子は壁で跳ね返らず、画面外へ出ていく
    bool boxOpen = (f >= EVAP_F);

    // ---- 事前スキャン(花粉の収集と分子の数え上げ) ----
    sEnemyShot* big[8];
    int nBig = 0;  // 花粉の数(蒸発中のものも含む)
    int nLive = 0;  // 蒸発していない花粉の数
    int nMol = 0;  // 分子の数

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 0) {
            if (nBig < 8) big[nBig++] = p;
            if (p->param_i[1] == 0) nLive++;
        }
        else {
            // サイクル先頭で残留分子の速度を初期温度へ揃える(温度リセット)
            if (f == 0) p->speed = molSpeed;
            nMol++;
        }
        p = p->next;
    }

    // ---- 花粉の出現(敵が3粒、間隔を置いて吐き出す) ----
    if (f % 18 == 0 && f < 54 && nLive < 3) {
        int slot = nLive;
        sEnemyShot* b = AddShot(pSet, enemy.x, enemy.y + 12.0, 0.0, 0.0, img_enemyShotLargeBall[1]); // 黄 = 花粉
        b->param_i[0] = 0;
        b->param_i[1] = 0;
        // 目標スロットへ向かってゆっくり漂う初速。その後の動きは分子の衝突任せ。
        b->param_d[0] = (BIG_X[slot] + GetRand(60) - 30 - enemy.x) / 140.0;
        b->param_d[1] = (BIG_Y[slot] + GetRand(40) - 20 - (enemy.y + 12.0)) / 140.0;
        if (nBig < 8) big[nBig++] = b;

        if (f == 0) {
            // 新サイクル開始の予告音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }

    // ---- 温度上昇の演出音 ----
    if (f == 300 || f == 600 || f == 900) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // ---- 蒸発イベント(沸騰の頂点で全てを解放する) ----
    if (f == EVAP_F) {
        for (int i = 0; i < nBig; i++) {
            sEnemyShot* b = big[i];
            if (b->param_i[1] != 0) continue;
            b->param_i[1] = 1;                     // 蒸発中: 減衰・壁反射・衝撃を打ち切り上へ飛び去る
            b->param_d[0] = (b->x - 240.0) * 0.015;
            b->param_d[1] = -7.0;
            for (int k = 0; k < 10; k++) {         // 白い蒸気を弾ける
                double ang = k * (DX_PI * 2.0 / 10.0) + 0.3;
                SpawnMolecule(pSet, b->x + cos(ang) * 26.0, b->y + sin(ang) * 26.0, ang, 3.8, 6);
            }
        }
        // 残っていた分子を一斉に加速して画面外へ排気(消去はメインルーチンが行う)
        sEnemyShot* q = pSet->pEnemyShotHead->next;
        while (q != pSet->pEnemyShotHead) {
            if (q->param_i[0] == 1) {
                q->speed *= 1.8;
                if (q->speed > 6.0) q->speed = 6.0;
            }
            q = q->next;
        }
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // ---- 分子の補充(熱源である敵から噴き出す) ----
    if (!boxOpen) {
        if (nMol < targetN) {
            // 真下を中心に ±80度の扇ランダム
            SpawnMolecule(pSet, enemy.x, enemy.y + 12.0,
                DX_PI / 2.0 + (GetRand(160) - 80) / 180.0 * DX_PI, molSpeed, 3); // 水色 = 水分子
        }
    }
    else if (f % 20 == 0) {
        // 排気中は間欠的に漏れる程度。弾リストが完全に空にならないための保険も兼ねる。
        SpawnMolecule(pSet, enemy.x, enemy.y + 12.0,
            DX_PI / 2.0 + (GetRand(80) - 40) / 180.0 * DX_PI, molSpeed, 3);
    }

    // ---- 移動 ----
    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 0) {
            if (p->param_i[1] == 0) {
                // 花粉: 減衰 → 速度上限 → 移動 → 器の壁で反射
                double vx = p->param_d[0] * DECAY;
                double vy = p->param_d[1] * DECAY;
                double sp = sqrt(vx * vx + vy * vy);
                if (sp > VMAX) {
                    vx *= VMAX / sp;
                    vy *= VMAX / sp;
                }
                p->param_d[0] = vx;
                p->param_d[1] = vy;
                p->x += vx;
                p->y += vy;

                if (p->x < BALL_R) { p->x = 2.0 * BALL_R - p->x; p->param_d[0] = -p->param_d[0]; }
                else if (p->x > 480.0 - BALL_R) { p->x = 2.0 * (480.0 - BALL_R) - p->x; p->param_d[0] = -p->param_d[0]; }
                if (p->y < BALL_R) { p->y = 2.0 * BALL_R - p->y; p->param_d[1] = -p->param_d[1]; }
                else if (p->y > 480.0 - BALL_R) { p->y = 2.0 * (480.0 - BALL_R) - p->y; p->param_d[1] = -p->param_d[1]; }
            }
            else {
                // 蒸発中の花粉: まっすぐ上へ(画面外でメインルーチンが消去)
                p->x += p->param_d[0];
                p->y += p->param_d[1];
            }
        }
        else {
            // 分子: 等速直線運動。蓋が閉じている間は器の壁で反射し畳み続ける。
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
            if (!boxOpen) {
                if (p->x < MOL_R) { p->x = 2.0 * MOL_R - p->x; p->muki = DX_PI - p->muki; }
                else if (p->x > 480.0 - MOL_R) { p->x = 2.0 * (480.0 - MOL_R) - p->x; p->muki = DX_PI - p->muki; }
                if (p->y < MOL_R) { p->y = 2.0 * MOL_R - p->y; p->muki = -p->muki; }
                else if (p->y > 480.0 - MOL_R) { p->y = 2.0 * (480.0 - MOL_R) - p->y; p->muki = -p->muki; }
            }
        }
        p = p->next;
    }

    // ---- 分子 × 花粉 の衝突(ブラウン運動の核心) ----
    // 分子は花粉の表面で鏡面反射し、花粉は分子の入射方向へ僅かに押される。
    // 分子の飛来はランダムなので、花粉の軌道は自然とジグザグになる。
    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 1) {
            for (int i = 0; i < nBig; i++) {
                sEnemyShot* b = big[i];
                double dx = p->x - b->x;
                double dy = p->y - b->y;
                double rr = BALL_R + MOL_R;
                if (dx * dx + dy * dy < rr * rr) {
                    double d = sqrt(dx * dx + dy * dy);
                    if (d < 0.001) { dx = 0.001; d = 0.001; } // 中心一致の保険
                    double nx = dx / d;
                    double ny = dy / d;
                    // めり込み分を表面の外へ押し出す
                    p->x = b->x + nx * (rr + 1.0);
                    p->y = b->y + ny * (rr + 1.0);

                    double mvx = p->speed * cos(p->muki);
                    double mvy = p->speed * sin(p->muki);
                    double dot = mvx * nx + mvy * ny;
                    if (dot < 0.0) { // 花粉に近づいてきた分子だけ反射
                        double rvx = mvx - 2.0 * dot * nx;
                        double rvy = mvy - 2.0 * dot * ny;
                        p->muki = atan2(rvy, rvx);
                        // 花粉へ衝撃(蒸発中は除く)
                        if (b->param_i[1] == 0) {
                            b->param_d[0] += IMPULSE * mvx;
                            b->param_d[1] += IMPULSE * mvy;
                        }
                    }
                    break; // 複数花粉との重なりは次フレーム以降に解消させる
                }
            }
        }
        p = p->next;
    }

    // ---- 花粉同士の衝突(等質量の弾性衝突: 法線方向の速度成分を交換) ----
    for (int i = 0; i < nBig; i++) {
        for (int j = i + 1; j < nBig; j++) {
            sEnemyShot* a = big[i];
            sEnemyShot* b = big[j];
            if (a->param_i[1] != 0 || b->param_i[1] != 0) continue;

            double dx = b->x - a->x;
            double dy = b->y - a->y;
            double rr = BALL_R * 2.0;
            double d2 = dx * dx + dy * dy;
            if (d2 < rr * rr && d2 > 0.0001) {
                double d = sqrt(d2);
                double nx = dx / d;
                double ny = dy / d;
                double ov = (rr - d) * 0.5;
                a->x -= nx * ov;  a->y -= ny * ov;   // 重なりの解消
                b->x += nx * ov;  b->y += ny * ov;

                double van = a->param_d[0] * nx + a->param_d[1] * ny;
                double vbn = b->param_d[0] * nx + b->param_d[1] * ny;
                if (vbn - van < 0.0) {               // 接近中のみ交換
                    a->param_d[0] += (vbn - van) * nx;
                    a->param_d[1] += (vbn - van) * ny;
                    b->param_d[0] += (van - vbn) * nx;
                    b->param_d[1] += (van - vbn) * ny;
                }
            }
        }
    }
}

// ================= 敵本体のパターン =================
void EnemyPat_BrownianMotion_Zai()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;

        // 弾幕セットはこの1つだけ生成する。
        // (大玉も小玉も全てこのセットのリストで管理し、
        //   相互の衝突計算を単一リスト内で完結させている)
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBrownian;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = DX_PI / 2.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        // 熱源としてゆっくり左右に揺れる(分子の噴出位置が散らばる)
        enemy.x += 0.7 * muki;
        if (enemy.x < 70.0)  muki = 1;
        if (enemy.x > 410.0) muki = -1;
    }
}