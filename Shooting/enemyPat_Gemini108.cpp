// =========================================================
// 弾幕パターン：迷路生成（速度0の弾を配置）
// =========================================================
static void ShotMaze(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 24x24の迷路データ (0:通路, 1:壁)
        int maze[24][24] = {
            {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
            {1,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1},
            {1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,1,0,1},
            {1,0,1,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,1,0,0,1},
            {1,0,1,1,1,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1},
            {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1},
            {1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,1,0,1},
            {1,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,1},
            {1,0,1,1,1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,1,1,1,1,1},
            {1,0,1,0,0,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,0,0,0,1},
            {1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1,1,1,1,1,1,1,0,1},
            {1,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,1},
            {1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1},
            {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,1,0,1,0,0,1},
            {1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,0,1,0,1,1},
            {1,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0,1},
            {1,0,1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,0,1,1,1,1,0,1},
            {1,0,1,0,1,0,1,0,0,0,0,0,1,0,1,0,1,0,0,0,0,1,0,1},
            {1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,1,1,1,0,1,0,1},
            {1,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,0,0,0,1,0,1,0,1},
            {1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,0,1,0,1,0,1},
            {1,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
            {1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1},
            {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
        };

        // 念のため、自機スタート地点(左上)とボスエリア(右下)を確実に広げておく
        for (int i = 1; i <= 2; i++) {
            for (int j = 1; j <= 2; j++) {
                maze[i][j] = 0;
            }
        }
        for (int i = 20; i <= 22; i++) {
            for (int j = 19; j <= 22; j++) {
                maze[i][j] = 0;
            }
        }

        for (int i = 0; i < 24; i++) {
            for (int j = 0; j < 24; j++) {
                if (maze[i][j] == 1) {
                    sEnemyShot* pEnemyShot = new sEnemyShot;
                    // 1マス20ピクセル。中心座標を計算して配置
                    pEnemyShot->x = j * 20.0 + 10.0;
                    pEnemyShot->y = i * 20.0 + 10.0;
                    pEnemyShot->muki = 0.0;
                    pEnemyShot->speed = 0.0; // 速度0で動かない壁にする

                    // 中玉(青)で壁を形成。適度な隙間があり自機が通りやすい
                    pEnemyShot->kind = img_enemyShotMediumBall[4];

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
    }
}

// =========================================================
// 弾幕パターン：ボスの反撃（自機狙い3Way）
// =========================================================
static void ShotBossAim(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * (15.0 / 180.0 * DX_PI);
            pEnemyShot->speed = 1.8;
            pEnemyShot->kind = img_enemyShotSmallBall[0]; // 小玉(赤)を使用

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

// =========================================================
// 敵本体のパターン関数
// =========================================================
void EnemyPat_FixedMaze_Gemini()
{
    if (count == 1) {
        // 自機と敵の初期位置を固定で設定
        player.x = 30.0;  // 迷路の左上通路付近
        player.y = 30.0;

        enemy.x = 450.0;  // 迷路の右下エリア
        enemy.y = 450.0;
        enemy.maxHp = enemy.hp = 50;

        // 迷路生成（開始時に1度だけ発動）
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMaze;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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

    // ★ ギミック処理：迷路を抜けるまではダメージを与えられない
    // ボスのいる右下エリアに侵入するまで、毎フレームHPを全回復させて実質無敵状態にする
    if (player.x < 380.0 || player.y < 380.0) {
        enemy.hp = enemy.maxHp;
    }

    // ★ ボスの攻撃処理
    // 80フレームに1回、自機を狙って弾を撃つ
    if (count % 80 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBossAim;
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