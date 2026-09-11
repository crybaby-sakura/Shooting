// enemyPat_ShuuuRaisen.cpp
// 驟雨雷閃 -雨下のかみなり-
// 雨の中を稲妻が走る様子をモチーフにした無限ループパターン。
// 専用素材はないため、小玉(雨)・中玉(雷雲)・短レーザー(稲妻)・菱形弾(着弾スパーク)・銃弾(自機狙い)の
// 組み合わせだけで表現する。
// 敵本体関数名は EnemyPat_ThunderInRain_Claude。

// ============================================================
// 定数
// ============================================================
static constexpr int    CYCLE_LEN = 600; // 1周期のフレーム数(以後ループ)
static constexpr int    CLOUD_NUM = 3;    // 雷雲の数
static const double     CLOUD_X[CLOUD_NUM] = { 80.0, 240.0, 400.0 };
static constexpr double CLOUD_Y = 30.0;
static constexpr int    PUFF_PER_CLOUD = 7;    // 1つの雲を構成する弾数

static constexpr int    WARNING_PHASE_START = 100;  // 雷雲予兆フェーズ開始
static constexpr int    STRIKE_PHASE_START = 100;  // 落雷フェーズ開始(予兆と重ねて進行)
static constexpr int    STRIKE_PHASE_END = 460;  // 落雷フェーズ終了
static constexpr int    STRIKE_INTERVAL = 40;   // 1回の落雷にかける予兆～発動のフレーム数
static constexpr int    STRIKE_NUM = (STRIKE_PHASE_END - STRIKE_PHASE_START) / STRIKE_INTERVAL; // 9回
static constexpr int    INTENSIFY_START = 460;  // 雷鳴・驟雨強化フェーズ開始

// ============================================================
// 雨（背景で常時降り続ける小玉。斜めのsin揺らぎで風に流される雨を表現）
// param_d[0]=初期x  param_d[1]=初期y  param_d[2]=sin位相
// ============================================================
static void ShotRain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double baseX = pShot->param_d[0];
        double baseY = pShot->param_d[1];
        double phase0 = pShot->param_d[2];

        pShot->x = baseX + 5.0 * sin(t * 0.04 + phase0) / 2;
        pShot->y = baseY + 4.6 * t / 3;

        double vx = 5.0 * 0.04 * cos(t * 0.04 + phase0);
        pShot->muki = atan2(4.6, vx);

        pShot = pShot->next;
    }
}

// ============================================================
// 雷雲（画面上部に常駐。次に落雷する雲だけ黒⇔白にパルスして予告する）
// param_d[0]=固定x  param_d[1]=固定y  param_i[0]=自分が属する雲のインデックス
// ============================================================
static void ShotCloud(sEnemyShotSet* pEnemyShotSet)
{
    int localFrame = (count - 1) % CYCLE_LEN;
    int warnCloud = -1;
    int warnPhaseFrame = 0;
    if (localFrame >= WARNING_PHASE_START && localFrame < STRIKE_PHASE_END) {
        int strikeIdx = (localFrame - STRIKE_PHASE_START) / STRIKE_INTERVAL;
        warnCloud = strikeIdx % CLOUD_NUM;
        warnPhaseFrame = (localFrame - STRIKE_PHASE_START) % STRIKE_INTERVAL;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0];
        pShot->y = pShot->param_d[1];
        pShot->muki = DX_PI / 2.0;

        int cloudIdx = pShot->param_i[0];
        if (cloudIdx == warnCloud) {
            // 予兆窓の後半(落雷16フレーム前から)は高速フラッシュに切り替える
            bool fastFlash = warnPhaseFrame >= STRIKE_INTERVAL - 16;
            int  blinkLen = fastFlash ? 6 : 14;
            pShot->kind = ((warnPhaseFrame % blinkLen) < blinkLen / 2)
                ? img_enemyShotMediumBall[7]  // 黒
                : img_enemyShotMediumBall[6]; // 白
        }
        else {
            pShot->kind = img_enemyShotMediumBall[7]; // 通常時は黒
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 稲妻（ジグザグの短レーザー。発生直後は静止フラッシュし、
// 一定時間後に二次関数的に加速して画面外へ抜ける）
// param_d[0..1]=静止位置(x,y)  param_d[2]=フラッシュ後の脱出方向
// param_i[0]=静止フレーム数
// ============================================================
static void ShotLightningBolt(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        int    holdFrames = pShot->param_i[0];
        double fixedX = pShot->param_d[0];
        double fixedY = pShot->param_d[1];
        double exitMuki = pShot->param_d[2];

        if (t < holdFrames) {
            pShot->x = fixedX;
            pShot->y = fixedY;
        }
        else {
            double u = t - holdFrames;
            double dist = 0.6 * u * u;
            pShot->x = fixedX + dist * cos(exitMuki);
            pShot->y = fixedY + dist * sin(exitMuki);
        }
        // muki(向き)は生成時にセグメントの傾きとして固定済みのためここでは変更しない

        pShot = pShot->next;
    }
}

// ============================================================
// 着弾ショックバースト（着弾点から放射状に拡散するリング。
// 音速差を表現するため一瞬静止してから拡散する）
// param_d[0]=角度  param_d[1]=originX  param_d[2]=originY
// param_i[0]=静止(遅延)フレーム数
// ============================================================
static void ShotImpactBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        int    holdFrames = pShot->param_i[0];
        double angle = pShot->param_d[0];
        double ox = pShot->param_d[1];
        double oy = pShot->param_d[2];

        double u = (t < holdFrames) ? 0.0 : (t - holdFrames);
        double r = 5.0 * u;
        pShot->x = ox + r * cos(angle);
        pShot->y = oy + r * sin(angle);
        pShot->muki = angle;

        pShot = pShot->next;
    }
}

// ============================================================
// 自機狙い3way（着弾と同時に発射する追撃）
// param_d[0]=originX  param_d[1]=originY  param_d[2]=speed
// ============================================================
static void ShotAimedFromImpact(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double ox = pShot->param_d[0];
        double oy = pShot->param_d[1];
        double speed = pShot->param_d[2];

        pShot->x = ox + speed * t * cos(pShot->muki);
        pShot->y = oy + speed * t * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// ヘルパー: 新しいShotSetを1つ生成して登録し、ポインタを返す
// ============================================================
static sEnemyShotSet* CreateShotSet(double x, double y, double muki, int kind, sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
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

// ヘルパー: ShotSetの連結リストへ1発追加
static sEnemyShot* AddShot(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ============================================================
// 落雷イベントを1回発生させる(稲妻+着弾バースト+自機狙い3way)
// ============================================================
static void TriggerLightningStrike(int cloudIdx)
{
    double startX = CLOUD_X[cloudIdx];
    double startY = CLOUD_Y + 15.0;

    // 着弾点: 雲の直下を中心に乱数で少し散らす(リプレイ再現性のためGetRand使用)
    double landX = startX + (GetRand(120) - 60);
    if (landX < 20.0)  landX = 20.0;
    if (landX > 460.0) landX = 460.0;
    double landY = 420.0;

    if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

    // --- 稲妻本体: ジグザグの短レーザー6本 ---
    const int SEGMENT_NUM = 6;
    sEnemyShotSet* pBoltSet = CreateShotSet(startX, startY, 0.0, 0, ShotLightningBolt);
    double prevX = startX, prevY = startY;
    for (int i = 0; i < SEGMENT_NUM; i++) {
        double ratio = (double)(i + 1) / SEGMENT_NUM;
        double nx = startX + (landX - startX) * ratio + ((i % 2 == 0) ? 18.0 : -18.0);
        double ny = startY + (landY - startY) * ratio;

        double midX = (prevX + nx) * 0.5;
        double midY = (prevY + ny) * 0.5;
        double segAngle = atan2(ny - prevY, nx - prevX);

        sEnemyShot* pShot = AddShot(pBoltSet);
        pShot->kind = img_enemyShotLaser[6]; // 白
        pShot->muki = segAngle;
        pShot->speed = 0.0;
        pShot->param_d[0] = midX;
        pShot->param_d[1] = midY;
        pShot->param_d[2] = DX_PI / 2.0 + segAngle * 0.15; // ほぼ下方向へ脱出
        pShot->param_i[0] = 14; // 14フレーム静止フラッシュ

        prevX = nx;
        prevY = ny;
    }

    // --- 着弾ショックバースト: 32方向リング ---
    const int RING_NUM = 32;
    sEnemyShotSet* pRingSet = CreateShotSet(landX, landY, 0.0, 0, ShotImpactBurst);
    for (int i = 0; i < RING_NUM; i++) {
        double angle = DX_PI * 2.0 * i / RING_NUM;
        sEnemyShot* pShot = AddShot(pRingSet);
        pShot->kind = img_enemyShotDiamond[1]; // 黄(スパーク)
        pShot->muki = angle;
        pShot->speed = 0.0;
        pShot->param_d[0] = angle;
        pShot->param_d[1] = landX;
        pShot->param_d[2] = landY;
        pShot->param_i[0] = 6; // 雷鳴の遅れ(静止フレーム)
    }

    // --- 自機狙い3way ---
    sEnemyShotSet* pAimSet = CreateShotSet(landX, landY, 0.0, 0, ShotAimedFromImpact);
    double aimBase = atan2(player.y - landY, player.x - landX);
    for (int i = -1; i <= 1; i++) {
        double angle = aimBase + i * (DX_PI / 12.0);
        sEnemyShot* pShot = AddShot(pAimSet);
        pShot->kind = img_enemyShotBullet[1]; // 黄
        pShot->muki = angle;
        pShot->speed = 4.5;
        pShot->param_d[0] = landX;
        pShot->param_d[1] = landY;
        pShot->param_d[2] = 4.5;
    }
}

// ============================================================
// 雨粒を数発まとめて発生させる
// ============================================================
static void SpawnRainDrops(int num)
{
    sEnemyShotSet* pSet = CreateShotSet(0.0, 0.0, 0.0, 0, ShotRain);
    for (int i = 0; i < num; i++) {
        sEnemyShot* pShot = AddShot(pSet);
        pShot->kind = img_enemyShotSmallBall[4]; // 青(雨)
        pShot->speed = 0.0;
        pShot->param_d[0] = (double)GetRand(520) - 40.0;  // 初期x(画面外まで含む横幅)
        pShot->param_d[1] = -10.0 - (double)GetRand(30);  // 初期y(画面上端の外側)
        pShot->param_d[2] = (double)GetRand(628) / 100.0; // sin位相(0~2π相当)
    }
}

// ============================================================
// 雷雲を1つ生成する(常駐)
// ============================================================
static void SpawnCloud(int cloudIdx)
{
    sEnemyShotSet* pSet = CreateShotSet(0.0, 0.0, 0.0, 0, ShotCloud);
    for (int i = 0; i < PUFF_PER_CLOUD; i++) {
        sEnemyShot* pShot = AddShot(pSet);
        pShot->kind = img_enemyShotMediumBall[7]; // 黒(雲)
        pShot->speed = 0.0;
        double offset = (i - (PUFF_PER_CLOUD - 1) / 2.0) * 9.0;
        pShot->param_d[0] = CLOUD_X[cloudIdx] + offset;
        pShot->param_d[1] = CLOUD_Y + ((i % 2 == 0) ? 0.0 : 5.0);
        pShot->param_i[0] = cloudIdx;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_ThunderInRain_Claude()
{
    static bool strikeDone[STRIKE_NUM];

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 20.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        for (int i = 0; i < STRIKE_NUM; i++) strikeDone[i] = false;
        for (int i = 0; i < CLOUD_NUM; i++) SpawnCloud(i);
    }

    int localFrame = (count - 1) % CYCLE_LEN;
    if (localFrame == 0 && count != 1) {
        // 周回のたびに落雷済みフラグをリセット
        for (int i = 0; i < STRIKE_NUM; i++) strikeDone[i] = false;
    }

    // --- 雨(常時降り続ける。雷鳴・驟雨強化フェーズでは密度を上げる) ---
    bool intensify = (localFrame >= INTENSIFY_START);
    int  rainInterval = intensify ? 3 : 6;
    if (count % rainInterval == 1) {
        SpawnRainDrops(intensify ? 8 : 6);
    }

    // --- 雷雲予兆～落雷 ---
    if (localFrame >= STRIKE_PHASE_START && localFrame < STRIKE_PHASE_END) {
        int strikeIdx = (localFrame - STRIKE_PHASE_START) / STRIKE_INTERVAL;
        int strikeFrameInWindow = (localFrame - STRIKE_PHASE_START) % STRIKE_INTERVAL;

        if (strikeFrameInWindow == 0) {
            // 予兆窓の開始と同時に予告音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        if (strikeFrameInWindow == STRIKE_INTERVAL - 1 && !strikeDone[strikeIdx]) {
            int cloudIdx = strikeIdx % CLOUD_NUM;
            TriggerLightningStrike(cloudIdx);
            strikeDone[strikeIdx] = true;
        }
    }
}