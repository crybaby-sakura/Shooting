// enemyPat_Tmp.cpp

// 「流符『夜空のカーペット』」
// 星の弾を画面いっぱいに敷き詰め、波打つ通路を作りながら降下させる。
// 終盤では敷かれた星が一斉に流星化し、プレイヤー方向へ流れ込む。

static void AddNightCarpetShot(sEnemyShotSet* pEnemyShotSet, int index, int count)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    const double columns = 19.0;
    const double baseX = 12.0 + index * (456.0 / (columns - 1.0));

    pEnemyShot->x = baseX;
    pEnemyShot->y = pEnemyShotSet->y;
    pEnemyShot->muki = 0.0;
    pEnemyShot->speed = 0.0;
    pEnemyShot->kind = (index % 4 == 0) ? img_enemyShotMediumOval[3] : img_enemyShotSmallBall[6];
    pEnemyShot->margin = 40.0;

    // [0] 基準X、[1] 基準Y、[2] 位相、[3] 列番号
    // [4][5] 流星化時の座標、[6] 流星化時の角度
    pEnemyShot->param_d[0] = baseX;
    pEnemyShot->param_d[1] = pEnemyShotSet->y;
    pEnemyShot->param_d[2] = index * 0.44 + count * 0.08;
    pEnemyShot->param_d[3] = index;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static void ShotNightCarpet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    // 一枚の「カーペット」を構成する横一列の星
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 19; i++) {
            AddNightCarpetShot(pEnemyShotSet, i, pEnemyShotSet->kind);
        }
    }

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int age = pShot->count;

        if (age < 190) {
            const double t = age * 0.055 + pShot->param_d[2];
            const double rowWave = 22.0 * sin(t * 0.72);
            const double sway = 34.0 * sin(t * 0.31);
            const double gapCenter = 240.0 + 165.0 * sin((age + pEnemyShotSet->kind * 18) * 0.028);
            const double dx = pShot->param_d[0] + sway - gapCenter;
            const double push = (fabs(dx) < 52.0)
                ? ((dx < 0.0) ? -42.0 : 42.0)
                : 0.0;

            pShot->x = pShot->param_d[0] + rowWave + sway + push;
            pShot->y = pShot->param_d[1] + age * 1.18;

            // 絨毯の途中だけ少し色を変えて、波の流れを視覚的に強調
            if ((int)pShot->param_d[3] % 4 == 0)
                pShot->kind = (age % 36 < 18) ? img_enemyShotMediumOval[3] : img_enemyShotMediumOval[4];
            else
                pShot->kind = (age % 44 < 22) ? img_enemyShotSmallBall[6] : img_enemyShotSmallBall[3];
        }
        else {
            // カーペットの先端がほどけて、一斉に流星になる
            if (age == 190 && pEnemyShotSet->kind % 2 == 0) {
                const double burstT = age * 0.055 + pShot->param_d[2];
                pShot->param_d[4] = pShot->param_d[0]
                    + 22.0 * sin(burstT * 0.72)
                    + 34.0 * sin(burstT * 0.31);
                pShot->param_d[5] = pShot->param_d[1] + age * 1.18;

                double targetAngle = atan2(player.y - pShot->param_d[5], player.x - pShot->param_d[4]);
                const double spread = ((pShot->param_d[3] - 9.0) / 9.0) * 0.18;
                pShot->param_d[6] = targetAngle + spread;

                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }

            const double t = age - 190.0;
            const double accel = 2.0 + 0.010 * t;

            pShot->x = pShot->param_d[4] + cos(pShot->param_d[6]) * accel * t;
            pShot->y = pShot->param_d[5] + sin(pShot->param_d[6]) * accel * t;
            pShot->muki = pShot->param_d[6];
            pShot->speed = accel;

            pShot->kind = ((int)pShot->param_d[3] % 3 != 0)
                ? img_enemyShotDiamond[4]
                : img_enemyShotMediumOval[4];
        }

        pShot = pShot->next;
    }
}

static void AddNightMeteorSet(sEnemyShotSet* pEnemyShotSet, int index)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    pEnemyShot->x = pEnemyShotSet->x;
    pEnemyShot->y = pEnemyShotSet->y;
    pEnemyShot->muki = pEnemyShotSet->muki;
    pEnemyShot->speed = 0.0;
    pEnemyShot->kind = (index % 2 == 0) ? img_enemyShotDiamond[4] : img_enemyShotMediumOval[4];
    pEnemyShot->margin = 240.0;

    pEnemyShot->param_d[0] = index;
    pEnemyShot->param_d[1] = 240.0 + 120.0 * sin(index * 0.73);
    pEnemyShot->param_d[2] = 0.010 + 0.0025 * (index % 5);

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static void ShotNightMeteorFan(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 11; i++) {
            AddNightMeteorSet(pEnemyShotSet, i);
        }
    }

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double age = pShot->count;
        const double theta = pShot->param_d[0] * 0.53 + age * pShot->param_d[2];
        const double radius = pShot->param_d[1] + age * 0.62;

        pShot->x = pEnemyShotSet->x + cos(theta) * radius;
        pShot->y = pEnemyShotSet->y + sin(theta) * radius * 0.58;
        pShot->muki = theta + DX_PI / 2.0;

        pShot->kind = (pShot->count % 28 < 14)
            ? img_enemyShotDiamond[4]
            : img_enemyShotMediumOval[4];

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_NightCarpet_ChatGPT()
{
    static int carpetIndex;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
        carpetIndex = 0;
    }
    else {
        // 星が敷かれる様子を見せるため、ボスはゆっくり左右へ流れる
        enemy.x = 240.0 + 138.0 * sin(count * 0.013);
        enemy.y = 68.0 + 18.0 * sin(count * 0.017);
    }

    // 予告音を鳴らしてから、一定間隔で「夜空の一枚」を敷いていく
    if (count % 18 == 1) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNightCarpet;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = carpetIndex++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ときどき、ほどけた星空を大きく円状に流す「流星の扇」を重ねる
    if (count % 180 == 91) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNightMeteorFan;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y - 50.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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
