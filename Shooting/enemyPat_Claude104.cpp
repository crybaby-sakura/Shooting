// enemyPat_HakuriIssen.cpp
//
// 剥離一閃 -ブラジリアンワックス-
// 専用素材なし。既存の小玉/中玉/鱗弾/菱形弾/銃弾のみを組み合わせて
// 「塗布 → 圧着 → 引き剥がし → 仕上げ」の施術サイクルを表現する
// 無限ループ4フェーズパターン。
//
// ①塗布フェーズ   : 橙の小玉グリッドが列ごとに上から降りてきて肌(施術エリア)に敷かれる
// ②圧着フェーズ   : 白の中玉グリッド(クロス)が剛体で落下し静止、鱗弾スタックの体毛が
//                    肌から生え揃う。牽制のチクッと弾を断続発射
// ③引き剥がしフェーズ: 一瞬の溜めの後、クロスが高速で横滑り。体毛は同方向へちぎれ飛び、
//                    ワックス層は中心から放射状に加速飛散。衝撃波リング＋自機狙い3wayで痛みを表現
// ④仕上げフェーズ : つるすべの輝き(白小玉の緩やかな拡散)の後、自機狙い5wayで締めて①へループ

// ============================================================
//  パターン全体の設計値
// ============================================================

// 施術エリア（画面固定座標。enemy本体の揺れとは独立させ、burst計算を単純化する）
static const double AreaCenterX = 240.0;
static const double AreaTopY = 150.0;

// ワックス層（小玉グリッド）
static const int    WaxCols = 16 * 2;
static const int    WaxRows = 6 * 2;
static const double WaxColSpacing = 18.0 / 2;
static const double WaxRowSpacing = 14.0 / 2;

// クロス（圧着ストリップ）グリッド
static const int    ClothCols = 18;
static const int    ClothRows = 7;
static const double ClothColSpacing = 17.0;
static const double ClothRowSpacing = 13.0;

// 体毛（鱗弾スタック）
static const int    HairColStep = 2 + 1; // ワックス列2本ごとに1本の毛
static const int    HairSegments = 4;

// フェーズ区切り（0-based、ループ周期 CycleLen で繰り返す）
static const int PhaseApplyBegin = 0;
static const int PhaseApplyLen = 150;                              // ①塗布
static const int PhasePressBegin = PhaseApplyBegin + PhaseApplyLen;  // 150
static const int PhasePressLen = 150;                              // ②圧着
static const int PhaseRipBegin = PhasePressBegin + PhasePressLen;  // 300
static const int PhaseRipLen = 90;                               // ③引き剥がし
static const int PhaseAfterBegin = PhaseRipBegin + PhaseRipLen;      // 390
static const int PhaseAfterLen = 150;                              // ④仕上げ
static const int CycleLen = PhaseAfterBegin + PhaseAfterLen;  // 540

static const int RipPauseFrames = 10; // 引き剥がし直前の「溜め」

// ============================================================
//  補助関数
// ============================================================

// pSetの弾リストに1発追加して返す
static sEnemyShot* AppendShot(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->margin = 240;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 新規sEnemyShotSetを生成しリストに繋いで返す
static sEnemyShotSet* CreateShotSet(double x, double y, sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = 0.0;
    pSet->kind = 0;
    pSet->patternFunc = func;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

static inline double EaseOutQuad(double t)
{
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return 1.0 - (1.0 - t) * (1.0 - t);
}

// ============================================================
//  ①ワックス層：橙の小玉グリッド。列ごとに上から降りて着地し静止。
//  ③引き剥がしフェーズに入ると、施術エリア中心から放射状に加速飛散する。
// ============================================================
static void ShotWaxLayer(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        double colX = pEnemyShotSet->x;
        for (int row = 0; row < WaxRows; row++) {
            sEnemyShot* pShot = AppendShot(pEnemyShotSet);
            double targetY = AreaTopY + row * WaxRowSpacing;
            pShot->kind = img_enemyShotSmallBall[8]; // 橙
            pShot->param_d[0] = colX;          // 確定x
            pShot->param_d[1] = targetY;       // 確定y
            pShot->param_d[2] = targetY - 60.0; // 降下開始y
            pShot->muki = DX_PI / 2.0;
        }
    }

    int cyc = (count - 1) % CycleLen;
    double areaCenterY = AreaTopY + (WaxRows - 1) * WaxRowSpacing / 2.0;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (cyc >= PhaseRipBegin) {
            // ③引き剥がし：中心から放射状に加速して弾け飛ぶ
            double elapsed = (double)(cyc - PhaseRipBegin);
            double angle = atan2(pShot->param_d[1] - areaCenterY, pShot->param_d[0] - AreaCenterX);
            double dist = 0.055 * elapsed * elapsed;
            pShot->x = pShot->param_d[0] + dist * cos(angle);
            pShot->y = pShot->param_d[1] + dist * sin(angle);
            pShot->muki = angle;
        }
        else {
            // ①塗布〜②圧着：上から降りて着地、以後静止
            double t = pShot->count / 24.0;
            double ease = EaseOutQuad(t);
            pShot->x = pShot->param_d[0];
            pShot->y = pShot->param_d[2] + (pShot->param_d[1] - pShot->param_d[2]) * ease;
            pShot->muki = DX_PI / 2.0;
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  ②クロス：白の中玉グリッド。剛体で落下→静止。
//  ③引き剥がしで一瞬の溜めの後、高速で横に滑走する。
// ============================================================
static void ShotClothPress(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double originX = AreaCenterX - (ClothCols - 1) * ClothColSpacing / 2.0;
        double originY = AreaTopY - (ClothRows - 1) * ClothRowSpacing / 2.0 + 20.0;
        for (int row = 0; row < ClothRows; row++) {
            for (int col = 0; col < ClothCols; col++) {
                sEnemyShot* pShot = AppendShot(pEnemyShotSet);
                double targetX = originX + col * ClothColSpacing;
                double targetY = originY + row * ClothRowSpacing;
                pShot->kind = img_enemyShotMediumBall[6]; // 白
                pShot->param_d[0] = targetX;
                pShot->param_d[1] = targetY;
                pShot->param_d[2] = targetY - 220.0; // 降下開始y
                pShot->muki = DX_PI / 2.0;
            }
        }
    }

    int cyc = (count - 1) % CycleLen;
    int loopIndex = (count - 1) / CycleLen;
    double dirSign = (loopIndex % 2 == 0) ? 1.0 : -1.0; // ループごとに引き剥がし方向を左右交互に

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (cyc >= PhaseRipBegin) {
            double elapsed = (double)(cyc - PhaseRipBegin);
            double dx = 0.0;
            if (elapsed > RipPauseFrames) {
                dx = (elapsed - RipPauseFrames) * 7.5;
            }
            pShot->x = pShot->param_d[0] + dirSign * dx;
            pShot->y = pShot->param_d[1];
            pShot->muki = (dirSign > 0.0) ? 0.0 : DX_PI;
        }
        else {
            double t = pShot->count / 40.0;
            double ease = EaseOutQuad(t);
            pShot->x = pShot->param_d[0];
            pShot->y = pShot->param_d[2] + (pShot->param_d[1] - pShot->param_d[2]) * ease;
            pShot->muki = DX_PI / 2.0;
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  体毛：鱗弾を縦に4segスタックし、根本側から時間差(param_i[1]の遅延)で
//  生え揃う。③引き剥がしの瞬間、クロスと同じ方向へ勢いよくちぎれ飛ぶ。
// ============================================================
static void ShotHairStrand(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        double baseX = pEnemyShotSet->x;
        for (int seg = 0; seg < HairSegments; seg++) {
            sEnemyShot* pShot = AppendShot(pEnemyShotSet);
            double targetY = AreaTopY - 4.0 - seg * 5.0; // 根本(肌)から上へ積む
            pShot->kind = img_enemyShotScale[7]; // 黒
            pShot->param_d[0] = baseX;
            pShot->param_d[1] = targetY;
            pShot->param_i[0] = seg;
            pShot->param_i[1] = seg * 4; // 生え始めの遅延フレーム
            pShot->muki = -DX_PI / 2.0;
        }
    }

    int cyc = (count - 1) % CycleLen;
    int loopIndex = (count - 1) / CycleLen;
    double dirSign = (loopIndex % 2 == 0) ? 1.0 : -1.0;
    int releaseAt = PhaseRipBegin + RipPauseFrames;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (cyc >= releaseAt) {
            // ちぎれて飛散：クロスの引き剥がし方向へ、セグメントごとに扇状に開きながら加速
            double elapsed = (double)(cyc - releaseAt);
            double fanOffset = (pShot->param_i[0] - (HairSegments - 1) / 2.0) * 1.6;
            pShot->x = pShot->param_d[0] + dirSign * elapsed * 9.0;
            pShot->y = pShot->param_d[1] - fanOffset * elapsed;
            pShot->muki = (dirSign > 0.0) ? 0.0 : DX_PI;
        }
        else {
            // 生え揃いフェーズ：根本から遅延つきでせり上がる
            double localCount = pShot->count - pShot->param_i[1];
            double t = localCount / 18.0;
            double ease = EaseOutQuad(t);
            double startY = pShot->param_d[1] + 20.0;
            pShot->x = pShot->param_d[0];
            pShot->y = startY + (pShot->param_d[1] - startY) * ease;
            pShot->muki = -DX_PI / 2.0;
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  衝撃波リング：③引き剥がしの瞬間、施術エリア中心から放射
// ============================================================
static void ShotShockRing(sEnemyShotSet* pEnemyShotSet)
{
    static const int RingCount = 40;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < RingCount; i++) {
            sEnemyShot* pShot = AppendShot(pEnemyShotSet);
            double angle = i * (2.0 * DX_PI / RingCount);
            pShot->kind = img_enemyShotDiamond[1]; // 黄
            pShot->param_d[0] = angle;
            pShot->muki = angle;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double speed = 3.6;
        double dist = speed * pShot->count;
        pShot->x = pEnemyShotSet->x + dist * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y + dist * sin(pShot->param_d[0]);
        pShot = pShot->next;
    }
}

// ============================================================
//  自機狙いショット：③引き剥がしの痛みと④仕上げのフィニッシュに共用
//  pEnemyShotSet->param_i[0] に way数を入れてから使う
// ============================================================
static void ShotAimedSpread(sEnemyShotSet* pEnemyShotSet)
{
    int way = pEnemyShotSet->param_i[0];

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double spread = 12.0 * DX_PI / 180.0;
        for (int i = 0; i < way; i++) {
            sEnemyShot* pShot = AppendShot(pEnemyShotSet);
            double angle = baseAngle + (i - (way - 1) / 2.0) * spread;
            pShot->kind = img_enemyShotBullet[0]; // 赤
            pShot->param_d[0] = angle;
            pShot->muki = angle;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double speed = 3.3;
        double dist = speed * pShot->count;
        pShot->x = pEnemyShotSet->x + dist * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y + dist * sin(pShot->param_d[0]);
        pShot = pShot->next;
    }
}

// ============================================================
//  ②圧着中の「チクッ」牽制弾：クロスの端付近から自機狙い2way
// ============================================================
static void ShotStingPulse(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pShot = AppendShot(pEnemyShotSet);
            double angle = baseAngle + (i - 0.5) * (10.0 * DX_PI / 180.0);
            pShot->kind = img_enemyShotBullet[1]; // 黄
            pShot->param_d[0] = angle;
            pShot->muki = angle;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double speed = 2.6;
        double dist = speed * pShot->count;
        pShot->x = pEnemyShotSet->x + dist * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y + dist * sin(pShot->param_d[0]);
        pShot = pShot->next;
    }
}

// ============================================================
//  ④仕上げ：つるすべの輝き。白小玉が中心からゆるやかに拡散する。
// ============================================================
static void ShotAfterglowSparkle(sEnemyShotSet* pEnemyShotSet)
{
    static const int SparkleCount = 28 * 2;

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < SparkleCount; i++) {
            sEnemyShot* pShot = AppendShot(pEnemyShotSet);
            double angle = GetRand(3600) / 3600.0 * 2.0 * DX_PI;
            double speed = 0.5 + GetRand(100) / 100.0;
            pShot->kind = img_enemyShotSmallBall[6]; // 白
            pShot->param_d[0] = angle;
            pShot->param_d[1] = speed;
            pShot->muki = angle;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double dist = pShot->param_d[1] * pShot->count;
        pShot->x = pEnemyShotSet->x + dist * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y + dist * sin(pShot->param_d[0]);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体：剥離一閃 -ブラジリアンワックス-
// ============================================================
void EnemyPat_BrazilianWax_Claude()
{
    if (count == 1) {
        enemy.x = AreaCenterX;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        enemy.x = AreaCenterX + 40.0 * sin((count - 1) / 130.0);
    }

    int cyc = (count - 1) % CycleLen;

    // ①塗布：列ごとに時間差でワックスの小玉グリッドを敷いていく
    {
        int t = cyc - PhaseApplyBegin;
        int colInterval = 6 / 2;
        if (t >= 0 && t < WaxCols * colInterval && t % colInterval == 0) {
            int col = t / colInterval;
            double colX = (AreaCenterX - (WaxCols - 1) * WaxColSpacing / 2.0) + col * WaxColSpacing;
            CreateShotSet(colX, AreaTopY, ShotWaxLayer);
        }

        // 塗布中の軽い牽制（垂らし弾）
        if (t >= 20 && t < PhaseApplyLen && t % 25 == 0) {
            CreateShotSet(AreaCenterX, AreaTopY - 20.0, ShotStingPulse);
        }
    }

    // ②圧着：クロス本体を1回だけ生成、体毛は列を間引いて複数本
    if (cyc == PhasePressBegin) {
        CreateShotSet(AreaCenterX, AreaTopY, ShotClothPress);

        for (int col = 0; col < WaxCols; col += HairColStep) {
            double colX = (AreaCenterX - (WaxCols - 1) * WaxColSpacing / 2.0) + col * WaxColSpacing;
            CreateShotSet(colX, AreaTopY, ShotHairStrand);
        }
    }

    // ②圧着中の「チクッ」牽制弾（クロス着地後〜引き剥がし直前まで断続的に、左右交互）
    {
        int t = cyc - (PhasePressBegin + 45);
        if (t >= 0 && t < (PhasePressLen - 45) && t % 18 == 0) {
            double side = ((t / 18) % 2 == 0) ? -1.0 : 1.0;
            double clothHalfWidth = (ClothCols - 1) * ClothColSpacing / 2.0;
            CreateShotSet(AreaCenterX + side * clothHalfWidth, AreaTopY, ShotStingPulse);
        }
    }

    // ③引き剥がし：衝撃波リング → 自機狙い3way（体毛とクロスの滑走は各パターン内で自律的に発火）
    if (cyc == PhaseRipBegin) {
        double areaCenterY = AreaTopY + (WaxRows - 1) * WaxRowSpacing / 2.0;
        CreateShotSet(AreaCenterX, areaCenterY, ShotShockRing);
    }
    if (cyc == PhaseRipBegin + RipPauseFrames) {
        sEnemyShotSet* pSet = CreateShotSet(AreaCenterX, AreaTopY, ShotAimedSpread);
        pSet->param_i[0] = 3;
    }

    // ④仕上げ：つるすべの輝き → 自機狙い5wayフィニッシュ
    if (cyc == PhaseAfterBegin) {
        double areaCenterY = AreaTopY + (WaxRows - 1) * WaxRowSpacing / 2.0;
        CreateShotSet(AreaCenterX, areaCenterY, ShotAfterglowSparkle);
    }
    if (cyc == PhaseAfterBegin + 90) {
        sEnemyShotSet* pSet = CreateShotSet(AreaCenterX, AreaTopY, ShotAimedSpread);
        pSet->param_i[0] = 5;
    }
}