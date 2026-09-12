// enemyPat_Tmp.cpp
//
// 弾幕：迷宮攻略
// 速度0の敵弾を格子状に配置して迷路を作り、
// 左上から右下まで迷路を抜けるまでボスを無敵状態にする。

namespace
{
    constexpr int MAZE_W = 23;
    constexpr int MAZE_H = 23;
    constexpr double MAZE_ORIGIN_X = 10.0;
    constexpr double MAZE_ORIGIN_Y = 10.0;
    constexpr double MAZE_CELL = 20.0;

    // '#' が壁、'.' が通路。
    // 左上の (1,1) から右下の (21,21) まで一続きの通路になっている。
    static const char* const maze[MAZE_H] =
    {
        "#######################",
        "#...#...........#.....#",
        "###.#####.###.###.###.#",
        "#.#.#...#...#...#...#.#",
        "#.#.#.#.#####.#.###.#.#",
        "#.#...#.....#.#.....#.#",
        "#.#########.#.#######.#",
        "#.........#.#...#...#.#",
        "#.#.#######.###.#.###.#",
        "#.#.......#.#.....#...#",
        "#.#######.#.#####.#.###",
        "#.#...#...#.....#.#...#",
        "#.###.#.#.#####.#.###.#",
        "#.....#.#.#...#.#...#.#",
        "###.###.#.#.#.#.#####.#",
        "#.#.#...#.#.#.#.....#.#",
        "#.#.#.#####.#.#####.#.#",
        "#...#.#...#.#.....#.#.#",
        "#.###.#.#.#.#.#####.#.#",
        "#.#...#.#...#.......#.#",
        "#.#.###.#############.#",
        "#.#...................#",
        "#######################"
    };

    // 速度0の敵弾を迷路の壁として保持するだけのパターン。
    static void ShotMazeWall(sEnemyShotSet* pEnemyShotSet)
    {
        (void)pEnemyShotSet;
    }

    static void CreateMaze(sEnemyShotSet* pEnemyShotSet)
    {
        for (int y = 0; y < MAZE_H; y++) {
            for (int x = 0; x < MAZE_W; x++) {
                if (maze[y][x] != '#') {
                    continue;
                }

                sEnemyShot* pEnemyShot = new sEnemyShot;

                pEnemyShot->x = MAZE_ORIGIN_X + MAZE_CELL * x;
                pEnemyShot->y = MAZE_ORIGIN_Y + MAZE_CELL * y;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;

                // 20x20の大玉を20px間隔で並べ、壁を途切れさせない。
                pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白
                pEnemyShot->margin = 999.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
}

// 敵本体のパターン
void EnemyPat_FixedMaze_ChatGPT()
{
    static bool bossUnlocked = false;

    constexpr double START_X = MAZE_ORIGIN_X + MAZE_CELL * 1.0;
    constexpr double START_Y = MAZE_ORIGIN_Y + MAZE_CELL * 1.0;
    constexpr double BOSS_X = MAZE_ORIGIN_X + MAZE_CELL * 21.0;
    constexpr double BOSS_Y = MAZE_ORIGIN_Y + MAZE_CELL * 21.0;

    if (count == 1) {
        // 自機は左上の迷路入口から開始。
        player.x = START_X;
        player.y = START_Y;

        // ボスは右下に固定。
        enemy.x = 240;
        enemy.y = 60;
        enemy.maxHp = enemy.hp = 100;

        bossUnlocked = false;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 毎フレーム右下に固定しておく。
        //enemy.x = BOSS_X;
        //enemy.y = BOSS_Y;
    }

    // 右下のゴールまで到達したらボス戦を解禁。
    // 一度解禁したら、その後はゴールから離れても解除しない。
    if (!bossUnlocked) {
        const double dx = player.x - BOSS_X;
        const double dy = player.y - BOSS_Y;
        if (dx * dx + dy * dy <= 18.0 * 18.0) {
            bossUnlocked = true;
        }
        else {
            // ボスへのダメージを毎フレーム無効化する。
            enemy.hp = enemy.maxHp;
        }
    }

    // 迷路は開始時に一度だけ生成する。
    if (count == 1) {
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

        enemyShotSetHead.prev->next = pEnemyShotSet;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev = pEnemyShotSet;

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        CreateMaze(pEnemyShotSet);
    }
}