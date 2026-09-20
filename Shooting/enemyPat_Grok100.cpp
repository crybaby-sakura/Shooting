// enemyPat_Hilbert.cpp
// ヒルベルト曲線をモチーフにした段階的充填弾幕
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ヒルベルト曲線ユーティリティ（標準的な d2xy）
// n は 2の冪（グリッドサイズ）、d は 0 ～ n*n-1 のインデックス
// ============================================================
static void rot(int n, int* x, int* y, int rx, int ry)
{
    if (ry == 0) {
        if (rx == 1) {
            *x = n - 1 - *x;
            *y = n - 1 - *y;
        }
        // Swap x and y
        int t = *x;
        *x = *y;
        *y = t;
    }
}

static void d2xy(int n, int d, int* x, int* y)
{
    int rx, ry, s, t = d;
    *x = *y = 0;
    for (s = 1; s < n; s *= 2) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);
        rot(s, x, y, rx, ry);
        *x += s * rx;
        *y += s * ry;
        t /= 4;
    }
}

// ============================================================
// 弾幕パターン：ヒルベルト曲線段階的充填
// pEnemyShotSet->param_i[0] : 現在のオーダー (1〜4)
// pEnemyShotSet->param_i[1] : 次に生成する曲線上のインデックス
// pEnemyShotSet->param_i[2] : 1フレームあたりに生成する弾数
// pEnemyShotSet->param_d[0] : 曲線を描く正方形の左上X
// pEnemyShotSet->param_d[1] : 曲線を描く正方形の左上Y
// pEnemyShotSet->param_d[2] : 1セルあたりのピクセルサイズ
// ============================================================
static void ShotHilbertFill(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ効果音
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // パラメータ取得
    int order = pEnemyShotSet->param_i[0];
    if (order < 1) order = 1;
    if (order > 4) order = 4;          // 16x16 = 256点まで（負荷と見やすさのバランス）
    int n = 1 << order;                // グリッドサイズ (2,4,8,16)
    int totalPoints = n * n;           // 曲線上の総点数
    int& nextIndex = pEnemyShotSet->param_i[1];
    int spawnPerFrame = pEnemyShotSet->param_i[2];
    if (spawnPerFrame < 1) spawnPerFrame = 1;

    double baseX = pEnemyShotSet->param_d[0];
    double baseY = pEnemyShotSet->param_d[1];
    double cellSize = pEnemyShotSet->param_d[2];

    // 曲線上の点を順次弾として生成（段階的に埋めていく）
    for (int s = 0; s < spawnPerFrame; s++) {
        if (nextIndex >= totalPoints) break;

        int hx, hy;
        d2xy(n, nextIndex, &hx, &hy);

        // 画面座標に変換（セル中心）
        double bx = baseX + (hx + 0.5) * cellSize;
        double by = baseY + (hy + 0.5) * cellSize;

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = bx;
        pEnemyShot->y = by;

        // 中心から外側へ少しだけ押し出す方向（完全静止だと味気ないため）
        double dx = bx - (baseX + n * 0.5 * cellSize);
        double dy = by - (baseY + n * 0.5 * cellSize);
        double len = sqrt(dx * dx + dy * dy);
        if (len < 1.0) len = 1.0;
        pEnemyShot->muki = atan2(dy, dx);
        // オーダーが高いほど速度を落として密に見せる
        pEnemyShot->speed = 0.15 + (4 - order) * 0.12;

        // 弾種・色：オーダーに応じて変化させ視覚的階層を強調
        // 小玉〜中玉を中心に、色は青〜シアン〜白系統で統一感を出す
        int colorIdx;
        switch (order) {
        case 1: colorIdx = 4; break; // 青
        case 2: colorIdx = 3; break; // シアン
        case 3: colorIdx = 6; break; // 白
        default: colorIdx = 5; break; // マゼンタ
        }
        // オーダーが低い（粗い）ときは少し大きめの弾、高いときは小玉
        if (order <= 2) {
            pEnemyShot->kind = img_enemyShotMediumBall[colorIdx];
        }
        else {
            pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];
        }

        // 自由パラメータに自分の曲線インデックスを記録（必要なら後で利用可能）
        pEnemyShot->param_i[0] = nextIndex;
        pEnemyShot->param_i[1] = order;

        // リストに追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        nextIndex++;
    }

    // 既に生成済みの弾を移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ヒルベルト曲線を段階的に細かくしながらエリアを埋めていく
// ============================================================
void EnemyPat_HilbertCurve_Grok()
{
    static int phase;          // 0:移動準備, 1〜:曲線オーダー
    static int wait;           // 次のショットセットまでの待ち
    static double areaSize;    // 現在の正方形エリアの一辺

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 1;
        wait = 30;             // 最初の待機
        areaSize = 280.0;      // 初期エリアサイズ
    }
    else {
        // 敵本体は画面上部でゆっくり左右に揺れる
        enemy.x = 240.0 + 80.0 * sin(count * 0.02);
        enemy.y = 60.0 + 8.0 * sin(count * 0.03);
    }

    // 一定間隔で次のオーダーのヒルベルト曲線弾幕を発射
    if (wait > 0) {
        wait--;
        return;
    }

    // 最大オーダー4まで段階的に上げる。終わったらループ
    if (phase > 4) {
        phase = 1;
        areaSize = 260.0 + GetRand(40); // 少しサイズを変えてバリエーション
    }

    // 新しいショットセットを作成
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotHilbertFill;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = phase;

    // パラメータ設定
    // order
    pEnemyShotSet->param_i[0] = (phase == 0) ? 1 : phase;
    // 次の生成インデックス
    pEnemyShotSet->param_i[1] = 0;
    // 1フレームあたりの生成数（オーダーが高いほど多めに出してテンポを保つ）
    int order = pEnemyShotSet->param_i[0];
    pEnemyShotSet->param_i[2] = 1 + order;   // 2〜5個/frame

    // 正方形エリアの配置（画面中央寄り、敵の下）
    double half = areaSize * 0.5;
    pEnemyShotSet->param_d[0] = enemy.x - half;           // left
    pEnemyShotSet->param_d[1] = enemy.y + 80.0;                  // top（敵の下から）
    // セルサイズ = エリア一辺 / グリッド数
    int n = 1 << order;
    pEnemyShotSet->param_d[2] = areaSize / (double)n;

    // 弾リストのダミーヘッド
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    // ショットセットリストに追加
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    // 次の準備
    phase++;
    // オーダーが高いほど生成に時間がかかるので待ちを長くする
    // 総点数 = n*n、1フレーム spawnPerFrame 個なので概算フレーム数 + 余韻
    int total = n * n;
    int spf = 1 + order;
    wait = (total / spf) + 40 + GetRand(20);
}