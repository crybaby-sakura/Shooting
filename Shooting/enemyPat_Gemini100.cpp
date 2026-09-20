// enemyPat_hilbertMaze.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ======================================================================
// ヒルベルト曲線の座標データ管理
// ======================================================================
static double hilbert_x[256];
static double hilbert_y[256];
static bool hilbert_init = false;

// ヒルベルト曲線(オーダー4: 256頂点)の座標を事前計算する関数
static void InitHilbert() {
    int n = 16;
    for (int d = 0; d < 256; d++) {
        int rx, ry, s, t = d;
        int x = 0, y = 0;
        for (s = 1; s < n; s *= 2) {
            rx = 1 & (t / 2);
            ry = 1 & (t ^ rx);
            if (ry == 0) {
                if (rx == 1) {
                    x = s - 1 - x;
                    y = s - 1 - y;
                }
                int temp = x; x = y; y = temp;
            }
            x += s * rx;
            y += s * ry;
            t /= 4;
        }
        // 画面(480x480)の中心付近を基準に、28ピクセル間隔で配置
        // (小玉同士の隙間が自機の通り道になります)
        hilbert_x[d] = 240.0 + (x - 7.5) * 28.0;
        hilbert_y[d] = 240.0 + (y - 7.5) * 28.0;
    }
    hilbert_init = true;
}


// ======================================================================
// 弾幕パターン定義
// ======================================================================

// 弾幕：壁生成（ヒルベルト曲線の描画）
static void ShotWall(sEnemyShotSet* pEnemyShotSet)
{
    // 1フレームに4辺ずつ壁を展開し、約1秒強で全体を構築する
    int start_d = pEnemyShotSet->count * 4;
    int end_d = start_d + 4;

    if (start_d < 255) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int d = start_d; d < end_d && d < 255; d++) {
            // 頂点と頂点の間を3分割して小玉を並べ、壁の線を形成
            for (int i = 0; i < 3; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                double rate = i / 3.0; // 0.0, 0.33, 0.66
                pEnemyShot->x = hilbert_x[d] * (1.0 - rate) + hilbert_x[d + 1] * rate;
                pEnemyShot->y = hilbert_y[d] * (1.0 - rate) + hilbert_y[d + 1] * rate;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0; // 完全に静止させる

                // 3番（シアン）の小玉を使用してサイバー感を出す
                pEnemyShot->kind = img_enemyShotSmallBall[3];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // 最後の終点を塞ぐ
            if (d == 254) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = hilbert_x[255];
                pEnemyShot->y = hilbert_y[255];
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[3];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
}

// 弾幕：サージ弾（一筆書きのルートを走るパルス）
static void ShotSurge(sEnemyShotSet* pEnemyShotSet)
{
    // 一定間隔（45フレーム毎）でサージ弾を発射し続ける
    if (pEnemyShotSet->count % 48 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = hilbert_x[0]; // 迷路の始点
        pEnemyShot->y = hilbert_y[0];
        pEnemyShot->speed = 10.0; // 比較的高速
        pEnemyShot->kind = img_enemyShotLargeBall[5]; // 5番（マゼンタ）の中楕円弾

        // param_i[0] を「次に目指す頂点のインデックス」として利用する
        pEnemyShot->param_i[0] = 1;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 既に発射されているサージ弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int target_idx = pShot->param_i[0];

        // 迷路内を走行中
        if (target_idx < 256) {
            double tx = hilbert_x[target_idx];
            double ty = hilbert_y[target_idx];
            double dx = tx - pShot->x;
            double dy = ty - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);

            // ターゲット頂点に到達、あるいは通り過ぎる場合
            if (dist <= pShot->speed) {
                pShot->x = tx; // 頂点位置にぴったりスナップさせる
                pShot->y = ty;
                pShot->param_i[0]++; // 次の目標インデックスへ

                if (pShot->param_i[0] < 256) {
                    // 次の目標への角度を計算（楕円弾の描画向きを曲げるため）
                    double nx = hilbert_x[pShot->param_i[0]];
                    double ny = hilbert_y[pShot->param_i[0]];
                    pShot->muki = atan2(ny - pShot->y, nx - pShot->x);
                }
                else {
                    // 迷路を完走したら画面下方向へ飛び去るようにする
                    pShot->muki = DX_PI / 2.0 * 0;
                }
            }
            else {
                // 目標に向かって進む
                pShot->muki = atan2(dy, dx);
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        // 迷路完走後（画面外への離脱）
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}


// ======================================================================
// 敵本体の挙動（ボスAI）
// ======================================================================
void EnemyPat_HilbertCurve_Gemini()
{
    // 初回実行時にヒルベルト曲線の座標を計算
    if (!hilbert_init) {
        InitHilbert();
    }

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0; // 画面上部中央付近に鎮座
        enemy.maxHp = enemy.hp = 200;

        // 予告音としてチャージ音を鳴らす
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 1. 空間充填の壁（迷路）を生成する弾幕セットを登録
        sEnemyShotSet* pWallSet = new sEnemyShotSet;
        pWallSet->count = 0;
        pWallSet->patternFunc = ShotWall;
        pWallSet->pEnemyShotHead = new sEnemyShot;
        pWallSet->pEnemyShotHead->prev = pWallSet->pEnemyShotHead;
        pWallSet->pEnemyShotHead->next = pWallSet->pEnemyShotHead;

        pWallSet->prev = enemyShotSetHead.prev;
        pWallSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWallSet;
        enemyShotSetHead.prev = pWallSet;
    }

    // 壁が全体に行き渡る直前のタイミング（60フレーム後）でサージ弾の射出を開始
    if (count == 60) {
        // 2. 経路に沿って一筆書きサージを走らせる弾幕セットを登録
        sEnemyShotSet* pSurgeSet = new sEnemyShotSet;
        pSurgeSet->count = 0;
        pSurgeSet->patternFunc = ShotSurge;
        pSurgeSet->pEnemyShotHead = new sEnemyShot;
        pSurgeSet->pEnemyShotHead->prev = pSurgeSet->pEnemyShotHead;
        pSurgeSet->pEnemyShotHead->next = pSurgeSet->pEnemyShotHead;

        pSurgeSet->prev = enemyShotSetHead.prev;
        pSurgeSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSurgeSet;
        enemyShotSetHead.prev = pSurgeSet;
    }
}