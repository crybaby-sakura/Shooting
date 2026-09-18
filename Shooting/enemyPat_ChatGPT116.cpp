// enemyPat_wilsonCloudChamber.cpp
// 弾幕：ウィルソンの霧箱

// 霧を構成する小玉。広い範囲をゆっくり漂いながら下降する。
static void ShotMist(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;

            double spread = (GetRand(1000) / 1000.0) * 480.0 - 240.0;
            double phase = GetRand(628) / 100.0;

            pEnemyShot->x = pEnemyShotSet->x + spread;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(80) - 40;
            pEnemyShot->muki = DX_PI / 2.0;
            pEnemyShot->speed = 0.22 + GetRand(25) / 100.0 - 0.07;
            pEnemyShot->kind = img_enemyShotSmallBall[3 + GetRand(2)];
            pEnemyShot->param_d[0] = phase;
            pEnemyShot->param_d[1] = 0.45 + GetRand(35) / 100.0;
            pEnemyShot->param_d[2] = pEnemyShot->x;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->y += pShot->speed;
        pShot->x = pShot->param_d[2] + sin(pShot->count * 0.045 + pShot->param_d[0]) * 18.0 * pShot->param_d[1];

        pShot = pShot->next;
    }
}

// 曲がる粒子飛跡。複数の弾を同じ曲線上に流して、霧箱の軌跡を表現する。
static void ShotTrackCurve(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
    }

    if (pEnemyShotSet->count % 3 == 0 && pEnemyShotSet->count < 66) {
        int emitted = pEnemyShotSet->count / 3;
        double t = emitted / 22.0;

        double baseX = pEnemyShotSet->param_d[0] + sin(t * 5.4 + pEnemyShotSet->param_d[2]) * 170.0;
        double baseY = pEnemyShotSet->param_d[1] + t * 520.0;

        for (int side = -1; side <= 1; side++) {
            pEnemyShot = new sEnemyShot;

            double angle = DX_PI / 2.0
                + sin(t * 4.0 + pEnemyShotSet->param_d[2]) * 0.72
                + side * 0.16;

            pEnemyShot->x = baseX + side * 6.0;
            pEnemyShot->y = baseY;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 3.7 + GetRand(25) / 100.0;
            pEnemyShot->kind = (side == 0) ? img_enemyShotBullet[3] : img_enemyShotBullet[5];
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = 0.014 + GetRand(8) / 1000.0;
            pEnemyShot->param_i[0] = side;
            pEnemyShot->margin = 120;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 飛跡をゆるく湾曲させる。
        pShot->muki = pShot->param_d[0]
            + sin((pShot->count + 20 * pShot->param_i[0]) * 0.03 + pEnemyShotSet->param_d[2]) * 0.55;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 霧箱内で粒子が衝突し、飛跡が枝分かれしたように見せるパターン。
static void ShotTrackBranch(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
    }

    if (pEnemyShotSet->count % 8 == 0 && pEnemyShotSet->count < 48) {
        int index = pEnemyShotSet->count / 8;
        double t = index / 6.0;
        double centerX = pEnemyShotSet->param_d[0] + sin(t * 2.7 + pEnemyShotSet->param_d[2]) * 150.0;
        double centerY = pEnemyShotSet->param_d[1] + t * 410.0;

        // 主軌跡。
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = centerX;
        pEnemyShot->y = centerY;
        pEnemyShot->muki = DX_PI / 2.0 + sin(t * 5.0 + pEnemyShotSet->param_d[2]) * 0.45;
        pEnemyShot->speed = 3.3;
        pEnemyShot->kind = img_enemyShotMediumOval[1];
        pEnemyShot->param_d[0] = pEnemyShot->muki;
        pEnemyShot->param_d[1] = t;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 衝突点から左右へ枝分かれ。
        for (int branch = -1; branch <= 1; branch += 2) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = centerX;
            pEnemyShot->y = centerY;
            pEnemyShot->muki = DX_PI / 2.0 + branch * (0.78 + GetRand(20) / 100.0);
            pEnemyShot->speed = 2.9 + GetRand(25) / 100.0;
            pEnemyShot->kind = img_enemyShotDiamond[8 - GetRand(2)];
            pEnemyShot->param_d[0] = pEnemyShot->muki;
            pEnemyShot->param_d[1] = branch;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 初期角度を基準に、時間とともに僅かに曲げる。
        pShot->muki = pShot->param_d[0] + sin(pShot->count * 0.035 + pShot->param_d[1]) * 0.28;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 霧の密度が一気に増し、その中から飛跡が立ち上がる短い高密度パターン。
static void ShotObservationBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count < 42 && pEnemyShotSet->count % 2 == 0) {
        double centerX = pEnemyShotSet->x + sin(pEnemyShotSet->count * 0.08 + pEnemyShotSet->param_d[0]) * 105.0;
        double centerY = pEnemyShotSet->y + 70.0 + pEnemyShotSet->count * 4.0;

        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI;

            pEnemyShot->x = centerX + GetRand(70) - 35;
            pEnemyShot->y = centerY + GetRand(24) - 12;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.2 + GetRand(35) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[6 + GetRand(2)];
            pEnemyShot->param_d[0] = angle;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    if (pEnemyShotSet->count == 18 || pEnemyShotSet->count == 36) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 18; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = i * DX_PI * 2.0 / 18.0 + pEnemyShotSet->param_d[1];
            double radius = 20.0 + GetRand(120);

            pEnemyShot->x = player.x + cos(angle) * radius;
            pEnemyShot->y = player.y + sin(angle) * radius;
            pEnemyShot->muki = angle + DX_PI + sin(i * 1.7) * 0.25;
            pEnemyShot->speed = 3.4 + GetRand(20) / 100.0;
            pEnemyShot->kind = img_enemyShotBullet[6];
            pEnemyShot->param_d[0] = pEnemyShot->muki;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->muki = pShot->param_d[0] + sin(pShot->count * 0.05) * 0.18;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

static void CreateWilsonShotSet(sEnemyShotSet::PatternFunc patternFunc, double x, double y, double muki, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;
    pEnemyShotSet->param_d[0] = GetRand(628) / 100.0;
    pEnemyShotSet->param_d[1] = GetRand(628) / 100.0;
    pEnemyShotSet->param_d[2] = GetRand(628) / 100.0;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 敵本体のパターン
void EnemyPat_CloudChamber_ChatGPT()
{
    static int muki;
    static int shot_cycle;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 52.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_cycle = 0;
    }
    else {
        enemy.x += 0.82 * (double)muki;
        if (count % 138 == 69) muki *= -1;
    }

    // 常時漂う霧。小玉を薄く積み重ね、霧箱そのものを表現する。
    if (count % 9 == 1) {
        CreateWilsonShotSet(ShotMist,
            240.0 + sin(count * 0.021) * 85.0,
            -20.0,
            DX_PI / 2.0,
            shot_cycle++);
    }

    // 曲がった飛跡が定期的に横切る。
    if (count % 184 == 18) {
        CreateWilsonShotSet(ShotTrackCurve,
            110.0 + GetRand(260),
            -35.0,
            DX_PI / 2.0,
            shot_cycle++);
    }

    // 別方向から別の曲線飛跡を重ね、観測される粒子を増やす。
    if (count % 184 == 54) {
        CreateWilsonShotSet(ShotTrackCurve,
            370.0 - GetRand(260),
            -25.0,
            DX_PI / 2.0,
            shot_cycle++);
    }

    // 衝突による枝分かれ飛跡。
    if (count % 268 == 86) {
        CreateWilsonShotSet(ShotTrackBranch,
            115.0 + GetRand(250),
            -40.0,
            DX_PI / 2.0,
            shot_cycle++);
    }

    // 一定周期ごとに「観測」し、霧の中から多数の飛跡を一気に浮かび上がらせる。
    //if (count % 268 == 42) {
    //    CreateWilsonShotSet(ShotObservationBurst,
    //        enemy.x,
    //        enemy.y,
    //        DX_PI / 2.0,
    //        shot_cycle++);
    //}
}
