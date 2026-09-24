// enemyPat_FuuinKairou.cpp
//
// 「迷路を抜けるまでボスに攻撃できない」ステージ。
//
// 概要:
//   ・速度0の弾で画面全体(8x8グリッド)を覆う格子状の迷路を構築する。
//   ・迷路は再帰的バックトラッカー法(穴掘り法)で生成し、GetRandのみを
//     使うのでリプレイ再現性がある。全マスが繋がる一本道の迷路になるため、
//     自機の初期位置(左上)からボスの部屋(右下)へ必ず到達できる。
//   ・自機がボスの部屋へ到達するまでは、ボスのHPを毎フレーム全回復させる
//     ことで実質的な無敵状態にし、「迷路を抜けないと攻撃が通らない」を表現する。
//   ・到達した瞬間に無敵を解除し、迷路の壁は各弾が保持していた
//     「画面中心から見た向き」に沿って加速しながら放射状に飛散して消滅する
//     (=封印解除演出)。以降は通常のボス戦として自機狙い3wayを周期発射する。
//
// 使用素材:
//   ・弾種: 中玉(7.0x7.0) … 迷路の壁。密着間隔で並べて隙間のない壁にするため
//     小玉より粗いピッチで足りる中玉を採用。銃弾は自機狙い3wayの見た目に使用。
//   ・色: 外周は白(6)で「画面の境界」を、内壁はシアン(3)で「迷路本体」を
//     視覚的に区別。自機狙い弾は赤(0)。
//   ・効果音: 封印解除の瞬間に sound_enemyShot_extreme、通常弾幕には
//     sound_enemyShot_medium を使用。


static const int    MAZE_GRID_N = 8;                    // 迷路の縦横マス数
static const double MAZE_CELL = 480.0 / MAZE_GRID_N;   // 1マスのサイズ(=60.0)
static const double MAZE_CENTER_X = 240.0;                 // 放射飛散の基準点(画面中心)
static const double MAZE_CENTER_Y = 240.0;

// 迷路の壁1本(始点から終点まで)を、隙間なく密着させた弾の列として構築する。
static void SpawnMazeWallLine(sEnemyShotSet* pWallSet, double x1, double y1, double x2, double y2, int colorKind)
{
    const double pitch = 6.0; // 中玉(7.0x7.0)が重なり合って隙間ができない間隔
    double dx = x2 - x1;
    double dy = y2 - y1;
    double len = sqrt(dx * dx + dy * dy);
    int    n = (int)(len / pitch) + 1;

    for (int i = 0; i <= n; i++) {
        double t = (double)i / (double)n;
        double x = x1 + dx * t;
        double y = y1 + dy * t;

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = x;
        pShot->y = y;
        pShot->speed = 0.0; // 迷路の壁は封印解除まで静止させる
        // 画面中心から見た方向を保持しておき、封印解除時にそのまま放射飛散の向きとして使う
        pShot->muki = atan2(y - MAZE_CENTER_Y, x - MAZE_CENTER_X);
        pShot->kind = img_enemyShotMediumBall[colorKind];

        pShot->prev = pWallSet->pEnemyShotHead->prev;
        pShot->next = pWallSet->pEnemyShotHead;
        pWallSet->pEnemyShotHead->prev->next = pShot;
        pWallSet->pEnemyShotHead->prev = pShot;
    }
}

// 迷路生成(再帰的バックトラッカー/穴掘り法)。GetRandのみ使用するため再現性あり。
// wallRight[r][c] : マス(r,c)と(r,c+1)の間の壁の有無
// wallDown[r][c]  : マス(r,c)と(r+1,c)の間の壁の有無
static void CarveMaze(bool wallRight[MAZE_GRID_N][MAZE_GRID_N], bool wallDown[MAZE_GRID_N][MAZE_GRID_N])
{
    bool visited[MAZE_GRID_N][MAZE_GRID_N];
    for (int r = 0; r < MAZE_GRID_N; r++) {
        for (int c = 0; c < MAZE_GRID_N; c++) {
            visited[r][c] = false;
            wallRight[r][c] = true;
            wallDown[r][c] = true;
        }
    }

    struct Cell { int r, c; };
    Cell stack[MAZE_GRID_N * MAZE_GRID_N];
    int  sp = 0;

    visited[0][0] = true;
    stack[sp++] = { 0, 0 };

    while (sp > 0) {
        Cell cur = stack[sp - 1];

        int dirs[4];
        int dirCount = 0;
        if (cur.r > 0 && !visited[cur.r - 1][cur.c]) dirs[dirCount++] = 0; // 上
        if (cur.r < MAZE_GRID_N - 1 && !visited[cur.r + 1][cur.c]) dirs[dirCount++] = 1; // 下
        if (cur.c > 0 && !visited[cur.r][cur.c - 1]) dirs[dirCount++] = 2; // 左
        if (cur.c < MAZE_GRID_N - 1 && !visited[cur.r][cur.c + 1]) dirs[dirCount++] = 3; // 右

        if (dirCount == 0) {
            sp--; // 行き止まり: バックトラック
            continue;
        }

        // GetRand(x)は0からxまでのx+1種類を返すので、候補数-1を渡す
        int dir = dirs[GetRand(dirCount - 1)];
        int nr = cur.r, nc = cur.c;

        switch (dir) {
        case 0: nr = cur.r - 1; wallDown[nr][nc] = false; break;
        case 1: nr = cur.r + 1; wallDown[cur.r][cur.c] = false; break;
        case 2: nc = cur.c - 1; wallRight[cur.r][nc] = false; break;
        case 3: nc = cur.c + 1; wallRight[cur.r][cur.c] = false; break;
        }

        visited[nr][nc] = true;
        stack[sp++] = { nr, nc };
    }
}

// 迷路全体(外周+内壁)を弾で構築する。
static void BuildMazeWalls(sEnemyShotSet* pWallSet)
{
    bool wallRight[MAZE_GRID_N][MAZE_GRID_N];
    bool wallDown[MAZE_GRID_N][MAZE_GRID_N];
    CarveMaze(wallRight, wallDown);

    // 内壁(迷路生成で残った壁のみ): シアン(3)
    for (int r = 0; r < MAZE_GRID_N; r++) {
        for (int c = 0; c < MAZE_GRID_N - 1; c++) {
            if (wallRight[r][c]) {
                double x = (c + 1) * MAZE_CELL;
                SpawnMazeWallLine(pWallSet, x, r * MAZE_CELL, x, (r + 1) * MAZE_CELL, 3);
            }
        }
    }
    for (int r = 0; r < MAZE_GRID_N - 1; r++) {
        for (int c = 0; c < MAZE_GRID_N; c++) {
            if (wallDown[r][c]) {
                double y = (r + 1) * MAZE_CELL;
                SpawnMazeWallLine(pWallSet, c * MAZE_CELL, y, (c + 1) * MAZE_CELL, y, 3);
            }
        }
    }

    // 外周(常に壁・迷路生成の対象外): 白(6)で内壁と区別する
    for (int c = 0; c < MAZE_GRID_N; c++) {
        SpawnMazeWallLine(pWallSet, c * MAZE_CELL, 0.0, (c + 1) * MAZE_CELL, 0.0, 6); // 上端
        SpawnMazeWallLine(pWallSet, c * MAZE_CELL, 480.0, (c + 1) * MAZE_CELL, 480.0, 6); // 下端
    }
    for (int r = 0; r < MAZE_GRID_N; r++) {
        SpawnMazeWallLine(pWallSet, 0.0, r * MAZE_CELL, 0.0, (r + 1) * MAZE_CELL, 6); // 左端
        SpawnMazeWallLine(pWallSet, 480.0, r * MAZE_CELL, 480.0, (r + 1) * MAZE_CELL, 6); // 右端
    }
}

// 迷路の壁: 封印解除まで静止したまま(初回のみ迷路を構築する)。
static void ShotWallSealed(sEnemyShotSet* pWallSet)
{
    if (pWallSet->count == 0) {
        BuildMazeWalls(pWallSet);
    }
    // 速度0のため座標は不変。ここでは特に何もしない。
}

// 封印解除後: 各弾が保持していた「画面中心から見た向き」に沿って加速しながら放射状に飛散する。
static void ShotWallRelease(sEnemyShotSet* pWallSet)
{
    sEnemyShot* pShot = pWallSet->pEnemyShotHead->next;
    while (pShot != pWallSet->pEnemyShotHead) {
        pShot->speed += 0.06 / 3; // 毎フレーム加速
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 封印解除後のボス本体弾幕: 自機狙い3way
static void ShotAimed3Way(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = -1; i <= 1; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + i * (15.0 / 180.0 * DX_PI);
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotBullet[0]; // 赤い銃弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_FixedMaze_Claude()
{
    static sEnemyShotSet* pWallSet = nullptr;
    static bool           sealReleased = false;
    static int            releaseCount = 0;

    // ゴール(ボスの部屋=右下マス)判定の基準座標
    const double goalX = (MAZE_GRID_N - 1) * MAZE_CELL; // 420.0
    const double goalY = (MAZE_GRID_N - 1) * MAZE_CELL; // 420.0

    if (count == 1) {
        // 自機は左上マスの中央、ボスは右下マスの中央に固定
        player.x = MAZE_CELL * 0.5;
        player.y = MAZE_CELL * 0.5;
        enemy.x = 240;
        enemy.y = 50;
        enemy.maxHp = enemy.hp = 50; // 200で固定

        sealReleased = false;
        releaseCount = 0;

        pWallSet = new sEnemyShotSet;
        pWallSet->count = 0;
        pWallSet->patternFunc = ShotWallSealed;
        pWallSet->x = MAZE_CENTER_X;
        pWallSet->y = MAZE_CENTER_Y;

        pWallSet->pEnemyShotHead = new sEnemyShot;
        pWallSet->pEnemyShotHead->prev = pWallSet->pEnemyShotHead;
        pWallSet->pEnemyShotHead->next = pWallSet->pEnemyShotHead;

        pWallSet->prev = enemyShotSetHead.prev;
        pWallSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWallSet;
        enemyShotSetHead.prev = pWallSet;
    }

    if (!sealReleased) {
        // 自機がボスの部屋(右下マス)へ到達するまでは、ダメージを毎フレーム全回復させて実質無敵にする
        if (player.x >= goalX && player.y >= goalY) {
            sealReleased = true;
            releaseCount = count;

            if (pWallSet != nullptr) {
                pWallSet->patternFunc = ShotWallRelease; // 迷路の壁を封印解除演出として放射状に飛散させる
            }

            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK); // 封印解除音
        }
        else {
            enemy.hp = enemy.maxHp;
        }
    }
    else {
        // 封印解除後は通常のボス弾幕(自機狙い3way)を周期的に発射する
        if ((count - releaseCount) % 50 == 1) {
            sEnemyShotSet* pAimSet = new sEnemyShotSet;
            pAimSet->count = 0;
            pAimSet->patternFunc = ShotAimed3Way;
            pAimSet->x = enemy.x;
            pAimSet->y = enemy.y;

            pAimSet->pEnemyShotHead = new sEnemyShot;
            pAimSet->pEnemyShotHead->prev = pAimSet->pEnemyShotHead;
            pAimSet->pEnemyShotHead->next = pAimSet->pEnemyShotHead;

            pAimSet->prev = enemyShotSetHead.prev;
            pAimSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pAimSet;
            enemyShotSetHead.prev = pAimSet;
        }
    }
}