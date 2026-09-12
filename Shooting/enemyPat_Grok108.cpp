// enemyPat_Tmp.cpp
// 速度0の敵弾でステージ全体を覆う格子状の迷路を作り、
// 自機が迷路を抜けないとボスに攻撃できないステージ。
// 自機初期位置: 左上、敵機位置: 右下で固定。

// 迷路の壁を形成する静止弾（speed = 0）の配置パターン
// 格子状に配置し、通路を残して迷路にする。
// 弾の位置は一度置いたら動かない。
static void ShotMazeWall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // count == 0 のときだけ弾を生成する（以降は静止のまま）
    if (pEnemyShotSet->count == 0) {
        // 予告音を軽く鳴らす（迷路生成時のみ）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 画面サイズ 480x480
        // 格子間隔（セルサイズ）。自機が通れる隙間を確保するため適度に大きくする。
        const double cell = 40.0;
        // 壁の厚みを表現するため、格子線上に中玉を並べる
        // 通路幅は約 cell - 弾半径*2 程度になる想定

        // 迷路の通路パターンを定義する（簡易的な迷路）
        // 0: 壁なし（通路）、1: 壁あり
        // 縦横の格子線を選択的に消して通路を作る
        // 行数・列数は 480/cell ≈ 12 程度
        const int GRID_N = 12; // 0〜12 の格子線

        // 水平方向の壁（横線）を置くかどうかのフラグ
        // hWall[y][x] : y行目の横壁のうち、x番目のセグメント
        // 垂直方向の壁（縦線）
        // vWall[y][x]

        // 固定パターンで左上から右下へ一本道＋分岐のある迷路を作る
        // 簡易迷路生成: 基本は全部壁にして、必要な通路を開ける

        // まず全格子線を壁候補として弾を置き、通路にしたい箇所はスキップする
        // 通路設計（0-indexed のセル座標で考える）
        // プレイヤー開始: 左上セル付近 (0,0)
        // ボス: 右下 (11,11) 付近

        // 横壁を置くかどうか（各水平線上の各セグメント）
        // true = 壁を置く
        bool hWall[GRID_N + 1][GRID_N];
        bool vWall[GRID_N][GRID_N + 1];

        // 初期化: すべて壁
        for (int y = 0; y <= GRID_N; ++y) {
            for (int x = 0; x < GRID_N; ++x) {
                hWall[y][x] = true;
            }
        }
        for (int y = 0; y < GRID_N; ++y) {
            for (int x = 0; x <= GRID_N; ++x) {
                vWall[y][x] = true;
            }
        }

        // 外周は必ず壁（画面端を閉じる）
        // 内側に通路を切り出す
        // 左上スタート → 右へ → 下 → 左 → 下 → ... とジグザグ＋分岐

        // 通路を開けるヘルパー（横壁を消す / 縦壁を消す）
        auto openH = [&](int y, int x) {
            if (y >= 0 && y <= GRID_N && x >= 0 && x < GRID_N)
                hWall[y][x] = false;
        };
        auto openV = [&](int y, int x) {
            if (y >= 0 && y < GRID_N && x >= 0 && x <= GRID_N)
                vWall[y][x] = false;
        };

        // ===== メイン通路（左上 → 右下） =====
        // 上辺を一部開ける（スタート付近）
        openH(0, 0); openH(0, 1);

        // 1行目を右へ
        openV(0, 1); openV(0, 2); openV(0, 3); openV(0, 4);
        openH(1, 0); openH(1, 1); openH(1, 2); openH(1, 3);

        // 下へ降りる
        openV(1, 4);
        openH(2, 3); openH(2, 4);

        // 左へ
        openV(2, 3); openV(2, 2); openV(2, 1);
        openH(3, 0); openH(3, 1); openH(3, 2);

        // 下へ
        openV(3, 0);
        openH(4, 0); openH(4, 1);

        // 右へ大きく進む
        openV(4, 1); openV(4, 2); openV(4, 3); openV(4, 4); openV(4, 5); openV(4, 6);
        openH(5, 2); openH(5, 3); openH(5, 4); openH(5, 5);

        // 下へ
        openV(5, 6);
        openH(6, 5); openH(6, 6);

        // 左へ
        openV(6, 5); openV(6, 4); openV(6, 3);
        openH(7, 3); openH(7, 4);

        // 下へ
        openV(7, 3);
        openH(8, 2); openH(8, 3);

        // 右へ
        openV(8, 3); openV(8, 4); openV(8, 5); openV(8, 6); openV(8, 7);
        openH(9, 4); openH(9, 5); openH(9, 6);

        // 下へ
        openV(9, 7);
        openH(10, 6); openH(10, 7);

        // 右下へ最終接近
        openV(10, 8); openV(10, 9); openV(10, 10);
        openH(11, 7); openH(11, 8); openH(11, 9); openH(11, 10);

        // ボス周辺を少し開ける（攻撃できるように）
        openV(11, 11);
        openH(12, 10); openH(12, 11);

        // 追加の分岐・抜け道（少し探索要素を付ける）
        openH(2, 6); openV(1, 6); openV(2, 6);
        openH(4, 8); openV(3, 8); openV(4, 8);
        openH(6, 1); openV(5, 1); openV(6, 1);
        openH(8, 9); openV(7, 9); openV(8, 9);
        openH(9, 1); openV(8, 1); openV(9, 1);

        // ===== 実際に弾を配置 =====
        // 弾種: 中玉（視認性が良く、隙間が分かりやすい）
        // 色: 青寄り（4）やシアン（3）で迷路感を出す。一部白でアクセント。
        const int colorBase = 4; // 青

        // 水平壁（横線）
        for (int gy = 0; gy <= GRID_N; ++gy) {
            for (int gx = 0; gx < GRID_N; ++gx) {
                if (!hWall[gy][gx]) continue;

                // 1セグメントに複数弾を並べて壁の厚みを出す
                for (int k = 0; k < 3; ++k) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = (gx + 0.5) * cell + (k - 1) * 12.0;
                    pEnemyShot->y = gy * cell;
                    pEnemyShot->muki = 0.0;
                    pEnemyShot->speed = 0.0;          // 静止
                    pEnemyShot->kind = img_enemyShotMediumBall[colorBase];
                    pEnemyShot->margin = 30.0;        // 画面端でも消えにくく

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }

        // 垂直壁（縦線）
        for (int gy = 0; gy < GRID_N; ++gy) {
            for (int gx = 0; gx <= GRID_N; ++gx) {
                if (!vWall[gy][gx]) continue;

                for (int k = 0; k < 3; ++k) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = gx * cell;
                    pEnemyShot->y = (gy + 0.5) * cell + (k - 1) * 12.0;
                    pEnemyShot->muki = 0.0;
                    pEnemyShot->speed = 0.0;
                    pEnemyShot->kind = img_enemyShotMediumBall[(colorBase + 1) % 9]; // 少し色違い
                    pEnemyShot->margin = 30.0;

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }

        // 交差点を強調するため、角にも小玉を追加（視認性向上）
        for (int gy = 0; gy <= GRID_N; ++gy) {
            for (int gx = 0; gx <= GRID_N; ++gx) {
                // 周囲に壁がある交差点のみ
                bool hasWall = false;
                if (gx < GRID_N && hWall[gy][gx]) hasWall = true;
                if (gx > 0 && hWall[gy][gx - 1]) hasWall = true;
                if (gy < GRID_N && vWall[gy][gx]) hasWall = true;
                if (gy > 0 && vWall[gy - 1][gx]) hasWall = true;
                if (!hasWall) continue;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = gx * cell;
                pEnemyShot->y = gy * cell;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
                pEnemyShot->margin = 30.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // speed == 0 なので移動処理は不要。
    // ただし仕様上、他の弾と同様にループは残しておく（将来の拡張用）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 静止弾なので位置更新なし
        // pShot->x += pShot->speed * cos(pShot->muki);
        // pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ボスからの軽い牽制弾（8方向ばら撒き）
static void ShotBossHarass(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 8方向に低速弾
        for (int i = 0; i < 8; ++i) {
            pEnemyShot = new sEnemyShot;
            double ang = pEnemyShotSet->muki + i * (DX_PI * 2.0 / 8.0);
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 1.2 + (i % 3) * 0.3;
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤
            pEnemyShot->margin = 20.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ボス本体のパターン
// 右下に固定。迷路を抜けてきた自機にのみ攻撃可能になるよう、
// 弾幕は控えめにし、主に迷路自体を障害物として機能させる。
void EnemyPat_FixedMaze_Grok()
{
    // 初回のみ初期化
    if (count == 1) {
        // ゲーム画面は 480x480
        // 自機は通常左上付近に出る想定なので、敵は右下に固定
        enemy.x = 420.0;
        enemy.y = 420.0;
        enemy.maxHp = enemy.hp = 100; // 200で固定
        player.x = 20;
        player.y = 20;

        // 迷路を一度だけ生成する ShotSet を登録
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMazeWall;
        pEnemyShotSet->x = 0.0;
        pEnemyShotSet->y = 0.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 敵本体は完全固定（動かない）
    // 迷路を抜けて近づいてきた自機に対して、軽い牽制弾を時々撃つ
    // （迷路自体が主障害なので弾幕は控えめ）
    if (count > 120 && count % 90 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBossHarass;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}