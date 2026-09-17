// enemyPat_Tmp.cpp
// 弾幕：「陰蜂式・無限蜂の巣」
//
// ・7セルの六角形を連結した蜂の巣を形成
// ・蜂の巣は回転しながら収縮／再膨張し、隙間の位置を変化させる
// ・定期的に高速の自機狙い扇状弾を追加
// ・大きな崩壊タイミングでは放射状の高速弾を追加
//
// count / pEnemyShotSet->count / pEnemyShot->count の加算、
// 画面外の弾の削除はメインルーチン側で行う仕様。

// ------------------------------------------------------------
//  共通ヘルパー
// ------------------------------------------------------------
static sEnemyShotSet* CreateEnemyShotSet(sEnemyShotSet::PatternFunc patternFunc,
    double x, double y,
    double muki, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = patternFunc;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

static void AddShot(sEnemyShotSet* pSet,
    double x, double y,
    double muki, double speed,
    int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// ------------------------------------------------------------
//  蜂の巣本体
// ------------------------------------------------------------
static void ShotHive(sEnemyShotSet* pSet)
{
    constexpr int CELL_COUNT = 7;
    constexpr int SHOTS_PER_CELL = 18;
    constexpr double CELL_DISTANCE = 72.0;
    constexpr double HEX_RADIUS = 40.0;

    // 初回だけ7セル分の六角形を生成
    if (pSet->count == 0) {
        static const double cellOffset[CELL_COUNT][2] = {
            { 0.0, 0.0 },
            {  0.0, -CELL_DISTANCE },
            { 62.35, -36.0 },
            { 62.35,  36.0 },
            {  0.0,  CELL_DISTANCE },
            {-62.35,  36.0 },
            {-62.35, -36.0 }
        };

        for (int cell = 0; cell < CELL_COUNT; ++cell) {
            for (int i = 0; i < SHOTS_PER_CELL; ++i) {
                // 6辺×3点で六角形の輪郭を作る
                const int edge = i / 3;
                const int segment = i % 3;
                const double t = (double)(segment + 1) / 4.0;

                const double a0 = edge * DX_PI / 3.0;
                const double a1 = (edge + 1) * DX_PI / 3.0;

                const double x0 = HEX_RADIUS * cos(a0);
                const double y0 = HEX_RADIUS * sin(a0);
                const double x1 = HEX_RADIUS * cos(a1);
                const double y1 = HEX_RADIUS * sin(a1);

                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = 0.0;
                pShot->speed = 0.0;
                pShot->kind = ((cell + edge) % 4 == 0)
                    ? img_enemyShotMediumBall[8]
                    : img_enemyShotSmallBall[0];

                pShot->param_i[0] = cell;
                pShot->param_i[1] = i;
                pShot->param_i[2] = 0;

                pShot->param_d[0] = cellOffset[cell][0];
                pShot->param_d[1] = cellOffset[cell][1];
                pShot->param_d[2] = x0 + (x1 - x0) * t;
                pShot->param_d[3] = y0 + (y1 - y0) * t;

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    const double t = (double)pSet->count;

    // 周期的に収縮し、蜂の巣の穴の形を大きく変える
    const double pulse = sin(t * DX_PI / 70.0);
    const double cellScale = 1.0 - 0.42 * pulse;
    const double hexScale = 1.0 + 0.18 * pulse;

    // 収縮期に回転方向が一時的に反転する
    const double rotation = 0.0065 * t + 0.42 * sin(t * DX_PI / 105.0);

    // 蜂の巣全体は緩やかに下へ流れ、横方向にも揺れる
    const double driftY = 1.25 * t;
    const double driftX = 28.0 * sin(t * DX_PI / 150.0);

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        const double ox = pShot->param_d[0] * cellScale + pShot->param_d[2] * hexScale;
        const double oy = pShot->param_d[1] * cellScale + pShot->param_d[3] * hexScale;

        const double rx = ox * cos(rotation) - oy * sin(rotation);
        const double ry = ox * sin(rotation) + oy * cos(rotation);

        // 後半ほど外周が崩れていくよう、横方向に小さな位相ずれを追加
        const double localWobble = 7.0 * sin(0.035 * t + pShot->param_i[1] * 0.9 + pShot->param_i[0]);

        pShot->x = pSet->x + driftX + rx + localWobble;
        pShot->y = pSet->y + driftY + ry;

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  高速自機狙い扇
// ------------------------------------------------------------
static void ShotAimedFan(sEnemyShotSet* pSet)
{
    constexpr int SHOT_COUNT = 7;
    constexpr double FAN_STEP = 0.045;

    if (pSet->count == 0) {
        // 発射時の照準角を固定し、以後は弾自身の進路を維持
        const double baseAngle = atan2(player.y - pSet->y, player.x - pSet->x);

        for (int i = 0; i < SHOT_COUNT; ++i) {
            const double offset = (i - SHOT_COUNT / 2) * FAN_STEP;
            const int colorIndex = (i & 1) ? 8 : 0;
            AddShot(pSet,
                pSet->x,
                pSet->y,
                baseAngle + offset,
                5.4 + 0.35 * (i & 1),
                img_enemyShotBullet[colorIndex]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  崩壊時の放射弾
// ------------------------------------------------------------
static void ShotCollapse(sEnemyShotSet* pSet)
{
    constexpr int SHOT_COUNT = 24;

    if (pSet->count == 0) {
        for (int i = 0; i < SHOT_COUNT; ++i) {
            const double angle = DX_PI * 2.0 * i / (double)SHOT_COUNT + pSet->muki;
            AddShot(pSet,
                pSet->x,
                pSet->y,
                angle,
                4.2 + (i % 3) * 0.28,
                (i & 1) ? img_enemyShotDiamond[8] : img_enemyShotScale[0]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 最初は直線、その後わずかに蛇行して「崩れ」を演出
        const double bend = 0.07 * sin(0.035 * pShot->count + pShot->muki * 3.0);
        pShot->x += pShot->speed * cos(pShot->muki + bend);
        pShot->y += pShot->speed * sin(pShot->muki + bend);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  極細レーザー風の弾列
// ------------------------------------------------------------
static void ShotNeedleLine(sEnemyShotSet* pSet)
{
    constexpr int SHOT_COUNT = 18;

    if (pSet->count == 0) {
        for (int i = 0; i < SHOT_COUNT; ++i) {
            const double distance = (double)i * 12.0;
            const double x = pSet->x - cos(pSet->muki) * distance;
            const double y = pSet->y - sin(pSet->muki) * distance;

            AddShot(pSet,
                x,
                y,
                pSet->muki,
                6.2,
                img_enemyShotDiamond[5]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Inbachi_ChatGPT()
{
    static int hiveCycle;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 68.0;
        enemy.maxHp = enemy.hp = 200;
        hiveCycle = 0;
    }
    else {
        // 上部を大きく横移動しつつ、わずかに上下する
        enemy.x = 240.0 + 145.0 * sin(count * 0.0075);
        enemy.y = 68.0 + 16.0 * sin(count * 0.0038);

        // 崩壊直前だけ予告音
        if (count % 150 == 100) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }

    // --------------------------------------------------------
    //  蜂の巣を定期的に生成
    // --------------------------------------------------------
    if (count % 60 == 1) {
        sEnemyShotSet* pSet = CreateEnemyShotSet(
            ShotHive,
            enemy.x,
            enemy.y + 22.0,
            0.0,
            hiveCycle++);

        (void)pSet;

        if ((hiveCycle & 1) == 1) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }

    // --------------------------------------------------------
    //  高速の自機狙い扇を絶え間なく重ねる
    // --------------------------------------------------------
    if (count % 13 == 1) {
        CreateEnemyShotSet(
            ShotAimedFan,
            enemy.x,
            enemy.y + 18.0,
            0.0,
            count);
    }

    // --------------------------------------------------------
    //  収縮が強くなるタイミングで弾列を通す
    // --------------------------------------------------------
    if (count % 75 == 25) {
        const double angle = atan2(player.y - enemy.y, player.x - enemy.x);
        CreateEnemyShotSet(
            ShotNeedleLine,
            enemy.x,
            enemy.y + 18.0,
            angle,
            count);
    }

    // --------------------------------------------------------
    //  蜂の巣崩壊：全方向へ一気に割れる
    // --------------------------------------------------------
    if (count % 150 == 1) {
        const double angle = atan2(player.y - enemy.y, player.x - enemy.x);

        CreateEnemyShotSet(
            ShotCollapse,
            enemy.x,
            enemy.y + 18.0,
            angle,
            count);

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
}