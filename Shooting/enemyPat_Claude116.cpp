// enemyPat_WilsonCloudChamber.cpp
// ウィルソンの霧箱(Wilson Cloud Chamber)モチーフ弾幕
//
// 過飽和蒸気中を荷電粒子が通過すると、電離した経路に沿って水滴が
// 凝結し「飛跡」が見える。磁場中では電荷の符号に応じて左右に弧を描き、
// ガンマ線の対生成では1点からV字に飛跡が分岐する。
// このパターンでは、既存の弾種を「先端(動く本体)」と「トレイル(その場に
// 残る粒)」の2役に使い分け、先端が通過した軌道上にトレイルを置き続ける
// ことで飛跡を表現する。


// ============================================================
//  役割・種別の識別に使う定数
// ============================================================
static const int ROLE_HEAD = 0;  // 先端(動く本体)
static const int ROLE_TRAIL = 1; // 飛跡(その場に残る粒)

static const int TRACK_COSMIC = 0;  // 宇宙線(直進)
static const int TRACK_CHARGED = 1; // 荷電粒子(磁場で等曲率に曲がる)
static const int TRACK_BETA = 2;    // ベータ粒子(多重散乱でジグザグ)
static const int TRACK_ALPHA = 3;   // アルファ粒子(太く短い)

static const double TRAIL_LIFE = 300.0; // トレイルの寿命(約10秒、60fps換算)

// ============================================================
//  共通ヘルパー: shotSet のリストに弾を1つ追加する
// ============================================================
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// トレイル(飛跡の粒)を1つ置く。以後は動かさず、その場に留まる。
static void DropTrail(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, int kind)
{
    sEnemyShot* pTrail = AddShot(pEnemyShotSet);
    pTrail->x = x;
    pTrail->y = y;
    pTrail->muki = muki; // 置かれた瞬間の進行方向を見た目用に保持
    pTrail->speed = 0.0;
    pTrail->kind = kind;
    pTrail->param_i[0] = ROLE_TRAIL;
    pTrail->param_d[0] = TRAIL_LIFE;
}

// ============================================================
//  弾幕: 飛跡(トラック)本体
//  1つの shotSet の中に「先端」と「トレイル」が混在する。
//  先端は種別ごとに軌道を計算しつつ、一定間隔でその場にトレイルを残す。
// ============================================================
static void ShotTrack(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == ROLE_TRAIL) {
            // 飛跡: 動かさず、寿命が尽きたら霧が晴れるように消す
            pShot->param_d[0] -= 1.0;
            if (pShot->param_d[0] <= 0.0) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        else {
            // 先端: 種別ごとの軌道更新
            switch (pShot->param_i[1]) {
            case TRACK_CHARGED:
                // 磁場中の等曲率運動: 向きを一定角速度で回転させ続ける
                pShot->muki += pShot->param_d[1] * pShot->param_i[2];
                break;
            case TRACK_BETA:
                // 多重散乱: 進行方向を毎フレーム小さくランダムに揺らす
                pShot->muki += (GetRand(200) - 100) / 100.0 * pShot->param_d[1];
                break;
            case TRACK_COSMIC:
            case TRACK_ALPHA:
            default:
                // 直進: 向きは変えない
                break;
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 一定間隔でトレイルを落とす(電離した水滴が凝結する様子)
            pShot->param_d[3] += 1.0;
            if (pShot->param_d[3] >= pShot->param_d[2]) {
                pShot->param_d[3] = 0.0;
                DropTrail(pEnemyShotSet, pShot->x, pShot->y, pShot->muki, pShot->param_i[3]);
            }

            // アルファ粒子は電離力が強く飛程が短いため、短時間で消滅させる
            if (pShot->param_i[1] == TRACK_ALPHA) {
                pShot->param_d[4] -= 1.0;
                if (pShot->param_d[4] <= 0.0) {
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                    pShot = pNext;
                    continue;
                }
            }
        }

        pShot = pNext;
    }
}

// ============================================================
//  背景: 過飽和蒸気のもや(避ける必要のほぼない低密度・低速の粒)
// ============================================================
static void ShotVapor(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % 6 == 0) {
        sEnemyShot* pShot = AddShot(pEnemyShotSet);
        while (true) {
            pShot->x = GetRand(480);
            pShot->y = GetRand(480);
            if (hypot(pShot->x - player.x, pShot->y - player.y) > 30.0) break;
        }
        pShot->muki = GetRand(359) / 180.0 * DX_PI;
        pShot->speed = 0.1 + GetRand(20) / 100.0;
        pShot->kind = img_enemyShotSmallBall[6]; // 白
        pShot->param_i[0] = ROLE_TRAIL; // 動くが「置かれた粒」と同じ扱いにする
        pShot->param_d[0] = 240.0;      // 短命で入れ替わり続ける
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot->param_d[0] -= 1.0;
        if (pShot->param_d[0] <= 0.0) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
            pShot = pNext;
            continue;
        }

        pShot = pNext;
    }
}

// ============================================================
//  1本の飛跡を発生させるヘルパー
// ============================================================
static void SpawnTrack(double x, double y, double muki, double speed,
    int trackType, int curveDir, double angularSpeed,
    int headKind, int trailKind, double trailInterval, double alphaLife)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotTrack;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    sEnemyShot* pHead = AddShot(pEnemyShotSet);
    pHead->x = x;
    pHead->y = y;
    pHead->muki = muki;
    pHead->speed = speed;
    pHead->kind = headKind;
    pHead->param_i[0] = ROLE_HEAD;
    pHead->param_i[1] = trackType;
    pHead->param_i[2] = curveDir;      // +1 or -1 (曲がる方向 = 電荷の符号)
    pHead->param_i[3] = trailKind;     // トレイルに使う画像
    pHead->param_d[1] = angularSpeed;  // 角速度 or ジグザグの振れ幅
    pHead->param_d[2] = trailInterval; // トレイルを落とす間隔(フレーム)
    pHead->param_d[3] = 0.0;           // トレイル間隔カウンタ
    pHead->param_d[4] = alphaLife;     // アルファ粒子の寿命(他種別では未使用)
}

// ============================================================
//  対生成: ガンマ線が1点から電子・陽電子の対になり、
//  逆向きに曲がる2本の飛跡としてV字に現れる。
//  (ガンマ線自体は電離しないため、対生成点までの飛跡は残らない)
// ============================================================
static void SpawnPairProduction(double x, double y, double muki)
{
    // 陽電子(赤)と電子(青)が逆向きに曲がりながら飛び出す
    SpawnTrack(x, y, muki, 2.6, TRACK_CHARGED, +1, 0.035,
        img_enemyShotDiamond[0], img_enemyShotSmallBall[0], 5.0, 0.0);
    SpawnTrack(x, y, muki, 2.6, TRACK_CHARGED, -1, 0.035,
        img_enemyShotDiamond[4], img_enemyShotSmallBall[4], 5.0, 0.0);
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_CloudChamber_Claude()
{
    static int vaporStarted;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        vaporStarted = 0;
    }
    else {
        // 霧箱本体はゆっくり左右に揺れるだけ
        enemy.x = 240.0 + sin(count / 140.0) * 120.0;
    }

    // 背景の蒸気(初回のみ shotSet を1つ作り、以後はその中で粒を生成し続ける)
    if (!vaporStarted) {
        vaporStarted = 1;

        sEnemyShotSet* pVapor = new sEnemyShotSet;
        pVapor->count = 0;
        pVapor->patternFunc = ShotVapor;
        pVapor->x = 240.0;
        pVapor->y = 240.0;

        pVapor->pEnemyShotHead = new sEnemyShot;
        pVapor->pEnemyShotHead->prev = pVapor->pEnemyShotHead;
        pVapor->pEnemyShotHead->next = pVapor->pEnemyShotHead;

        pVapor->prev = enemyShotSetHead.prev;
        pVapor->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pVapor;
        enemyShotSetHead.prev = pVapor;
    }

    // 宇宙線: 画面上端付近からほぼ下向きに高速で直進する
    if (count % 25 == 1) {
        double x = GetRand(480);
        double muki = (70 + GetRand(40)) / 180.0 * DX_PI; // 80±20度=だいたい下向き
        SpawnTrack(x, -10.0, muki, 5.5, TRACK_COSMIC, 0, 0.0,
            img_enemyShotBullet[6], img_enemyShotSmallBall[6], 4.0, 0.0);
    }

    // 荷電粒子: 中心付近から放射状に発生し、磁場で弧を描く
    if (count % 20 == 1) {
        double muki = GetRand(359) / 180.0 * DX_PI;
        int curveDir = (GetRand(1) == 0) ? +1 : -1;
        int color = (curveDir > 0) ? 0 : 4; // + は赤、- は青
        SpawnTrack(enemy.x, enemy.y, muki, 2.2, TRACK_CHARGED, curveDir, 0.025,
            img_enemyShotDiamond[color], img_enemyShotSmallBall[color], 5.0, 0.0);
    }

    // ベータ粒子: 細く、ゆらゆらとジグザグに揺れる飛跡
    if (count % 14 == 1) {
        double x;
        double y;
        while (true) {
            x = GetRand(480);
            y = GetRand(480);
            if (hypot(x - player.x, y - player.y) > 30.0) break;
        }
        double muki = GetRand(359) / 180.0 * DX_PI;
        SpawnTrack(x, y, muki, 1.8, TRACK_BETA, 0, 0.15,
            img_enemyShotScale[1], img_enemyShotSmallBall[1], 6.0, 0.0);
    }

    // アルファ粒子: 太く短い飛跡
    if (count % 45 == 1) {
        double x;
        double y;
        while (true) {
            x = GetRand(480);
            y = GetRand(480);
            if (hypot(x - player.x, y - player.y) > 30.0) break;
        }
        double muki = GetRand(359) / 180.0 * DX_PI;
        SpawnTrack(x, y, muki, 1.0, TRACK_ALPHA, 0, 0.0,
            img_enemyShotLargeBall[8], img_enemyShotMediumBall[8], 2.0, 20.0);
    }

    // 対生成: まれに、中心付近からV字の飛跡が発生する
    if (count % 90 == 1) {
        double muki = GetRand(359) / 180.0 * DX_PI;
        SpawnPairProduction(enemy.x, enemy.y, muki);
    }
}