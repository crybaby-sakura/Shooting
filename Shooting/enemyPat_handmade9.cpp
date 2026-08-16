// enemyPat_othello.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================================
// 定数・状態保持用の静的変数
// ============================================================================
static const int STEP_INTERVAL = 60;  // 1手あたりのフレーム数 (60FPSで1秒)
static const double BOARD_OFFSET_X = 72.0; // a1の座標 (72.0, 60.0)
static const double BOARD_OFFSET_Y = 60.0;
static const double GRID_SIZE = 48.0; // 石と石の間隔 (48ピクセル)

static int board_color[8][8];         // 0:無, 1:黒(赤), 2:白(シアン)
static sEnemyShot* board_shot[8][8];  // 盤面の石（大玉弾）へのポインタ
static sEnemyShotSet* pBoardSet = nullptr;

static int kifu_index = 0;
// 読み込む棋譜データ (全60手・120文字)
static const char kifu[] = "e6f4c3d6f5c6c4d3c5b6b5e7f6g5d2g6e3f3f7d7d8f8c7c8e8e2a5b4g4g3f1c1e1c2f2d1g2a6b7a4a7a8b8h1h7g1h3h6h4h5g7h2b2b3b1a1a3a2g8h8";


// ============================================================================
// サブ弾幕パターン関数群
// ============================================================================

// 盤面の石およびマス目（レーザー）をその場に留まらせるパターン（不動）
static void ShotBoard(sEnemyShotSet* pSet) {
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 打った石から全方位に中玉を撃つパターン
static void ShotOmni(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        int color = pSet->kind; // 0(赤) or 3(シアン)
        int img = img_enemyShotMediumBall[color];

        for (int i = 0; i < 36; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = i * DX_PI * 2 / 36.0;
            pShot->speed = 3.0;
            pShot->kind = img;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ひっくり返った石から菱形弾をばら撒くパターン
static void ShotScatterDiamond(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        int color = pSet->kind; // 0(赤) or 3(シアン)
        int img = img_enemyShotDiamond[color];

        for (int i = 0; i < 16; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;

            pShot->muki = GetRand(359) * DX_PI / 180.0;
            pShot->speed = (100 + GetRand(200)) / 100.0; // 速度1.0～3.0
            pShot->kind = img;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// ============================================================================
// オセロのロジックおよび盤面生成ヘルパー関数
// ============================================================================

// 盤面に石（大玉）を配置する
static void AddStoneToBoard(int r, int c, int color) {
    board_color[r][c] = color;
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = BOARD_OFFSET_X + c * GRID_SIZE;
    pShot->y = BOARD_OFFSET_Y + r * GRID_SIZE;
    pShot->speed = 0.0;
    pShot->muki = 0.0;

    // 1(黒) -> 0(赤), 2(白) -> 3(シアン)
    pShot->kind = img_enemyShotLargeBall[color == 1 ? 0 : 3];

    pShot->prev = pBoardSet->pEnemyShotHead->prev;
    pShot->next = pBoardSet->pEnemyShotHead;
    pBoardSet->pEnemyShotHead->prev->next = pShot;
    pBoardSet->pEnemyShotHead->prev = pShot;

    board_shot[r][c] = pShot;
}

// 白色短レーザーを使って盤面のグリッド線を生成する
static void CreateBoardGrid() {
    int laserImg = img_enemyShotLaser[6]; // 6:白
    double laserLen = 128.0;
    double center[] = {
        -GRID_SIZE * 0.5 + laserLen / 2,
        -GRID_SIZE * 0.5 + laserLen / 2 + 80,
        GRID_SIZE * 7.5 - laserLen / 2 - 80,
        GRID_SIZE * 7.5 - laserLen / 2
    };

    // 縦9本、横9本のグリッド線を短レーザーを敷き詰めて作成
    for (int i = 0; i <= 8; i++) {
        double linePos = (-GRID_SIZE / 2.0) + i * GRID_SIZE; // 48.0 + i*48.0

        for (int k = 0; k < 4; k++) {
            double segPos = center[k]; // 長さ64のセグメントを6連配置

            // 横線
            sEnemyShot* pLaserH = new sEnemyShot;
            pLaserH->x = BOARD_OFFSET_X + segPos;
            pLaserH->y = BOARD_OFFSET_Y + linePos;
            pLaserH->muki = 0.0;
            pLaserH->speed = 0.0;
            pLaserH->kind = laserImg;
            pLaserH->prev = pBoardSet->pEnemyShotHead->prev;
            pLaserH->next = pBoardSet->pEnemyShotHead;
            pBoardSet->pEnemyShotHead->prev->next = pLaserH;
            pBoardSet->pEnemyShotHead->prev = pLaserH;

            // 縦線
            sEnemyShot* pLaserV = new sEnemyShot;
            pLaserV->x = BOARD_OFFSET_X + linePos;
            pLaserV->y = BOARD_OFFSET_Y + segPos;
            pLaserV->muki = DX_PI / 2.0; // 90度回転
            pLaserV->speed = 0.0;
            pLaserV->kind = laserImg;
            pLaserV->prev = pBoardSet->pEnemyShotHead->prev;
            pLaserV->next = pBoardSet->pEnemyShotHead;
            pBoardSet->pEnemyShotHead->prev->next = pLaserV;
            pBoardSet->pEnemyShotHead->prev = pLaserV;
        }
    }
}

// 指定方向のひっくり返せる石の数を取得する
static int get_flip_count(int r, int c, int dr, int dc, int color) {
    int opp = (color == 1) ? 2 : 1;
    int nr = r + dr;
    int nc = c + dc;
    int flip_count = 0;

    while (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
        if (board_color[nr][nc] == opp) {
            flip_count++;
        }
        else if (board_color[nr][nc] == color) {
            return flip_count;
        }
        else {
            return 0;
        }
        nr += dr;
        nc += dc;
    }
    return 0;
}

// 弾幕セットを生成して発射（全方位）
static void FireOmni(int r, int c, int color) {
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotOmni;
    pSet->x = BOARD_OFFSET_X + c * GRID_SIZE;
    pSet->y = BOARD_OFFSET_Y + r * GRID_SIZE;
    pSet->kind = (color == 1) ? 0 : 3;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// 弾幕セットを生成して発射（ばら撒き）
static void FireDiamond(int r, int c, int color) {
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotScatterDiamond;
    pSet->x = BOARD_OFFSET_X + c * GRID_SIZE;
    pSet->y = BOARD_OFFSET_Y + r * GRID_SIZE;
    pSet->kind = (color == 1) ? 0 : 3;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}


// ============================================================================
// メインの敵パターン関数
// ============================================================================
void EnemyPat_Othello()
{
    // 初期化処理（1フレーム目）
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 30.0;

        // -------------------------------------------------------------------
        // HPの計算見積もり:
        // 60手 × 60フレーム/手 = 3,600フレーム（全再生時間）
        // 6フレームごとに1ダメージ受ける仕様のため:
        // 3600 / 6 = 600 ダメージで全手打ち終わると同時に倒し切れます。
        // -------------------------------------------------------------------
        enemy.maxHp = enemy.hp = 400;

        // 演出：画面下に自機を配置
        player.x = 240.0;
        player.y = 450.0;

        kifu_index = 0;

        // 盤面データ初期化
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                board_color[i][j] = 0;
                board_shot[i][j] = nullptr;
            }
        }

        // 盤面の石・格子線を保持する永続的なShotSetを作成
        pBoardSet = new sEnemyShotSet;
        pBoardSet->count = 0;
        pBoardSet->patternFunc = ShotBoard;
        pBoardSet->x = 240.0;
        pBoardSet->y = 240.0;
        pBoardSet->muki = 0.0;

        pBoardSet->pEnemyShotHead = new sEnemyShot;
        pBoardSet->pEnemyShotHead->prev = pBoardSet->pEnemyShotHead;
        pBoardSet->pEnemyShotHead->next = pBoardSet->pEnemyShotHead;

        pBoardSet->prev = enemyShotSetHead.prev;
        pBoardSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBoardSet;
        enemyShotSetHead.prev = pBoardSet;

        // 1. 白色の不動レーザーでグリッド線を描画
        CreateBoardGrid();

        // 2. オセロ初期配置の4石をセット
        AddStoneToBoard(3, 3, 2); // d4: 白(シアン)
        AddStoneToBoard(3, 4, 1); // e4: 黒(赤)
        AddStoneToBoard(4, 3, 1); // d5: 黒(赤)
        AddStoneToBoard(4, 4, 2); // e5: 白(シアン)
    }

    // STEP_INTERVAL (60フレーム=1秒) ごとに棋譜を1手進める
    if (count > 0 && count % STEP_INTERVAL == 0) {
        if (kifu_index < 120) {
            int c = kifu[kifu_index] - 'a';     // 列 (0～7)
            int r = kifu[kifu_index + 1] - '1'; // 行 (0～7)
            kifu_index += 2;

            // 奇数手目が黒(赤=1)、偶数手目が白(シアン=2)
            int turn_color = ((kifu_index / 2) % 2 == 1) ? 1 : 2;

            // 1. 石を置く（大玉）
            AddStoneToBoard(r, c, turn_color);

            // 2. 打った石から中玉の全方位弾を発射
            FireOmni(r, c, turn_color);

            // 3. ひっくり返し処理と菱形弾のばら撒き
            int dr[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
            int dc[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
            bool flipped = false;

            for (int i = 0; i < 8; i++) {
                int flip = get_flip_count(r, c, dr[i], dc[i], turn_color);
                for (int j = 1; j <= flip; j++) {
                    int fr = r + dr[i] * j;
                    int fc = c + dc[i] * j;

                    // 石の色と大玉弾の画像を更新
                    board_color[fr][fc] = turn_color;
                    if (board_shot[fr][fc]) {
                        board_shot[fr][fc]->kind = img_enemyShotLargeBall[turn_color == 1 ? 0 : 3];
                    }

                    // 裏返った位置から菱形弾を散布
                    FireDiamond(fr, fc, turn_color);
                    flipped = true;
                }
            }

            // 4. 効果音の再生
            if (flipped) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
            else {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
        }
    }
}