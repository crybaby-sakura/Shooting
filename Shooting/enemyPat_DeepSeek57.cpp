// EnemyPat_Rule30Wave.cpp
// ルール30の降下波動 + 色分け警告 弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"

// ---------- 定数 ----------
static constexpr int    CA_WIDTH = 49;          // セル数（奇数推奨、画面横幅480）
static constexpr double CELL_SIZE = 480.0 / CA_WIDTH;
static constexpr double BULLET_SPEED = 2.8;          // 弾の落下速度（ピクセル/フレーム）
static constexpr int    SPAWN_INTERVAL = 4;            // 新世代の弾を何フレームごとに生成するか

// ---------- ルール30 弾幕パターン ----------
static void Rule30Wave(sEnemyShotSet* pEnemyShotSet)
{
    // セルオートマトンの状態（前世代）
    static int ca_gen[CA_WIDTH];
    static bool initialized = false;

    // 初回呼び出し時に初期世代(世代0)を生成
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < CA_WIDTH; ++i) ca_gen[i] = 0;
        ca_gen[CA_WIDTH / 2] = 1;           // 画面中央の1セルのみ生存
        initialized = true;
    }

    // ---- 毎フレーム：既存弾の移動 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->y += BULLET_SPEED;           // 等速で下へ落下
        if (pShot->count == 90 && pShot->kind == img_enemyShotSmallBall[1]) {
            if (GetRand(2) != 0) {
                pShot->margin = -9999;
            }
        }
        pShot = pShot->next;
    }

    // ---- SPAWN_INTERVAL フレームごとに新世代の弾を生成 ----
    if (pEnemyShotSet->count % SPAWN_INTERVAL == 0 && pEnemyShotSet->count <= 360) {
        int next_gen[CA_WIDTH] = { 0 };

        for (int i = 0; i < CA_WIDTH; ++i) {
            // 近傍3セルを取得（端は0固定）
            int left = (i > 0) ? ca_gen[i - 1] : 0;
            int center = ca_gen[i];
            int right = (i < CA_WIDTH - 1) ? ca_gen[i + 1] : 0;
            int pattern = (left << 2) | (center << 1) | right;   // 3bitパターン

            // ルール30の遷移を適用
            int result;
            switch (pattern) {
            case 7: result = 0; break;  // 111 -> 0
            case 6: result = 0; break;  // 110 -> 0
            case 5: result = 0; break;  // 101 -> 0
            case 4: result = 1; break;  // 100 -> 1
            case 3: result = 1; break;  // 011 -> 1
            case 2: result = 1; break;  // 010 -> 1  ★警告パターン
            case 1: result = 1; break;  // 001 -> 1
            case 0: result = 0; break;  // 000 -> 0
            }
            next_gen[i] = result;

            // 生存セル → 弾を発射
            if (result == 1) {
                // 弾の出現位置（画面上端）
                double spawn_x = i * CELL_SIZE + CELL_SIZE * 0.5;
                double spawn_y = 0.0;

                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = spawn_x;
                pEnemyShot->y = spawn_y;
                pEnemyShot->muki = DX_PI / 2;       // 直進落下のため未使用
                pEnemyShot->speed = 0.0;       // 同上

                // ★色分け警告★
                // 近傍パターンが 010 (左:0, 中:1, 右:0) のときだけ
                // 赤い菱形弾（img_enemyShotDiamond[0]）を使い、
                // それ以外の発生パターン（100,011,001）は黄色い小玉（img_enemyShotSmallBall[1]）にする。
                // これにより「孤立した親セルから生まれる弾」が視覚的に際立ち、
                // プレイヤーに注意を促す。
                if (pattern == 2) {                     // 010 パターン → 警告
                    pEnemyShot->kind = img_enemyShotDiamond[0];   // 赤 (色番号0)
                }
                else {
                    pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄 (色番号1)
                }

                // 弾リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 次世代に更新
        for (int i = 0; i < CA_WIDTH; ++i) ca_gen[i] = next_gen[i];
    }
}

// ---------- 敵本体パターン ----------
void EnemyPat_Rule30_DeepSeek()
{
    static int muki; // 横移動の向き

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 簡単な水平往復移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    if (count % 420 == 1) {
        // ルール30弾幕を管理するショットセットを1つだけ生成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Rule30Wave;     // 上で定義したパターン関数
        pSet->x = 0.0;
        pSet->y = 0.0;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // 弾リストのヘッダを初期化
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルなショットセットリストに登録
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}