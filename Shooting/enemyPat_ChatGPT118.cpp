// 前方宣言
static void ShotBamboo(sEnemyShotSet* pEnemyShotSet);
static void ShotFireBurst(sEnemyShotSet* pEnemyShotSet);
static void ShotEmber(sEnemyShotSet* pEnemyShotSet);

// ============================================================
//  竹やぶ焼けた
//  ・竹の幹      : 緑の銃弾 / 緑の鱗弾
//  ・燃え落ちる  : 橙の菱形弾
//  ・炎/火の粉    : 赤・黄・橙の小玉
// ============================================================

static void AddShotSet(sEnemyShotSet::PatternFunc patternFunc,
    double x, double y, double muki, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

static void PlayPatternSound(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

// ------------------------------------------------------------
//  竹
//  下からまっすぐ伸びる緑の弾列が竹の幹を作る。
//  一定時間後、橙色の菱形弾へ変化して外側へ倒れ込む。
// ------------------------------------------------------------
static void ShotBamboo(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % 6 == 0 && pEnemyShotSet->count < 132) {
        sEnemyShot* pEnemyShot = new sEnemyShot;

        const int segment = pEnemyShotSet->count / 6;

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = 510.0;
        pEnemyShot->muki = -DX_PI / 2.0;
        pEnemyShot->speed = 2.6+0.5;
        pEnemyShot->margin = 120;

        // 竹の幹を、細い弾と小さな葉のような弾で交互に表現する。
        if (segment % 4 == 0) {
            pEnemyShot->kind = img_enemyShotScale[2];   // 緑
        }
        else {
            pEnemyShot->kind = img_enemyShotBullet[2];  // 緑
        }

        // 0: 根元のX、1: 倒れる向き、2: 揺れ位相
        // 3: 焼け落ちたときの基準X、4: 焼け落ちたときの基準Y
        pEnemyShot->param_d[0] = pEnemyShotSet->x;
        pEnemyShot->param_d[1] = (pEnemyShotSet->x < 240.0) ? -1.0 : 1.0;
        pEnemyShot->param_d[2] = GetRand(628) / 100.0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int t = pShot->count;

        if (t < 95) {
            // ほんの少し揺れながら、地面から竹が伸びていく。
            const double sway = 2.2 * sin(t * 0.065 + pShot->param_d[2]);
            pShot->x = pShot->param_d[0] + sway;
            pShot->y = 510.0 - pShot->speed * t;
            pShot->muki = -DX_PI / 2.0;
        }
        else {
            if (t == 95) {
                pShot->param_d[3] = pShot->x;
                pShot->param_d[4] = pShot->y;
                pShot->kind = img_enemyShotDiamond[8]; // 橙
            }

            const double burnT = t - 95.0;
            const double dir = pShot->param_d[1];

            // 焼けた竹が外側へ倒れ込み、下へ落ちていく。
            pShot->x = pShot->param_d[3]
                + dir * (0.85 * burnT + 0.0042 * burnT * burnT);
            pShot->y = pShot->param_d[4]
                + 1.65 * burnT + 0.0080 * burnT * burnT;

            pShot->muki = atan2(
                1.65 + 0.0160 * burnT,
                dir * (0.85 + 0.0084 * burnT)
            );
        }

        pShot = pShot->next;
    }

    // 竹の中腹から火が燃え広がる。
    if (pEnemyShotSet->count == 95 && pEnemyShotSet->param_i[0] == 0) {
        pEnemyShotSet->param_i[0] = 1;

        AddShotSet(
            ShotFireBurst,
            pEnemyShotSet->x,
            255.0,
            0.0,
            (pEnemyShotSet->x < 240.0) ? 0 : 1
        );
    }
}

// ------------------------------------------------------------
//  炎
//  竹の中腹から炎が扇状に広がり、少しずつ軌道を曲げる。
// ------------------------------------------------------------
static void ShotFireBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 28; ++i) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            const double center = 13.5;
            const double angle = -DX_PI / 2.0 + (i - center) * 0.072;
            const double speed = 2.3 + (i % 6) * 0.32;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = speed;
            pEnemyShot->margin = 120;

            // 赤・橙・黄を混ぜて炎を表現。
            const int color = (i + pEnemyShotSet->kind * 2) % 3;
            if (color == 0) {
                pEnemyShot->kind = img_enemyShotSmallBall[0];
            }
            else if (color == 1) {
                pEnemyShot->kind = img_enemyShotSmallBall[8];
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[1];
            }

            pEnemyShot->param_d[0] = GetRand(628) / 100.0; // 曲がり位相
            pEnemyShot->param_d[1] = (i % 2 == 0) ? 1.0 : -1.0;
            pEnemyShot->param_d[2] = 0.0;

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
        pShot->param_d[2] += 0.02;
        pShot->y += pShot->param_d[2];

        // 炎が少しうねりながら外へ広がる。
        pShot->muki +=
            0.0028 * pShot->param_d[1] *
            sin(pShot->count * 0.075 + pShot->param_d[0]);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  火の粉
//  地面から舞い上がって大きく弧を描き、最後は落ちてくる。
//  弾の位置は count から毎フレーム直接計算する。
// ------------------------------------------------------------
static void ShotEmber(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % 4 == 0 && pEnemyShotSet->count < 108) {
        sEnemyShot* pEnemyShot = new sEnemyShot;

        const int index = pEnemyShotSet->count / 4;
        const double phase = GetRand(628) / 100.0;
        const double vx =
            (GetRand(160) - 80) / 100.0;
        const double vy =
            3.0 + GetRand(100) / 100.0;

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = 515.0;
        pEnemyShot->muki = -DX_PI / 2.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->margin = 120;

        if (index % 3 == 0) {
            pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤
        }
        else if (index % 3 == 1) {
            pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙
        }
        else {
            pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄
        }

        // 0: 初期X、1: 横速度、2: 上向き速度、3: うねり位相
        pEnemyShot->param_d[0] = pEnemyShotSet->x;
        pEnemyShot->param_d[1] = vx;
        pEnemyShot->param_d[2] = vy;
        pEnemyShot->param_d[3] = phase;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double t = (double)pShot->count;

        // 舞い上がって左右へ散り、頂点を越えて落下する放物運動。
        pShot->x =
            pShot->param_d[0]
            + pShot->param_d[1] * t
            + 10.0 * sin(t * 0.075 + pShot->param_d[3]);

        pShot->y =
            515.0
            - pShot->param_d[2] * t
            + 0.015 * t * t;

        pShot->muki = atan2(
            -pShot->param_d[2] + 0.040 * t,
            pShot->param_d[1] + 0.75 * cos(t * 0.075 + pShot->param_d[3])
        );

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  敵本体
// ------------------------------------------------------------
void EnemyPat_TakeyabuYaketa_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 48.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.85 * muki;

        if (count % 150 == 75) {
            muki *= -1;
        }
    }

    const int cycle = (count - 1) % 600;

    // --------------------------------------------------------
    //  第一段：竹林が一斉に伸びる
    // --------------------------------------------------------
    if (cycle == 0) {
        PlayPatternSound(sound_enemyCharge);

        for (int i = 0; i < 9; ++i) {
            const double x = 35.0 + i * 51.0;

            AddShotSet(
                ShotBamboo,
                x,
                510.0,
                -DX_PI / 2.0,
                i
            );
        }
    }

    // --------------------------------------------------------
    //  第二段：燃え広がりを追加
    // --------------------------------------------------------
    if (cycle == 150) {
        PlayPatternSound(sound_enemyShot_heavy);

        AddShotSet(ShotFireBurst, 70.0, 285.0, 0.0, 0);
        AddShotSet(ShotFireBurst, 190.0, 230.0, 0.0, 1);
        AddShotSet(ShotFireBurst, 310.0, 265.0, 0.0, 0);
        AddShotSet(ShotFireBurst, 430.0, 220.0, 0.0, 1);
    }

    // --------------------------------------------------------
    //  第三段：焼けた竹が崩れながら、再び別の竹が伸びる
    // --------------------------------------------------------
    if (cycle == 270) {
        PlayPatternSound(sound_enemyShot_medium);

        for (int i = 0; i < 8; ++i) {
            const double x = 62.0 + i * 51.0;

            AddShotSet(
                ShotBamboo,
                x,
                510.0,
                -DX_PI / 2.0,
                i + 9
            );
        }
    }

    // --------------------------------------------------------
    //  最終段：画面下から大量の火の粉が舞い上がる
    // --------------------------------------------------------
    if (cycle == 420) {
        PlayPatternSound(sound_enemyShot_extreme);

        for (int i = 0; i < 10; ++i) {
            const double x = 12.0 + i * 50.5;

            AddShotSet(
                ShotEmber,
                x,
                515.0,
                -DX_PI / 2.0,
                i
            );
        }
    }
}
