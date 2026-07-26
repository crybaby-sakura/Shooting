// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ルール30 弾幕パターン用 静的状態管理
// ============================================================
static int g_cells[61];
static int g_next_cells[61];
static bool g_initialized = false;

// ============================================================
// ルール30 パターン関数
// ============================================================
static void Rule30Pattern(sEnemyShotSet* pEnemyShotSet)
{
    const int N = 61;
    const double cell_width = 480.0 / N; // 画面幅480を61等分

    // 初回初期化
    if (!g_initialized) {
        for (int i = 0; i < N; i++) {
            g_cells[i] = 0;
        }
        g_cells[N / 2] = 1; // 中央のみ1で初期化
        g_initialized = true;
    }

    // 8フレームごとに世代更新と弾の発射を行う
    if (pEnemyShotSet->count % 8 == 0) {
        // 発射予告音・効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 次の世代の状態を計算 (ルール30)
        for (int i = 0; i < N; i++) {
            int left = (i > 0) ? g_cells[i - 1] : 0;
            int center_val = g_cells[i];
            int right = (i < N - 1) ? g_cells[i + 1] : 0;

            // 3ビットのパターンをインデックスに変換 (例: 110 -> 6)
            int pattern = (left << 2) | (center_val << 1) | right;

            // ルール30の遷移表: 000->0, 001->1, 010->1, 011->1, 100->1, 101->0, 110->0, 111->0
            int rule30[] = { 0, 1, 1, 1, 1, 0, 0, 0 };
            g_next_cells[i] = rule30[pattern];
        }

        // 計算した次の世代に基づいて弾を発射
        // 時間経過で色を変化させ、模様の世代を視覚的に表現する (0:赤 ～ 8:橙)
        int color = (pEnemyShotSet->count / 8) % 9;
        double start_x = pEnemyShotSet->x - (N * cell_width) / 2.0; // 画面中央揃えの開始X座標

        for (int i = 0; i < N; i++) {
            if (g_next_cells[i] == 1) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                // セルの中心座標を計算
                pEnemyShot->x = start_x + (i * cell_width) + (cell_width / 2.0);
                pEnemyShot->y = 0.0;

                // 真下への直線落下とすることで、セルの配置と弾の軌跡が一致し「模様」として認識しやすくなる
                pEnemyShot->muki = DX_PI / 2.0;
                pEnemyShot->speed = 3.0; // 8フレーム間隔×速度3.0 = 24px間隔で模様がくっきり見える

                // 視認性が高く、幾何学模様を際立たせる「小玉」を使用
                pEnemyShot->kind = img_enemyShotSmallBall[color];

                // 弾をショットセットのリストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 状態を更新
        for (int i = 0; i < N; i++) {
            g_cells[i] = g_next_cells[i];
        }
    }

    // 弾の移動処理
    // (画面外消去やcountインクリメントはメインルーチンで行われる仕様)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Rule30_Qwen()
{
    static sEnemyShotSet* pRule30Set = nullptr;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 150; // パターンが長く続くためHPを多めに設定

        // パターン開始時の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 静的状態のリセット
        g_initialized = false;

        // ショットセットの初期化
        pRule30Set = new sEnemyShotSet;
        pRule30Set->count = 0;
        pRule30Set->patternFunc = Rule30Pattern;
        pRule30Set->x = enemy.x;
        pRule30Set->y = enemy.y;
        pRule30Set->kind = 0;

        // ダミーヘッドノードの作成
        pRule30Set->pEnemyShotHead = new sEnemyShot;
        pRule30Set->pEnemyShotHead->prev = pRule30Set->pEnemyShotHead;
        pRule30Set->pEnemyShotHead->next = pRule30Set->pEnemyShotHead;

        // グローバルリストに追加
        pRule30Set->prev = enemyShotSetHead.prev;
        pRule30Set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pRule30Set;
        enemyShotSetHead.prev = pRule30Set;
    }
}