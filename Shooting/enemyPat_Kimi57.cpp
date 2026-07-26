// enemyPat_rule30.cpp
// セル・オートマトン ルール30 をモチーフにした弾幕パターン
// 非対称三角陣（Asymmetric Rule30 Barrage）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// パラメータ設定
// ------------------------------------------------------------
#define CELL_COUNT    35      // 横一列のセル数（奇数推奨：中央を基準にするため）
#define CELL_SPACING  20.0    // セル間の水平間隔 [px]
#define GEN_INTERVAL  15      // 世代更新間隔 [フレーム]
#define SHOT_SPEED    2.0     // 弾の落下速度

// ------------------------------------------------------------
// 弾幕セット内の弾移動処理
// 各弾は生成時に決められた向き（真下）と速度で直進する
// ------------------------------------------------------------
static void ShotRule30(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// 画面上部に横一列のセルを配置し、ルール30の状態に応じて弾を真下に発射。
// 一定フレームごとに次世代へ遷移し、カオス的な非対称弾幕壁を形成する。
// ------------------------------------------------------------
void EnemyPat_Rule30_Kimi()
{
    static int    muki;                       // 敵の移動方向（1 or -1）
    static int    cellState[CELL_COUNT];      // 現在のルール30セル状態（0 or 1）
    static int    genCount;                   // 世代カウンタ（kind に使用）

    // ===== 初期化 =====
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        genCount = 0;

        // ルール30の初期状態：中央の1セルのみが1、残りは0
        for (int i = 0; i < CELL_COUNT; i++) {
            cellState[i] = 0;
        }
        cellState[CELL_COUNT / 2] = 1;

        // パターン開始の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    // ===== 敵移動 =====
    else {
        enemy.x += 0.88 * (double)muki;
        if (count % 120 == 60) muki *= -1;    // 120F周期で方向転換
    }

    // ===== 世代更新・弾生成 =====
    if (count % GEN_INTERVAL == 1) {
        // --- 新しい弾幕セットを生成 ---
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRule30;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;    // 真下
        pEnemyShotSet->kind = genCount++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 発射音（軽快な連射感）
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // --- 現在のセル状態に基づいて弾を配置 ---
        // セル群は敵の現在位置を中心に水平展開
        double startX = enemy.x - (CELL_COUNT - 1) * CELL_SPACING / 2.0;

        for (int i = 0; i < CELL_COUNT; i++) {
            if (cellState[i] == 1) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = startX + i * CELL_SPACING;
                pEnemyShot->y = enemy.y + 10.0;
                pEnemyShot->muki = DX_PI / 2.0;   // 真下
                pEnemyShot->speed = SHOT_SPEED;

                // 色分け：ルール30の非対称性を直感的に演出
                //   左側  ：青系（シアン/青）  → 比較的規則的な印象
                //   中央  ：白                  → 初期状態の象徴
                //   右側  ：赤系（赤/マゼンタ） → カオス的な印象
                int color;
                if (i < CELL_COUNT / 2) {
                    color = (i % 2 == 0) ? 4 : 3;   // 青(4) or シアン(3)
                }
                else if (i > CELL_COUNT / 2) {
                    color = (i % 2 == 0) ? 0 : 5;   // 赤(0) or マゼンタ(5)
                }
                else {
                    color = 6;                       // 白(6)
                }
                // 弾種：小玉（処理負荷を抑えつつ弾幕の粒立ちを綺麗に見せる）
                pEnemyShot->kind = img_enemyShotMediumBall[color];

                // 双方向リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 弾幕セットをグローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // --- 次世代のセル状態を計算（ルール30） ---
        // 遷移規則：
        //   111->0, 110->0, 101->0, 100->1, 011->1, 010->1, 001->1, 000->0
        int nextState[CELL_COUNT];
        for (int i = 0; i < CELL_COUNT; i++) {
            int left = (i > 0) ? cellState[i - 1] : 0;
            int self = cellState[i];
            int right = (i < CELL_COUNT - 1) ? cellState[i + 1] : 0;
            int pattern = (left << 2) | (self << 1) | right;

            // pattern == 4(100), 3(011), 2(010), 1(001) のとき 1
            nextState[i] = (pattern == 4 || pattern == 3 || pattern == 2 || pattern == 1) ? 1 : 0;
        }
        for (int i = 0; i < CELL_COUNT; i++) {
            cellState[i] = nextState[i];
        }
    }
}