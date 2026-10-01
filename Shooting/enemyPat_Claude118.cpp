// enemyPat_takeyabuYaketa.cpp
//
// 弾幕モチーフ：「竹やぶ焼けた」
// 静止した竹やぶ(縦に並んだ短レーザーの節)が下から炎に炙られ、
// 炎が触れた節から順に「緑(生きた竹)→橙(延焼中)→黒(焼け落ち)」と色を変えながら
// 崩れ落ちていき、全ての竹が焼け尽きたところで灰(全方位バラマキ)が舞い散る、
// という一連の流れを既存の弾素材のみで表現する。
//
// 使用素材:
//   竹の節     : 短レーザー(64.0x4.0) 緑(2) → 橙(8) → 黒(7)
//                muki=90度で寝かせて縦向きにし、竹の節に見立てる
//   火の粉     : 小玉(2.5x2.5) 橙(8)/赤(0) を交互に、上方向へ小さくゆらぎながら放出
//   飛び火     : 中玉(7.0x7.0) 赤(0) を自機狙いで低頻度に混ぜて緩急をつける
//   灰の飛散   : 小玉(2.5x2.5) 白(6) を全方位に大量放出(フィナーレ)

constexpr int T = 600;
static int countT;

namespace {

    constexpr int    NUM_COLUMNS = 6;                                       // 竹の本数
    constexpr double COLUMN_MARGIN = 30.0;                                    // 左右の余白
    constexpr double COLUMN_SPACING = (480.0 - COLUMN_MARGIN * 2) / (NUM_COLUMNS - 1);

    constexpr double GROUND_Y = 470.0;   // 竹の根元(画面下端付近)
    constexpr double TOP_Y = -20.0;   // 竹の先端(画面上端よりやや外)
    constexpr double SEGMENT_LEN = 64.0;    // 短レーザー1本分の長さ
    constexpr double SEGMENT_GAP = 10.0;    // 節の隙間
    constexpr double SEGMENT_PITCH = SEGMENT_LEN + SEGMENT_GAP;

    constexpr int    IGNITE_START = 90;      // 最初の列が燃え始めるフレーム
    constexpr int    IGNITE_DELAY = 16;      // 列ごとの着火タイムラグ(左→右に燃え広がる)
    constexpr double FLAME_RISE_SPD = 2.6;     // 炎が駆け上がる速さ(px/frame)
    constexpr int    EMBER_INTERVAL = 5*8;       // 火の粉を出す間隔(フレーム)
    constexpr int    STRAY_INTERVAL = 70*2;      // 飛び火(自機狙い)の間隔

    // 列 col の着火開始フレーム
    inline int IgniteStartFrame(int col)
    {
        return IGNITE_START + col * IGNITE_DELAY;
    }

    // 列 col の現在の炎の高さ(y座標)。まだ着火していなければ、
    // どの節よりも下(=未到達)を表す大きな値を返す。
    inline double FlameY(int col, int globalCount)
    {
        int t = globalCount - IgniteStartFrame(col);
        if (t <= 0) return GROUND_Y + SEGMENT_PITCH;
        return GROUND_Y - FLAME_RISE_SPD * t;
    }

} // namespace

// ------------------------------------------------------------
// 竹の節(縦向きの短レーザー)。1本の竹＝1つの ShotSet にまとめて生成する。
//   param_i[0] : 0=生きた竹 / 1=延焼中(橙) / 2=焼け落ち中(黒)
//   param_i[1] : 状態遷移後の経過フレーム(色替え・崩落モーション用)
//   param_d[0] : 落下速度(焼け落ち中に重力的に加速)
// ------------------------------------------------------------
static void ShotBambooColumn(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    if (pEnemyShotSet->count == 0) {
        double x = pEnemyShotSet->x;

        // 根元(GROUND_Y)から先端(TOP_Y)まで節を敷き詰める
        for (double y = GROUND_Y; y > TOP_Y; y -= SEGMENT_PITCH) {
            pShot = new sEnemyShot;

            pShot->x = x;
            pShot->y = y;
            pShot->muki = DX_PI / 2.0;            // 縦向きに寝かせて竹の節に見立てる
            pShot->speed = 0.0;                    // 延焼するまでは静止
            pShot->kind = img_enemyShotLaser[2];  // 緑=生きた竹

            pShot->param_i[0] = 0;
            pShot->param_i[1] = 0;
            pShot->param_d[0] = 0.0;
            pShot->margin = 70;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    int    col = pEnemyShotSet->kind;
    double flameY = FlameY(col, countT);

    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case 0: // まだ生きている竹
            if (flameY <= pShot->y) {
                // 炎がここまで到達 → 延焼開始
                pShot->param_i[0] = 1;
                pShot->kind = img_enemyShotLaser[8]; // 橙=燃えている
            }
            break;

        case 1: // 延焼中(橙)→しばらくしたら黒く焼け落ちる
            pShot->param_i[1]++;
            if (pShot->param_i[1] == 18) {
                pShot->param_i[0] = 2;
                pShot->kind = img_enemyShotLaser[7]; // 黒=焼け落ちた竹
            }
            break;

        case 2: // 黒く焼け落ちて崩れ落ちていく
            pShot->param_d[0] += 0.15;                             // 重力的に加速
            pShot->y += pShot->param_d[0];
            pShot->x += 0.4 * sin(pShot->param_i[1] * 0.1);      // 崩れながら左右にゆれる
            pShot->muki += 0.03;                                    // 傾いていく
            pShot->param_i[1]++;
            break;
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 火の粉(炎の先端から立ち上る小玉)。1回の生成で数発だけ出す使い捨てSet。
// ------------------------------------------------------------
static void ShotEmber(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 4; i++) {
            pShot = new sEnemyShot;

            pShot->x = pEnemyShotSet->x + GetRand(16) - 8;
            pShot->y = pEnemyShotSet->y + GetRand(10) - 5;
            // ほぼ真上、左右にわずかにゆらぐ(炎の揺らぎ)
            pShot->muki = -DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
            pShot->speed = 1.0 + GetRand(150) / 100.0;
            pShot->kind = (i % 2 == 0) ? img_enemyShotSmallBall[8] : img_enemyShotSmallBall[0]; // 橙/赤交互

            pShot->param_i[0] = 0;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 上昇しながら少しずつ横にゆらめかせる(炎の揺らぎ)
        pShot->param_i[0]++;
        pShot->x += pShot->speed * cos(pShot->muki) + sin(pShot->param_i[0] * 0.2) * 0.3;
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 飛び火(自機狙いの中玉)。低頻度で混ぜて緩急をつける。
// ------------------------------------------------------------
static void ShotStrayEmber(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    if (pEnemyShotSet->count == 0) {
        pShot = new sEnemyShot;

        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki; // 生成時点での自機狙い角度
        pShot->speed = 2.6;
        pShot->kind = img_enemyShotMediumBall[0]; // 赤

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// フィナーレ：全ての竹が焼け落ちた後、灰が全方位に舞い散る。
// ------------------------------------------------------------
static void ShotAsh(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    if (pEnemyShotSet->count == 0) {
        const int ASH_COUNT = 220; // 灰吹雪、密度は多めに
        for (int i = 0; i < ASH_COUNT; i++) {
            pShot = new sEnemyShot;

            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = (double)i / ASH_COUNT * 2.0 * DX_PI + (GetRand(40) - 20) / 180.0 * DX_PI;
            pShot->speed = 0.8 + GetRand(150) / 100.0;
            pShot->kind = img_enemyShotSmallBall[6]; // 白=灰

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot->speed *= 0.999; // ふわっと減速しながら漂う

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体：竹やぶ焼けた
// ------------------------------------------------------------
void EnemyPat_TakeyabuYaketa_Claude()
{
    static double columnX[NUM_COLUMNS];
    static bool   ashFired;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 30.0;
        enemy.maxHp = enemy.hp = 200;
    }

    countT = count % T;

    if (countT == 1) {
        ashFired = false;

        for (int i = 0; i < NUM_COLUMNS; i++) {
            columnX[i] = COLUMN_MARGIN + COLUMN_SPACING * i;
        }

        // フェーズ1：竹やぶを一斉に生成(静止した竹)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < NUM_COLUMNS; i++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotBambooColumn;
            pSet->x = columnX[i];
            pSet->y = 0.0;
            pSet->kind = i; // 列番号を保持

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }

    // フェーズ2〜3：各列が着火期間中、炎の高さから火の粉と飛び火を継続的に放出
    for (int i = 0; i < NUM_COLUMNS; i++) {
        int t = countT - IgniteStartFrame(i);
        if (t <= 0) continue;                 // まだ着火前

        double flameY = FlameY(i, countT);
        if (flameY < TOP_Y) continue;         // この列はもう先端まで焼け終わった

        // 火の粉
        if ((countT + i * 3) % EMBER_INTERVAL == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotEmber;
            pSet->x = columnX[i];
            pSet->y = flameY;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 低頻度で飛び火(自機狙い)を混ぜて緩急をつける
        if ((countT + i * 37) % STRAY_INTERVAL == 0) {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotStrayEmber;
            pSet->x = columnX[i];
            pSet->y = flameY;
            pSet->muki = atan2(player.y - flameY, player.x - columnX[i]);

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }

    // フェーズ4：全ての列が焼け終わったら灰を一度だけ舞い散らせる
    if (!ashFired) {
        int lastFinish = IgniteStartFrame(NUM_COLUMNS - 1)
            + (int)((GROUND_Y - TOP_Y) / FLAME_RISE_SPD) + 20;
        if (countT >= lastFinish) {
            ashFired = true;

            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotAsh;
            pSet->x = 240.0;
            pSet->y = 240.0;

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