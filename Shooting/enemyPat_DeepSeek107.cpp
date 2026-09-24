// enemyPat_Tmp.cpp
// 弾幕：雨下のかみなりをモチーフにした『雨雷符「驟雨遠雷」』

// ============================================================
// 補助関数
// ============================================================
static sEnemyShot* AddEnemyShot(sEnemyShotSet* pEnemyShotSet,
    double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

static void UpdateEnemyShots(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

static sEnemyShotSet* CreateEnemyShotSet(sEnemyShotSet::PatternFunc func,
    double x, double y, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = kind;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// ============================================================
// 常時の雨
// ============================================================
static void ShotRain(sEnemyShotSet* pEnemyShotSet)
{
    // 2フレームごとに雨粒を1～2個生成
    if (pEnemyShotSet->count % 6 == 0) {
        int num = 1 + GetRand(1); // 1 or 2
        for (int i = 0; i < num; i++) {
            double x = (double)GetRand(480);
            double y = -10.0 - (double)GetRand(20);
            double muki = DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
            double speed = 2.0 + GetRand(300) / 100.0-1;
            // 小玉・青
            AddEnemyShot(pEnemyShotSet, x, y, muki, speed, img_enemyShotSmallBall[4]);
        }
    }

    // 30フレームごとに強い雨脚（銃弾・シアン）
    if (pEnemyShotSet->count % 40 == 0) {
        double x = (double)GetRand(480);
        double y = -10.0;
        double muki = DX_PI / 2.0 + (GetRand(10) - 5) / 180.0 * DX_PI;
        AddEnemyShot(pEnemyShotSet, x, y, muki, 8.0-3, img_enemyShotBullet[3]);
    }

    UpdateEnemyShots(pEnemyShotSet);
}

// ============================================================
// 雷（予兆→落雷→雷鳴）
// ============================================================
static void ShotLightning(sEnemyShotSet* pEnemyShotSet)
{
    const int CHARGE_TIME = 60;   // 予兆時間（1秒）
    const int THUNDER_TIME = 66;  // 雷鳴発生

    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 落雷位置を決める（自機付近だが完全追尾しない）
        double targetX = player.x + (GetRand(80) - 40);
        if (targetX < 20.0) targetX = 20.0;
        if (targetX > 460.0) targetX = 460.0;
        pEnemyShotSet->param_d[0] = targetX;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;

        // 予兆の大玉（白）
        AddEnemyShot(pEnemyShotSet, targetX, pEnemyShotSet->y,
            DX_PI / 2.0, 0.5, img_enemyShotLargeBall[6]);

        // 予兆周囲の小弾（シアン）
        for (int i = 0; i < 6; i++) {
            double x = targetX + (GetRand(40) - 20);
            double y = pEnemyShotSet->y + (GetRand(20) - 10);
            double muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
            double speed = 1.0 + GetRand(100) / 100.0;
            AddEnemyShot(pEnemyShotSet, x, y, muki, speed, img_enemyShotSmallBall[3]);
        }
    }
    else if (pEnemyShotSet->count == CHARGE_TIME) {
        // 落雷音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double targetX = pEnemyShotSet->param_d[0];
        double startY = pEnemyShotSet->param_d[1];

        // 稲妻：上から下へジグザグに短レーザーと中玉を配置
        for (int i = 0; i < 14; i++) {
            double y = startY + i * 30.0;
            double x = targetX + ((i % 2 == 0) ? -15.0 : 15.0) + (GetRand(10) - 5);
            // 幹：短レーザー（黄）
            AddEnemyShot(pEnemyShotSet, x, y, DX_PI / 2.0, 14.0, img_enemyShotLaser[1]);
            // 節：中玉（黄）
            if (i % 3 == 0) {
                AddEnemyShot(pEnemyShotSet, x, y, DX_PI / 2.0, 12.0, img_enemyShotMediumBall[1]);
            }
            // 枝：小弾（シアン）を斜め下に
            for (int j = 0; j < 2; j++) {
                double branchMuki = DX_PI / 2.0 + ((j == 0) ? -0.6 : 0.6)
                    + (GetRand(20) - 10) / 180.0 * DX_PI;
                double branchSpeed = 4.0 + GetRand(200) / 100.0;
                AddEnemyShot(pEnemyShotSet, x, y, branchMuki, branchSpeed, img_enemyShotSmallBall[3]);
            }
        }
    }
    else if (pEnemyShotSet->count == THUNDER_TIME) {
        // 雷鳴音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double targetX = pEnemyShotSet->param_d[0];
        double targetY = 440.0; // 画面下部付近

        // 雷鳴：中弾を24方向
        for (int i = 0; i < 24; i++) {
            double muki = i * DX_PI * 2.0 / 24.0;
            AddEnemyShot(pEnemyShotSet, targetX, targetY, muki, 3.0, img_enemyShotMediumBall[1]);
        }
        // 雷鳴：小弾を36方向（少し遅れて／別速度）
        for (int i = 0; i < 36; i++) {
            double muki = i * DX_PI * 2.0 / 36.0 + 0.05;
            AddEnemyShot(pEnemyShotSet, targetX, targetY, muki, 5.0, img_enemyShotSmallBall[6]);
        }
    }

    UpdateEnemyShots(pEnemyShotSet);
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_ThunderInRain_DeepSeek()
{
    static int muki;
    static int shot_count;
    static bool rainCreated;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
        rainCreated = false;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 常時の雨セットを一度だけ作成
    if (count == 2 && !rainCreated) {
        CreateEnemyShotSet(ShotRain, 240.0, 0.0, 0);
        rainCreated = true;
    }

    // 定期的に雷を発生（最初は少し待ってから）
    if (count % 180 == 120) {
        CreateEnemyShotSet(ShotLightning, enemy.x, 0.0, shot_count++);
    }
}