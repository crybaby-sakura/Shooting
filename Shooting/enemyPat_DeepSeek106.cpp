// enemyPat_tmp_ikaruga.cpp
//
// 斑鳩風：赤/青の弾幕と、自機周囲を回る同色消弾バリア
//
// count, pEnemyShotSet->count, pEnemyShot->count のインクリメント、
// 画面外弾の削除はメインルーチンが行う想定です。

// ============================================================
// 定数
// ============================================================
static const int    BARRIER_ORB_NUM = 16;
static const double BARRIER_ORB_RADIUS = 48.0;
static const double BARRIER_HIT_RADIUS = 12.0;

static const int    BOSS_SHOT_INTERVAL = 20 / 2;
static const int    BOSS_SHOT_WAY = 7;
static const double BOSS_SHOT_ANGLE_STEP = 7.0 * DX_PI / 180.0;
static const double BOSS_SHOT_SPEED = 3.3;

// ============================================================
// 前方宣言
// ============================================================
static void ShotBossColor(sEnemyShotSet* pEnemyShotSet);
static void PlayerBarrierUpdate(sEnemyShotSet* pEnemyShotSet);

// ============================================================
// 補助関数
// ============================================================
static void AddEnemyShotSetToGlobal(sEnemyShotSet* pEnemyShotSet)
{
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

static sEnemyShotSet* CreateEnemyShotSet(
    sEnemyShotSet::PatternFunc func,
    double x,
    double y,
    double muki)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->patternFunc = func;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    AddEnemyShotSetToGlobal(pEnemyShotSet);
    return pEnemyShotSet;
}

static sEnemyShot* AddEnemyShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

    return pEnemyShot;
}

static void RemoveEnemyShot(sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev->next = pEnemyShot->next;
    pEnemyShot->next->prev = pEnemyShot->prev;
    delete pEnemyShot;
}

// ============================================================
// 自機周囲バリア
// ============================================================
static void SetBarrierColor(sEnemyShotSet* pBarrierSet, int color)
{
    if (pBarrierSet == nullptr) return;

    pBarrierSet->param_i[0] = color;

    sEnemyShot* p = pBarrierSet->pEnemyShotHead->next;
    while (p != pBarrierSet->pEnemyShotHead) {
        if (p->param_i[1] == 1) { // バリア玉
            p->param_i[0] = color;
            p->kind = img_enemyShotSmallBall[color];
        }
        p = p->next;
    }
}

static sEnemyShotSet* CreateBarrierSet(int color)
{
    sEnemyShotSet* pBarrierSet =
        CreateEnemyShotSet(PlayerBarrierUpdate, player.x, player.y, 0.0);

    pBarrierSet->param_i[0] = color;

    for (int i = 0; i < BARRIER_ORB_NUM; ++i) {
        sEnemyShot* p = AddEnemyShot(pBarrierSet);

        double ang = 2.0 * DX_PI * i / BARRIER_ORB_NUM;

        p->x = player.x + cos(ang) * BARRIER_ORB_RADIUS;
        p->y = player.y + sin(ang) * BARRIER_ORB_RADIUS;
        p->muki = 0.0;
        p->speed = 0.0;

        p->kind = img_enemyShotSmallBall[color];

        p->param_i[0] = color; // 0:赤, 4:青
        p->param_i[1] = 1;     // 1:バリア玉

        p->param_d[0] = ang;
        p->param_d[1] = BARRIER_ORB_RADIUS;

        p->margin = 999.0;
    }

    return pBarrierSet;
}

static void EraseSameColorShots(sEnemyShotSet* pBarrierSet)
{
    if (pBarrierSet == nullptr) return;

    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        if (pSet != pBarrierSet) {
            sEnemyShot* pShot = pSet->pEnemyShotHead->next;

            while (pShot != pSet->pEnemyShotHead) {
                sEnemyShot* pNext = pShot->next;

                if (pShot->param_i[1] == 2) { // 2:ボス弾
                    int shotColor = pShot->param_i[0];

                    sEnemyShot* pOrb = pBarrierSet->pEnemyShotHead->next;
                    while (pOrb != pBarrierSet->pEnemyShotHead) {
                        if (pOrb->param_i[1] == 1 &&
                            pOrb->param_i[0] == shotColor) {

                            double dx = pShot->x - pOrb->x;
                            double dy = pShot->y - pOrb->y;

                            if (dx * dx + dy * dy <
                                BARRIER_HIT_RADIUS * BARRIER_HIT_RADIUS) {
                                RemoveEnemyShot(pShot);
                                break;
                            }
                        }
                        pOrb = pOrb->next;
                    }
                }

                pShot = pNext;
            }
        }
        pSet = pSet->next;
    }
}

static void PlayerBarrierUpdate(sEnemyShotSet* pEnemyShotSet)
{
    // 自機の周囲を維持する
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (p->param_i[1] == 1) {
            double ang = p->param_d[0];
            double r = p->param_d[1];

            p->x = player.x + cos(ang) * r;
            p->y = player.y + sin(ang) * r;

            p->kind = img_enemyShotSmallBall[p->param_i[0]];
        }
        p = p->next;
    }

    // 同色のボス弾を消す
    EraseSameColorShots(pEnemyShotSet);
}

// ============================================================
// ボス弾
// ============================================================
static void ShotBossColor(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int color = pEnemyShotSet->param_i[0]; // 0:赤, 4:青

        double baseMuki = atan2(
            player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);

        for (int i = -(BOSS_SHOT_WAY / 2);
            i <= (BOSS_SHOT_WAY / 2);
            ++i) {

            sEnemyShot* p = AddEnemyShot(pEnemyShotSet);

            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;

            p->muki = baseMuki + i * BOSS_SHOT_ANGLE_STEP;
            p->speed = BOSS_SHOT_SPEED;

            p->kind = (color == 0)
                ? img_enemyShotMediumBall[0]
                : img_enemyShotMediumBall[4];

            p->param_i[0] = color;
            p->param_i[1] = 2; // 2:ボス弾
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_Ikaruga_DeepSeek()
{
    static int muki;
    static sEnemyShotSet* pBarrierSet;
    static int barrierColor;

    if (count == 1) {
        enemy.x = 120.0;
        enemy.y = 40.0;
        enemy.x2 = 360.0;
        enemy.y2 = 40.0;
        enemy.maxHp = enemy.hp = 200;

        muki = 1;
        pBarrierSet = nullptr;
        barrierColor = 0; // 赤

        // 0秒後：予告音
        if (CheckSoundMem(sound_enemyCharge)) {
            StopSoundMem(sound_enemyCharge);
        }
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (count > 1) {
        enemy.x += 0.98 * (double)muki;
        enemy.x2 += 0.98 * (double)muki;

        if (count % 120 == 60) muki *= -1;
    }

    // 1秒後、4秒後、7秒後...：極太予告音とバリア変化
    if (count % 180 == 61) {
        if (CheckSoundMem(sound_enemyShot_extreme)) {
            StopSoundMem(sound_enemyShot_extreme);
        }
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        if (pBarrierSet == nullptr) {
            // 初回：赤小玉を自機の周囲に配置
            pBarrierSet = CreateBarrierSet(0);
            barrierColor = 0;
        }
        else {
            // 以降：赤⇔青を切替
            barrierColor = (barrierColor == 0) ? 4 : 0;
            SetBarrierColor(pBarrierSet, barrierColor);
        }
    }

    // 3秒後、6秒後、9秒後...：予告音
    if (count % 180 == 1 && count != 1) {
        if (CheckSoundMem(sound_enemyCharge)) {
            StopSoundMem(sound_enemyCharge);
        }
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ボス射撃：赤はボス1、青はボス2
    if (count % BOSS_SHOT_INTERVAL == 1) {
        // 赤：ボス1
        sEnemyShotSet* pRedSet =
            CreateEnemyShotSet(ShotBossColor, enemy.x, enemy.y + 10.0, 0.0);
        pRedSet->param_i[0] = 0; // 赤
        pRedSet->param_i[1] = 1; // ボス1

        // 青：ボス2
        sEnemyShotSet* pBlueSet =
            CreateEnemyShotSet(ShotBossColor, enemy.x2, enemy.y2 + 10.0, 0.0);
        pBlueSet->param_i[0] = 4; // 青
        pBlueSet->param_i[1] = 2; // ボス2
    }
}