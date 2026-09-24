// enemyPat_Tmp.cpp - 速度0の格子迷路ステージ

static void ShotMazeWall(sEnemyShotSet* pEnemyShotSet)
{
    // 速度0の壁なので移動処理なし
    // count のインクリメントはメイン側で行われる

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double x0 = pShot->param_d[0];
        double y0 = pShot->param_d[1];
        int ix = pShot->param_i[0];
        int iy = pShot->param_i[1];
        if (ix != -1) {
            double arg = ((ix + iy) % 3) * DX_PI * 2.0 / 3.0;
            double amp = 15.0 * sin(pEnemyShotSet->count * 0.05 + arg);
            pShot->x = x0 + amp * cos(pShot->muki);
            pShot->y = y0 + amp * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

static void ShotBossAttack(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            // 全周 + 少しばらつき
            pShot->muki = pEnemyShotSet->muki + (DX_PI * 2.0 / 12.0) * i + (GetRand(20) - 10) / 180.0 * DX_PI;
            pShot->speed = 2.2 + GetRand(80) / 100.0;
            pShot->count = 0;
            pShot->margin = 30.0;
            // 中玉 赤 7.0x7.0
            pShot->kind = img_enemyShotMediumBall[0];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

void EnemyPat_FixedMaze_MetaAI()
{
    const int MAZE_W = 8;
    const int MAZE_H = 8;
    const double CELL = 60.0; // 480 / 8 = 60 レーザー64とほぼ一致

    static sEnemyShotSet* pMazeSet = nullptr;
    static bool unlocked = false;

    if (count == 1) {
        // 初期位置固定 左上 / 右下
        player.x = CELL / 2.0; // 30
        player.y = CELL / 2.0; // 30
        enemy.x = 240; // 450
        enemy.y = 50; // 450
        enemy.maxHp = enemy.hp = 50;
        unlocked = false;

        // 迷路用 ShotSet 作成
        pMazeSet = new sEnemyShotSet;
        pMazeSet->count = 0;
        pMazeSet->patternFunc = ShotMazeWall;
        pMazeSet->x = 0; pMazeSet->y = 0; pMazeSet->muki = 0; pMazeSet->kind = 0;
        pMazeSet->pEnemyShotHead = new sEnemyShot;
        pMazeSet->pEnemyShotHead->prev = pMazeSet->pEnemyShotHead;
        pMazeSet->pEnemyShotHead->next = pMazeSet->pEnemyShotHead;

        pMazeSet->prev = enemyShotSetHead.prev;
        pMazeSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMazeSet;
        enemyShotSetHead.prev = pMazeSet;

        // --- 迷路生成 (Recursive Backtracker) ---
        bool visited[MAZE_H][MAZE_W] = {};
        bool vWall[MAZE_H][MAZE_W + 1]; // 垂直壁 [y][x]
        bool hWall[MAZE_H + 1][MAZE_W]; // 水平壁 [y][x]

        for (int y = 0; y < MAZE_H; y++)
            for (int x = 0; x <= MAZE_W; x++) vWall[y][x] = true;
        for (int y = 0; y <= MAZE_H; y++)
            for (int x = 0; x < MAZE_W; x++) hWall[y][x] = true;

        visited[0][0] = true;
        std::vector<std::pair<int, int>> st;
        st.emplace_back(0, 0);

        while (!st.empty()) {
            int cy = st.back().first;
            int cx = st.back().second;

            std::vector<std::pair<int, int>> nbrs;
            if (cy > 0 && !visited[cy - 1][cx]) nbrs.emplace_back(cy - 1, cx);
            if (cy < MAZE_H - 1 && !visited[cy + 1][cx]) nbrs.emplace_back(cy + 1, cx);
            if (cx > 0 && !visited[cy][cx - 1]) nbrs.emplace_back(cy, cx - 1);
            if (cx < MAZE_W - 1 && !visited[cy][cx + 1]) nbrs.emplace_back(cy, cx + 1);

            if (nbrs.empty()) {
                st.pop_back();
            }
            else {
                int idx = GetRand((int)nbrs.size() - 1); // 0..n-1
                auto [ny, nx] = nbrs[idx];
                // 壁を壊す
                if (ny == cy - 1) hWall[cy][cx] = false; // 上
                else if (ny == cy + 1) hWall[cy + 1][cx] = false; // 下
                else if (nx == cx - 1) vWall[cy][cx] = false; // 左
                else if (nx == cx + 1) vWall[cy][cx + 1] = false; // 右

                visited[ny][nx] = true;
                st.emplace_back(ny, nx);
            }
        }

        // 迷路を弾として配置
        auto AddWallBullet = [&](double x, double y, double muki, int imgKind, int ix, int iy) {
            sEnemyShot* p = new sEnemyShot;
            p->x = p->param_d[0] = x; p->y = p->param_d[1] = y;
            p->muki = muki;
            p->speed = 0.0; // 速度0で固定
            p->count = 0;
            p->kind = imgKind;
            p->margin = 100.0; // 外周で消えないように大きめ
            p->param_i[0] = ix;
            p->param_i[1] = iy;
            p->prev = pMazeSet->pEnemyShotHead->prev;
            p->next = pMazeSet->pEnemyShotHead;
            pMazeSet->pEnemyShotHead->prev->next = p;
            pMazeSet->pEnemyShotHead->prev = p;
        };

        // 垂直壁: レーザー白 64x4 縦向き
        for (int y = 0; y < MAZE_H; y++) {
            for (int x = 0; x <= MAZE_W; x++) {
                if (vWall[y][x]) {
                    double px = x * CELL;
                    double py = y * CELL + CELL / 2.0;
                    AddWallBullet(px, py, DX_PI / 2.0, img_enemyShotLaser[6], x, y);
                }
            }
        }
        // 水平壁: レーザー白 横向き
        for (int y = 0; y <= MAZE_H; y++) {
            for (int x = 0; x < MAZE_W; x++) {
                if (hWall[y][x]) {
                    double px = x * CELL + CELL / 2.0;
                    double py = y * CELL;
                    AddWallBullet(px, py, 0.0, img_enemyShotLaser[6], x, y);
                }
            }
        }
        // 交点の柱: 中玉白 7x7で隙間を塞ぐ
        for (int y = 0; y <= MAZE_H; y++) {
            for (int x = 0; x <= MAZE_W; x++) {
                bool has = false;
                if (y < MAZE_H && x <= MAZE_W && vWall[y][x]) has = true;
                if (y > 0 && x <= MAZE_W && vWall[y - 1][x]) has = true;
                if (x < MAZE_W && y <= MAZE_H && hWall[y][x]) has = true;
                if (x > 0 && y <= MAZE_H && hWall[y][x - 1]) has = true;
                if (has) {
                    double px = x * CELL;
                    double py = y * CELL;
                    // 外周の角は重なるので1つだけ
                    AddWallBullet(px, py, 0.0, img_enemyShotMediumBall[6], -1, -1);
                }
            }
        }
    }
    else {
        // 敵位置は固定
        //enemy.x = 480.0 - CELL / 2.0;
        //enemy.y = 480.0 - CELL / 2.0;

        double dx = player.x - (480.0 - CELL / 2.0);
        double dy = player.y - (480.0 - CELL / 2.0);
        double dist = sqrt(dx * dx + dy * dy);

        if (!unlocked) {
            // ボスエリア到達判定
            if (dist < 75.0) {
                unlocked = true;
                // 迷路全消去
                if (pMazeSet) {
                    sEnemyShot* p = pMazeSet->pEnemyShotHead->next;
                    while (p != pMazeSet->pEnemyShotHead) {
                        sEnemyShot* nxt = p->next;
                        p->prev->next = p->next;
                        p->next->prev = p->prev;
                        delete p;
                        p = nxt;
                    }
                    pMazeSet->pEnemyShotHead->next = pMazeSet->pEnemyShotHead;
                    pMazeSet->pEnemyShotHead->prev = pMazeSet->pEnemyShotHead;
                }
                if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            }
            else {
                // 迷路抜けるまで無敵
                enemy.hp = enemy.maxHp;
            }
        }

        // 迷路クリア後は通常弾幕
        if (unlocked && count % 30 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotBossAttack;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}