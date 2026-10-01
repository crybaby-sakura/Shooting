// enemyPat_InbachiBanshogurenjin.cpp
// 隠しボス「陰蜂」風パターン：万象紅蓮陣（ばんしょうぐれんじん）
//
// フェーズ1：回転花弁弾幕（カール弾で花弁を形成、往復スライドしつつ交互回転）
// フェーズ2：追尾針弾の割り込み（自機狙い扇状の高速弾）
// フェーズ3：中大弾のランダム乱射（画面全体・終盤ほど減速）
// 上記3フェーズを1サイクル（420フレーム）として無限に周回する。

// 弾の色（0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙）
static constexpr int kColorRed = 0;
static constexpr int kColorCyan = 3;
static constexpr int kColorBlack = 7;
static constexpr int kColorOrange = 8;

// 1サイクルの長さ（フレーム数）。この周期でフェーズ1〜3を繰り返す。
static constexpr int kCycleLength = 420;

// ============================================================
// ショットセット生成／弾追加の共通処理（本ファイル内のみで使用）
// ============================================================
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
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

    return pEnemyShotSet;
}

static sEnemyShot* AppendShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// ============================================================
// フェーズ1：回転花弁弾幕（カール弾）
// 8方向×6発の花弁を、param_d[0]に格納した角速度でカールさせて展開する。
// param_i[0]を経過フレーム数として使い、40フレームでカールを止めて直進に
// 切り替えることで、確実に画面外へ抜けさせる（花弁が開いて弾け飛ぶ見た目にもなる）。
// pEnemyShotSet->kind の偶奇で回転方向と色（赤/黒）を交互に切り替える。
// ============================================================
static constexpr int kPetalDirections = 8;
static constexpr int kPetalBulletsPerDirection = 6;
static constexpr int kPetalCurlFrames = 40;

static void ShotPetalCurl(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double curlSign = (pEnemyShotSet->kind % 2 == 0) ? 1.0 : -1.0;
        int color = (pEnemyShotSet->kind % 2 == 0) ? kColorRed : kColorBlack;

        for (int dir = 0; dir < kPetalDirections; dir++) {
            double baseMuki = pEnemyShotSet->muki + dir * (2.0 * DX_PI / kPetalDirections);

            for (int j = 0; j < kPetalBulletsPerDirection; j++) {
                sEnemyShot* pEnemyShot = AppendShot(pEnemyShotSet);

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = baseMuki;
                pEnemyShot->speed = 1.8 + GetRand(40) / 100.0; // 1.8〜2.2
                pEnemyShot->kind = img_enemyShotScale[color];

                // 花弁の広がり：内側（j小）ほどカールが弱く、外側ほど強い
                pEnemyShot->param_d[0] = curlSign * (0.0015 + j * 0.0012);
                // param_i[0] は経過フレーム数カウンタとして使用（0初期化済み）
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->param_i[0]++;
        if (pShot->param_i[0] > kPetalCurlFrames) {
            pShot->param_d[0] = 0.0; // カール終了、以降は直進して画面外へ抜ける
        }

        pShot->muki += pShot->param_d[0];
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// フェーズ2：追尾針弾の割り込み
// 発射時点の自機狙い角を中心に±30度へ扇状に高速弾をばら撒く。
// ============================================================
static constexpr int kNeedleCount = 14;
static constexpr double kNeedleSpreadDeg = 30.0;

static void ShotNeedleFan(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double aimMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double spreadRad = kNeedleSpreadDeg * DX_PI / 180.0;

        for (int i = 0; i < kNeedleCount; i++) {
            sEnemyShot* pEnemyShot = AppendShot(pEnemyShotSet);

            double t = (double)i / (kNeedleCount - 1) - 0.5; // -0.5〜+0.5
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = aimMuki + t * spreadRad * 2.0;
            pEnemyShot->speed = 7.0;
            pEnemyShot->kind = img_enemyShotBullet[kColorCyan];
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// フェーズ3：中大弾のランダム乱射
// 画面全体にランダム配置し、発生が後半のフレームになるほど弾速を落とすことで
// 締めの瞬間に「見て避けられる」余地を残す（緩急のコントラスト）。
// ============================================================
static constexpr int kRandomBurstFrames = 18;   // 発生をかけるフレーム数
static constexpr int kRandomBulletsPerFrame = 4;

static void ShotRandomBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count < kRandomBurstFrames) {
        double progress = (double)pEnemyShotSet->count / (kRandomBurstFrames - 1); // 0.0〜1.0
        double speed = 3.5 - progress * 1.7; // 3.5(序盤)→1.8(終盤)へ減速

        for (int i = 0; i < kRandomBulletsPerFrame; i++) {
            sEnemyShot* pEnemyShot = AppendShot(pEnemyShotSet);

            pEnemyShot->x = GetRand(480);
            pEnemyShot->y = GetRand(480);
            pEnemyShot->muki = GetRand(3600) / 3600.0 * 2.0 * DX_PI; // 0〜2πのランダム方向
            pEnemyShot->speed = speed;
            pEnemyShot->kind = img_enemyShotMediumBall[kColorOrange];
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体：万象紅蓮陣
// ============================================================
void EnemyPat_Inbachi_Claude()
{
    static int slideDir;
    static int petalKind;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        slideDir = 1;
        petalKind = 0;
    }
    else {
        // ゆったりと左右にスライド（フェーズ1の花弁展開に合わせて180フレーム周期で反転）
        enemy.x += 0.5 * (double)slideDir;
        if ((count % 180) == 90) slideDir *= -1;
    }

    int c = (count - 1) % kCycleLength;

    // フェーズ1：花弁バースト（0〜179フレーム、20フレーム毎に発射、kindの偶奇で交互回転）
    if (c < 180 && (c % 20) == 0) {
        double muki = atan2(player.y - enemy.y, player.x - enemy.x);
        CreateShotSet(ShotPetalCurl, enemy.x, enemy.y + 10.0, muki, petalKind);
        petalKind++;
    }

    // フェーズ2：針弾の割り込み（120フレーム目、1回のみ）
    if (c == 120) {
        CreateShotSet(ShotNeedleFan, enemy.x, enemy.y + 10.0, 0.0, 0);
    }

    // フェーズ3：中大弾のランダム乱射（300フレーム目に開始、締めのあと次サイクルへ）
    if (c == 300) {
        CreateShotSet(ShotRandomBurst, enemy.x, enemy.y + 10.0, 0.0, 0);
    }
}