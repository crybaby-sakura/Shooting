// enemyPat_ShihenRantou.cpp
// 四片乱塔 ―― テトリスをテーマにした4フェーズ(降下→積層→ライン消去→崩壊リセット)パターン
//
// フェーズ1 降下   : 7種テトリミノが剛体フォーメーションとして回転しながら落下する
// フェーズ2 積層   : 着地したブロックが盤面下部に積み上がっていく
// フェーズ3 ライン消去 : 横一列が埋まると、その列がレーザー状に左右へ飛散し、自機狙い3wayが返礼として飛ぶ
// フェーズ4 トップアウト: 積み木が天井に達すると全ブロックが盤面中心から放射状に崩壊し、フェーズ1へループ

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  盤面設定
// ============================================================
static const int    NUM_COLS = 8;
static const int    NUM_ROWS = 7;
static const double CELL_SIZE = 45.0;
static const double BOARD_LEFT = 60.0;   // 盤面左端 x
static const double BOARD_FLOOR_Y = 430.0;  // 床(行0の下端)の y
static const double SPAWN_PIVOT_Y = 90.0;   // テトリミノ出現時の基準 y(常に画面上部)
static const double FALL_SPEED = 2.2;    // 落下速度(px/frame)
static const double SPIN_RATE = 0.16;   // 残り落下時間1frameあたりの回転量(rad)
static const double BURST_SPEED = 6.0;    // バースト後の飛散速度(px/frame)

static double ColToX(int col) { return BOARD_LEFT + CELL_SIZE * (col + 0.5); }
static double RowToY(int row) { return BOARD_FLOOR_Y - CELL_SIZE * (row + 0.5); }
static double BoardCenterX() { return BOARD_LEFT + CELL_SIZE * NUM_COLS * 0.5; }
static double BoardCenterY() { return BOARD_FLOOR_Y - CELL_SIZE * NUM_ROWS * 0.5; }

// ============================================================
//  テトリミノ形状定義(色は本家配色に準拠)
// ============================================================
struct ShapeCell { int c, r; };

static const ShapeCell SHAPE_I[4] = { {0,0},{1,0},{2,0},{3,0} };
static const ShapeCell SHAPE_O[4] = { {0,0},{1,0},{0,1},{1,1} };
static const ShapeCell SHAPE_T[4] = { {0,0},{1,0},{2,0},{1,1} };
static const ShapeCell SHAPE_S[4] = { {1,0},{2,0},{0,1},{1,1} };
static const ShapeCell SHAPE_Z[4] = { {0,0},{1,0},{1,1},{2,1} };
static const ShapeCell SHAPE_J[4] = { {0,0},{0,1},{1,1},{2,1} };
static const ShapeCell SHAPE_L[4] = { {2,0},{0,1},{1,1},{2,1} };

static const ShapeCell* const SHAPE_TABLE[7] = { SHAPE_I, SHAPE_O, SHAPE_T, SHAPE_S, SHAPE_Z, SHAPE_J, SHAPE_L };
static const int SHAPE_WIDTH[7] = { 4, 2, 3, 3, 3, 3, 3 };
static const int SHAPE_COLOR[7] = { 3, 1, 5, 2, 0, 4, 8 }; // シアン,黄,マゼンタ,緑,赤,青,橙

// ============================================================
//  盤面状態(このファイル内の全パターン関数が共有する権威データ)
// ============================================================
static bool           g_boardFilled[NUM_COLS][NUM_ROWS];
static sEnemyShotSet* g_boardOwner[NUM_COLS][NUM_ROWS];
static int            g_colHeight[NUM_COLS];     // 実際に着地済みのブロックによる高さ(ライン消去判定用)
static int            g_reservedHeight[NUM_COLS]; // 落下中も含めた「予約済み」の高さ(新規出現の重なり防止用)
static int            g_spawnCooldownUntil = 0;

// 消去などで盤面が変化した直後に、列の高さキャッシュを実データから引き直す
static void RecomputeColHeight(int col)
{
    for (int r = NUM_ROWS - 1; r >= 0; r--) {
        if (g_boardFilled[col][r]) {
            g_colHeight[col] = r + 1;
            if (g_reservedHeight[col] < g_colHeight[col]) g_reservedHeight[col] = g_colHeight[col];
            return;
        }
    }
    g_colHeight[col] = 0;
}

// 指定セル(col,row)を所有するShotSetの中から、そのセルに属する4発クラスタを
// バースト状態へ遷移させる。burstType: 1=ライン消去 / 2=トップアウト
//
// 注意: バースト開始時刻は「そのセル自身のpShot->count」で記録する。
// 弾の位置式はすべて各弾自身のpShot->count(生成からの経過フレーム数)を基準に
// 組み立てているため、ここをグローバルなカウンタで記録すると基準がずれて
// バースト後の経過フレーム数が大きく負になり、座標が暴走する。
static void TriggerCellBurst(int col, int row, int burstType, int laserImgIdx)
{
    sEnemyShotSet* pSet = g_boardOwner[col][row];
    if (!pSet) return;

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == col && pShot->param_i[1] == row && pShot->param_d[6] < 0.0) {
            pShot->param_d[6] = (double)pShot->count; // このセル自身の経過フレーム数を基準点にする
            pShot->param_i[2] = burstType;
            if (laserImgIdx >= 0) pShot->kind = laserImgIdx;
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  お返しの自機狙いN-way(ライン消去・トップアウト時に発射)
// ============================================================
static void ShotAimedFan(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        int n = pSet->param_i[0];
        double spread = pSet->param_d[0];
        double baseMuki = atan2(player.y - pSet->y, player.x - pSet->x);

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < n; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double t = (n == 1) ? 0.0 : ((double)i / (n - 1) - 0.5); // -0.5〜0.5
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = baseMuki + t * spread;
            pShot->speed = 3.4;
            pShot->kind = img_enemyShotBullet[6];

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 発射位置からの単純な等速直線(pShot->countのみから導出、加算的な積分はしない)
        pShot->x = pSet->x + pShot->speed * cos(pShot->muki) * pShot->count;
        pShot->y = pSet->y + pShot->speed * sin(pShot->muki) * pShot->count;
        pShot = pShot->next;
    }
}

static void SpawnAimedFan(double x, double y, int n, double spreadRad)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotAimedFan;
    pSet->x = x;
    pSet->y = y;
    pSet->param_i[0] = n;
    pSet->param_d[0] = spreadRad;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// 横一列が埋まっていれば消去し、バースト+自機狙い3wayの反撃を起こす
static void CheckAndClearLines()
{
    for (int row = 0; row < NUM_ROWS; row++) {
        bool full = true;
        for (int c = 0; c < NUM_COLS; c++) {
            if (!g_boardFilled[c][row]) { full = false; break; }
        }
        if (!full) continue;

        for (int c = 0; c < NUM_COLS; c++) {
            TriggerCellBurst(c, row, 1, img_enemyShotLaser[6]);
            g_boardFilled[c][row] = false;
            g_boardOwner[c][row] = nullptr;
        }
        for (int c = 0; c < NUM_COLS; c++) RecomputeColHeight(c);

        SpawnAimedFan(BoardCenterX(), RowToY(row), 3, 40.0 * DX_PI / 180.0);
    }
}

// 同時に複数ピースが落下中でも重ならないよう、「予約済み高さ」を基準に
// ハードドロップ位置(pieceBaseRow)を求める(g_reservedHeightは書き換えない)
static int ComputePieceBaseRow(int shape, int startCol)
{
    const ShapeCell* cells = SHAPE_TABLE[shape];
    int maxLocalRInCol[NUM_COLS];
    for (int i = 0; i < NUM_COLS; i++) maxLocalRInCol[i] = -1;
    for (int i = 0; i < 4; i++) {
        int col = startCol + cells[i].c;
        if (cells[i].r > maxLocalRInCol[col]) maxLocalRInCol[col] = cells[i].r;
    }
    int pieceBaseRow = 0;
    for (int c = 0; c < NUM_COLS; c++) {
        if (maxLocalRInCol[c] < 0) continue;
        int need = g_reservedHeight[c] + maxLocalRInCol[c];
        if (need > pieceBaseRow) pieceBaseRow = need;
    }
    return pieceBaseRow;
}

// ============================================================
//  テトリミノ本体のパターン関数
// ============================================================
static void ShotTetromino(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        int shape = pSet->param_i[0];
        int startCol = pSet->param_i[1];
        int pieceBaseRow = pSet->param_i[3]; // SpawnTetrominoで確定済みの着地位置
        const ShapeCell* cells = SHAPE_TABLE[shape];
        int color = SHAPE_COLOR[shape];

        // 4セルの最終着地座標と、その重心(=回転軸)を求める
        double finalX[4], finalY[4];
        int    col[4], row[4];
        double cx = 0.0, cy = 0.0;
        for (int i = 0; i < 4; i++) {
            col[i] = startCol + cells[i].c;
            row[i] = pieceBaseRow - cells[i].r;
            finalX[i] = ColToX(col[i]);
            finalY[i] = RowToY(row[i]);
            cx += finalX[i] * 0.25;
            cy += finalY[i] * 0.25;
            // (着地マスの予約はSpawnTetromino側で出現決定と同時に済ませてある)
        }

        double tland = (cy - SPAWN_PIVOT_Y) / FALL_SPEED;
        if (tland < 1.0) tland = 1.0;

        // 1セルにつき3x3の9発クラスタを配置し、密度と塊感を出す
        static const double CLUSTER_OFS[9][2] = {
            {-14,-14}, {0,-14}, {14,-14},
            {-14,  0}, {0,  0}, {14,  0},
            {-14, 14}, {0, 14}, {14, 14},
        };

        for (int i = 0; i < 4; i++) {
            for (int k = 0; k < 9; k++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->kind = img_enemyShotMediumBall[color];
                pShot->speed = FALL_SPEED;
                pShot->muki = 0.0;

                pShot->param_d[0] = tland;                                   // 着地までの経過フレーム数
                pShot->param_d[1] = cx;                                      // 回転軸 x(不変)
                pShot->param_d[2] = SPAWN_PIVOT_Y;                           // 出現時の軸 y
                pShot->param_d[3] = cy;                                      // 着地時の軸 y
                pShot->param_d[4] = (finalX[i] - cx) + CLUSTER_OFS[k][0];    // 軸からのローカルオフセットx
                pShot->param_d[5] = (finalY[i] - cy) + CLUSTER_OFS[k][1];    // 軸からのローカルオフセットy
                pShot->param_d[6] = -1.0;                                    // バースト開始フレーム(-1=未発火)
                pShot->param_i[0] = col[i];                                  // 所属する盤面列
                pShot->param_i[1] = row[i];                                  // 所属する盤面行
                pShot->param_i[2] = 0;                                       // バースト種別

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }

        pSet->param_i[0] = 0; // 0=未着地ロック, 1=着地ロック済み
        pSet->param_i[2] = 0; // 0=盤面未登録, 99=登録済み(二重登録防止)
    }

    // 毎フレームの位置更新(全弾、自身のcountのみから位置を再計算する)
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_d[6] >= 0.0) {
            // バースト中
            double elapsed = (double)pShot->count - pShot->param_d[6];
            double landedX = pShot->param_d[1] + pShot->param_d[4];
            double landedY = pShot->param_d[3] + pShot->param_d[5];
            double muki;
            if (pShot->param_i[2] == 1) {
                muki = (landedX < BoardCenterX()) ? DX_PI : 0.0; // ライン消去:中心から左右へ
            }
            else {
                muki = atan2(landedY - BoardCenterY(), landedX - BoardCenterX()); // トップアウト:放射状
            }
            pShot->muki = muki;
            pShot->x = landedX + BURST_SPEED * cos(muki) * elapsed;
            pShot->y = landedY + BURST_SPEED * sin(muki) * elapsed;
        }
        else {
            double tland = pShot->param_d[0];
            double elapsed = (double)pShot->count;
            double frac = (elapsed < tland) ? elapsed : tland;
            double pivotY = pShot->param_d[2] + FALL_SPEED * frac;
            double angle = (tland - frac) * SPIN_RATE; // 着地時にちょうど0radへ収束する回転

            double lox = pShot->param_d[4];
            double loy = pShot->param_d[5];
            double rdx = lox * cos(angle) - loy * sin(angle);
            double rdy = lox * sin(angle) + loy * cos(angle);

            pShot->x = pShot->param_d[1] + rdx;
            pShot->y = pivotY + rdy;
            pShot->muki = angle;

            // 着地イベント検出(このShotSetでまだロックしていない場合のみ、代表1セルを登録)
            if (pSet->param_i[0] == 0 && elapsed >= tland) {
                pSet->param_i[0] = 1;
                int c = pShot->param_i[0];
                int r = pShot->param_i[1];
                if (r >= 0 && r < NUM_ROWS) {
                    g_boardFilled[c][r] = true;
                    g_boardOwner[c][r] = pSet;
                    if (r + 1 > g_colHeight[c]) g_colHeight[c] = r + 1;
                }
            }
        }
        pShot = pShot->next;
    }

    // ロック直後、このShotSetが持つ残り3セルもまとめて盤面へ登録し、ライン消去判定を行う
    if (pSet->param_i[0] == 1 && pSet->param_i[2] != 99) {
        pSet->param_i[2] = 99;
        sEnemyShot* p2 = pSet->pEnemyShotHead->next;
        while (p2 != pSet->pEnemyShotHead) {
            int c = p2->param_i[0];
            int r = p2->param_i[1];
            if (r >= 0 && r < NUM_ROWS && !g_boardFilled[c][r]) {
                g_boardFilled[c][r] = true;
                g_boardOwner[c][r] = pSet;
                if (r + 1 > g_colHeight[c]) g_colHeight[c] = r + 1;
            }
            p2 = p2->next;
        }
        CheckAndClearLines();
    }
}

static void SpawnTetromino()
{
    int shape = GetRand(6); // 0〜6の7種
    int width = SHAPE_WIDTH[shape];

    // 出現列の候補を3つ試し、盤面が最も平らになる(＝着地行が最も低い)ものを選ぶ。
    // これにより偏った積み上がりで盤面が早期に手詰まりするのを防ぎ、
    // ライン消去が自然に発生しやすくなる。
    int bestStartCol = 0;
    int bestBaseRow = NUM_ROWS + 999;
    for (int trial = 0; trial < 3; trial++) {
        int candCol = GetRand(NUM_COLS - width);
        int candBaseRow = ComputePieceBaseRow(shape, candCol);
        if (candBaseRow < bestBaseRow) {
            bestBaseRow = candBaseRow;
            bestStartCol = candCol;
        }
    }

    if (bestBaseRow >= NUM_ROWS) return; // どの候補も本当に置き場がない

    int startCol = bestStartCol;
    int pieceBaseRow = bestBaseRow;

    // 着地予定マスを即座に予約し、以後の出現ピースとの重なりを防ぐ
    const ShapeCell* cells = SHAPE_TABLE[shape];
    for (int i = 0; i < 4; i++) {
        int c = startCol + cells[i].c;
        int r = pieceBaseRow - cells[i].r;
        if (r + 1 > g_reservedHeight[c]) g_reservedHeight[c] = r + 1;
    }

    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotTetromino;
    pSet->x = ColToX(startCol);
    pSet->y = SPAWN_PIVOT_Y;
    pSet->muki = 0.0;
    pSet->param_i[0] = shape;
    pSet->param_i[1] = startCol;
    pSet->param_i[3] = pieceBaseRow;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// 積み木が天井へ到達したら全ブロックを盤面中心から放射状に崩壊させ、盤面をリセットする
static void CheckTopOut(int nowCount)
{
    bool topped = false;
    for (int c = 0; c < NUM_COLS; c++) {
        if (g_colHeight[c] >= NUM_ROWS) { topped = true; break; }
    }
    if (!topped) return;

    for (int c = 0; c < NUM_COLS; c++) {
        for (int r = 0; r < NUM_ROWS; r++) {
            if (g_boardFilled[c][r]) {
                TriggerCellBurst(c, r, 2, img_enemyShotMediumBall[6]);
            }
            g_boardFilled[c][r] = false;
            g_boardOwner[c][r] = nullptr;
        }
        g_colHeight[c] = 0;
        g_reservedHeight[c] = 0;
    }

    SpawnAimedFan(enemy.x, enemy.y, 5, 70.0 * DX_PI / 180.0);
    g_spawnCooldownUntil = nowCount + 120; // 崩壊演出が捌けるまで次の出現を止める
}

static void TopLine(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        for (int i = -2; i <= 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = 240 + 64 * i;
            pEnemyShot->y = 110;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[0];

            pEnemyShot->prev = pSet->pEnemyShotHead->prev;
            pEnemyShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pEnemyShot;
            pSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
}

// ============================================================
//  敵本体
// ============================================================
void EnemyPat_TheMostFun_Claude() // 実プロジェクトへ組み込む際はこの関数名のまま使用する
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        for (int c = 0; c < NUM_COLS; c++) {
            g_colHeight[c] = 0;
            g_reservedHeight[c] = 0;
            for (int r = 0; r < NUM_ROWS; r++) {
                g_boardFilled[c][r] = false;
                g_boardOwner[c][r] = nullptr;
            }
        }
        g_spawnCooldownUntil = 0;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = TopLine;
        
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 盤面上部をゆっくり左右に漂う(位置はcountの式から直接算出)
    enemy.x = 240.0 + 70.0 * sin((double)count * 0.008);

    CheckTopOut(count);

    if (count >= g_spawnCooldownUntil && count % 55 == 1) {
        SpawnTetromino();
    }
}