#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 定数定義
static const double PI2 = DX_PI * 2.0;

// ============================================================
//  弾幕パターン：リサジュー・ルーム (Lissajous Loom)
// ============================================================
static void ShotLissajous(sEnemyShotSet* pSet)
{
    // リサジュー曲線のパラメータ
    const double A = 48.0 * 3;          // 横振幅
    const double B = 36.0 * 3;          // 縦振幅
    const int    a = 3;             // 周波数比 x
    const int    b = 4;             // 周波数比 y
    
    double delta = pSet->param_d[0]; // リサジューの位相差 (キャリアごとに異なる)

    // --- 初期化フェーズ (count == 0) ---
    if (pSet->count == 0) {
        // 効果音: リボン出現音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // パラメータの取得
        int color_idx = pSet->param_i[0]; // 色のインデックス
                
        // 曲線の描画範囲 (0 から 1.3PI までとし、アーチ状にする)
        const double U_START = 0.0;
        const double U_END = DX_PI * 1.3;
        const int    N = 42 * 2;      // 1本のリボンを構成する弾数

        for (int i = 0; i < N; i++) {
            // u を均等に分布させる
            double u = U_START + (U_END - U_START) * i / (double)(N - 1);

            sEnemyShot* pShot = new sEnemyShot;

            // 画像と色の設定 (中玉を使用)
            pShot->kind = img_enemyShotSmallBall[color_idx];
            pShot->margin = 480;

            // 個々の弾が自分の u (曲線上の位置) を覚える
            pShot->param_d[0] = u;
           
            // 双方向循環リストに追加
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // --- 更新フェーズ (毎フレーム) ---

    // 1. キャリア自体の下降
    pSet->y += 1.4; // 下降速度 (ピクセル/フレーム)

    // 2. 各弾の座標をリサジュー式で再計算
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double u = pShot->param_d[0]; // 保存しておいた u
        pShot->param_d[0] += 0.001;

        // リサジュー曲線の計算式
        // x = A * sin(a*u + delta)
        // y = B * sin(b*u)
        double local_x = A * sin(a * u + delta);
        double local_y = B * sin(b * u);

        // キャリアの位置を基準に配置
        pShot->x = pSet->x + local_x;
        pShot->y = pSet->y + local_y;

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_Lissajous_Qwen()
{
    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // ボスの移動 (サインカーブで左右に揺れる)
    enemy.x += 1.2 * cos((count - 1) * 0.025);

    // 弾幕キャリアの生成 (240フレーム = 約4秒ごと)
    if (count % 480 == 10) {
        // 3本のリボンを生成
        for (int c = 0; c < 3; c++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotLissajous;

            // 初期位置 (ボスの少し下から、3本を縦にずらして配置)
            pSet->x = enemy.x;
            pSet->y = enemy.y + 30.0 - c * 120.0 * 2;

            // パラメータ設定
            // param_i[0]: 色 (0:赤, 3:シアン, 5:マゼンタ をローテーション)
            int colors[] = { 0, 3, 5 };
            pSet->param_i[0] = colors[c % 3];

            // param_d[0]: 位相差 delta (3本が互い違いに絡むように 120度ずつずらす)
            pSet->param_d[0] = c * (PI2 / 3.0);

            // リストヘッドの初期化
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            // グローバルリスト (enemyShotSetHead) に接続
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}