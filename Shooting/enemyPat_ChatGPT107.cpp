// enemyPat_rainThunder.cpp

// 雨粒：青い小玉を上から大量に降らせる
static void ShotRain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += cos(pShot->muki) * pShot->speed;
        pShot->y += sin(pShot->muki) * pShot->speed;
        pShot = pShot->next;
    }
}

// 雷：白い中玉を連ねて、蛇行する落雷を作る
static void ShotThunder(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double t = (double)pShot->count + pShot->param_d[0];
        const double phase = pShot->param_d[1];
        const int branch = pShot->param_i[0];

        // 上から下へ進みながら、折れ曲がる雷の軌跡を作る。
        // count から直接座標を求めることで方向転換時の累積誤差を防ぐ。
        const double fall = 1.75 * t * 2;
        double centerX = pEnemyShotSet->x;
        centerX += 105.0 * sin(0.034 * t + phase);
        centerX += 44.0 * sin(0.083 * t + phase * 1.7);

        // 枝分かれした雷は左右にずらして別の落雷らしくする。
        centerX += (double)branch * 34.0;

        pShot->x = centerX;
        pShot->y = -55.0 + fall;
        pShot->x += 6.0 * sin(0.21 * t + phase);

        pShot = pShot->next;
    }
}

// 雷が通過した場所から飛び散る火花
static void ShotThunderSpark(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += cos(pShot->muki) * pShot->speed;
        pShot->y += sin(pShot->muki) * pShot->speed;
        pShot = pShot->next;
    }
}

static void AddShotToSet(sEnemyShotSet* pEnemyShotSet, int kind, double x, double y, double muki, double speed)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->margin = 120;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static void InitShotSet(sEnemyShotSet* pEnemyShotSet, void(*patternFunc)(sEnemyShotSet*))
{
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 敵本体のパターン
void EnemyPat_ThunderInRain_ChatGPT()
{
    static int muki;
    static int thunderId;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 48.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        thunderId = 0;
    }
    else {
        enemy.x += 0.82 * (double)muki;
        if (count % 150 == 75)
            muki *= -1;
    }

    // 雨音のように青い小玉を連続散布する。
    if (count % 20 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        InitShotSet(pEnemyShotSet, ShotRain);
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;

        const int n = 7;
        for (int i = 0; i < n; i++) {
            const double spread = (double)(i - (n - 1) / 2) * 48.0;
            const double x = enemy.x + spread + (double)(GetRand(30) - 15);
            const double y = enemy.y + (double)GetRand(20);
            const double mukiShot = DX_PI * 0.5 + (GetRand(20) - 10) * 0.006;
            const double speed = 4.0 + (double)GetRand(80) / 100.0 - 1;
            AddShotToSet(pEnemyShotSet, img_enemyShotSmallBall[4], x, y, mukiShot, speed);
        }
    }

    // 一定間隔で落雷。複数本を同時に発生させて、雨の中を縦に走らせる。
    if (count % 180 == 20) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (count % 180 == 45) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int branch = -1; branch <= 1; branch++) {
            sEnemyShotSet* pThunderSet = new sEnemyShotSet;
            InitShotSet(pThunderSet, ShotThunder);
            pThunderSet->x = enemy.x + branch * 24.0;
            pThunderSet->y = enemy.y;
            pThunderSet->kind = thunderId++;

            const double phase = 0.75 * (double)thunderId + branch * 0.8;

            // 一列の中玉を密に並べ、一本の雷として見せる。
            for (int i = 0; i < 34; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pThunderSet->x;
                pShot->y = -55.0;
                pShot->muki = DX_PI * 0.5;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotMediumBall[6];
                pShot->param_d[0] = -(double)i * 2.5;
                pShot->param_d[1] = phase;
                pShot->param_i[0] = branch;
                pShot->margin = 480;

                pShot->prev = pThunderSet->pEnemyShotHead->prev;
                pShot->next = pThunderSet->pEnemyShotHead;
                pThunderSet->pEnemyShotHead->prev->next = pShot;
                pThunderSet->pEnemyShotHead->prev = pShot;
            }

            // 雷の末端や枝から飛び散る火花。
            sEnemyShotSet* pSparkSet = new sEnemyShotSet;
            InitShotSet(pSparkSet, ShotThunderSpark);
            pSparkSet->x = pThunderSet->x;
            pSparkSet->y = pThunderSet->y;
            pSparkSet->kind = thunderId;

            for (int i = 0; i < 14; i++) {
                const double ang = DX_PI * 0.5 + DX_PI * 2.0 * (double)i / 14.0;
                const double speed = 2.0 + (double)GetRand(90) / 100.0;
                AddShotToSet(
                    pSparkSet,
                    img_enemyShotSmallBall[1],
                    pThunderSet->x,
                    enemy.y + 35.0,
                    ang,
                    speed
                );
            }
        }
    }

    // 雷の合間にも、雨の密度を上げる短い集中豪雨を入れる。
    if (count % 180 >= 46 && count % 180 <= 60 && count % 2 == 0) {
        sEnemyShotSet* pRainBurst = new sEnemyShotSet;
        InitShotSet(pRainBurst, ShotRain);
        pRainBurst->x = enemy.x;
        pRainBurst->y = enemy.y;

        for (int i = 0; i < 11-8; i++) {
            const double x = enemy.x + (double)(GetRand(420) - 210);
            const double y = enemy.y + (double)GetRand(55);
            const double mukiShot = DX_PI * 0.5 + (GetRand(28) - 14) * 0.006;
            const double speed = 4.5 + (double)GetRand(70) / 100.0 - 1;
            AddShotToSet(pRainBurst, img_enemyShotSmallBall[4], x, y, mukiShot, speed);
        }
    }
}
