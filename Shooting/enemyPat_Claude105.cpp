// enemyPat_TatakiwariRanbu.cpp
// 「叩き割り乱舞」 スイカ割りをモチーフにした4フェーズパターン
// 専用素材がないため、既存の大玉/小玉/短レーザーの組み合わせで
// スイカ本体（皮・果肉・種）とバットの目隠しスイングを表現する。
//
// フェーズ構成:
//   1. 実体成形: スイカ本体（緑大玉の二重リング=皮、赤小玉の同心円=果肉、
//      黒小玉=種）を中心に静止形成し、ゆっくり自転する
//   2. 目隠し徘徊: バット役の短レーザーが左右に揺れながら2回フェイントスイング
//      し、外れた位置に黄色の警告リング（歓声の「そっちだよ」演出）を出す
//   3. 命中・分裂: 本命の一撃で皮は左右に弧を描きつつ遅く、果肉・種は
//      凍結角度のまま放射状に速く飛散。同時に自機狙い3wayを発射
//   4. 歓声フィナーレ: 四隅で橙色の拍手リングが時間差発生した後、
//      自機狙い5wayで締める

// 0～1の範囲でtriangular/cosine bumpを返す（tがcenterから±halfWidth内でのみ非0）
static double Bump(double t, double center, double halfWidth)
{
    double d = t - center;
    if (d < -halfWidth || d > halfWidth) return 0.0;
    return 0.5 * (1.0 + cos(DX_PI * d / halfWidth));
}

// スイカ本体（皮・果肉・種）
static void ShotBody(sEnemyShotSet* pEnemyShotSet)
{
    const int    HIT_LOCAL = 439; // EnemyPat_Suikawari_Claude側のHIT(440)に同期（生成フレーム差1を考慮）
    const double ROT_SPEED = 0.003; // 静止フェーズの緩やかな自転

    if (pEnemyShotSet->count == 0) {
        // 皮（外周・大玉、緑、二重リングで厚みを出す）
        double rindRadii[] = { 70.0, 60.0 };
        int    rindCounts[] = { 28, 22 };
        for (int r = 0; r < 2; r++) {
            for (int i = 0; i < rindCounts[r]; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                double angle = 2.0 * DX_PI * i / rindCounts[r];
                pEnemyShot->param_d[0] = rindRadii[r];
                pEnemyShot->param_d[1] = angle;
                pEnemyShot->param_i[0] = 0; // 皮
                pEnemyShot->kind = img_enemyShotLargeBall[2]; // 緑

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 果肉（内側・小玉、赤、同心円状に敷き詰める）
        double fleshRadii[] = { 12.0, 20.0, 28.0, 36.0, 44.0, 52.0 };
        int    fleshCounts[] = { 6, 12, 18, 24, 30, 36 };
        for (int r = 0; r < 6; r++) {
            for (int i = 0; i < fleshCounts[r]; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                double angle = 2.0 * DX_PI * i / fleshCounts[r] + 0.3 * r; // 層ごとに位相をずらす
                pEnemyShot->param_d[0] = fleshRadii[r];
                pEnemyShot->param_d[1] = angle;
                pEnemyShot->param_i[0] = 1; // 果肉
                pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 種（黒・小玉、リプレイ安全なGetRandでランダム配置）
        for (int i = 0; i < 24; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double radius = 8.0 + GetRand(4400) / 100.0;              // 8.0～52.0
            double angle = 2.0 * DX_PI * GetRand(3600) / 3600.0;      // 0～2π
            pEnemyShot->param_d[0] = radius;
            pEnemyShot->param_d[1] = angle;
            pEnemyShot->param_i[0] = 2; // 種
            pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double baseRadius = pShot->param_d[0];
        double baseAngle = pShot->param_d[1];

        if (pEnemyShotSet->count < HIT_LOCAL) {
            // 静止フェーズ：緩やかに自転するだけ
            double angle = baseAngle + ROT_SPEED * pEnemyShotSet->count;
            pShot->x = pEnemyShotSet->x + baseRadius * cos(angle);
            pShot->y = pEnemyShotSet->y + baseRadius * sin(angle);
            pShot->muki = angle;
        }
        else {
            // 命中後：凍結角度から放射状に加速飛散
            double elapsed = pEnemyShotSet->count - HIT_LOCAL;
            double frozenAngle = baseAngle + ROT_SPEED * HIT_LOCAL;

            if (pShot->param_i[0] == 0) {
                // 皮：やや遅く、左右に弧を描きながら開く（パカッと割れる動き）
                double side = (cos(frozenAngle) >= 0.0) ? 1.0 : -1.0;
                double angle = frozenAngle + side * 0.00025 * elapsed * elapsed;
                double r = baseRadius + 1.6 * elapsed;
                pShot->x = pEnemyShotSet->x + r * cos(angle);
                pShot->y = pEnemyShotSet->y + r * sin(angle);
                pShot->muki = angle;
            }
            else {
                // 果肉・種：速く放射状に飛散（種はさらに一段速い）
                double speed = (pShot->param_i[0] == 2) ? 3.6 : 3.0;
                double r = baseRadius + speed * elapsed + 0.02 * elapsed * elapsed;
                pShot->x = pEnemyShotSet->x + r * cos(frozenAngle);
                pShot->y = pEnemyShotSet->y + r * sin(frozenAngle);
                pShot->muki = frozenAngle;
            }
        }

        pShot = pShot->next;
    }
}

// バット役（目隠し徘徊→2回フェイント→本命ヒット→振り抜け）
static void ShotBat(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->kind = img_enemyShotLaser[7]; // 黒（バットのシルエット）

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count;
        const double topY = 40.0;
        const double centerX = 240.0;
        const double dipFeint = 110.0; // 中心(140)まで届かない＝空振り
        const double dipHit = 148.0;   // 中心を超えて着弾を表現

        double y = topY
            + (dipFeint - topY) * Bump(t, 120.0, 45.0)
            + (dipFeint - topY) * Bump(t, 280.0, 45.0)
            + (dipHit - topY) * Bump(t, 440.0, 55.0);

        double swingStraighten = 1.0 - Bump(t, 440.0, 90.0); // 本命に向けて振れ幅を収束
        double x = centerX + 80.0 * sin(t * 0.015) * swingStraighten;

        if (t >= 470.0) {
            // 命中後：そのまま振り抜けて画面下へ
            y = dipHit + (t - 470.0) * 5.0;
            x = centerX + 20.0 * sin(t * 0.04);
        }

        pShot->x = x;
        pShot->y = y;
        pShot->muki = DX_PI / 2.0; // 振り下ろし方向

        pShot = pShot->next;
    }
}

// 汎用リングバースト（警告の歓声リング／拍手の紙吹雪リング兼用）
// pEnemyShotSet->kind = 色番号, param_i[0] = 弾数, param_d[0] = 初期半径, param_d[1] = 拡散速度
static void ShotRingBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int n = pEnemyShotSet->param_i[0];
        for (int i = 0; i < n; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double angle = 2.0 * DX_PI * i / n;
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->kind = img_enemyShotSmallBall[pEnemyShotSet->kind];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double angle = pShot->param_d[0];
        double r = pEnemyShotSet->param_d[0] + pEnemyShotSet->param_d[1] * pShot->count;
        pShot->x = pEnemyShotSet->x + r * cos(angle);
        pShot->y = pEnemyShotSet->y + r * sin(angle);
        pShot->muki = angle;
        pShot = pShot->next;
    }
}

static void SpawnRingBurst(double x, double y, int n, double startRadius, double expandSpeed, int colorIndex)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotRingBurst;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->kind = colorIndex;
    pEnemyShotSet->param_i[0] = n;
    pEnemyShotSet->param_d[0] = startRadius;
    pEnemyShotSet->param_d[1] = expandSpeed;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 汎用自機狙いNway（3way/5way兼用）
// pEnemyShotSet->kind = 色番号, param_i[0] = way数, param_d[0] = 速さ, param_d[1] = 間隔角度
static void ShotAimedFan(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int n = pEnemyShotSet->param_i[0];
        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double spacing = pEnemyShotSet->param_d[1];
        for (int i = 0; i < n; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double angle = baseAngle + spacing * (i - (n - 1) / 2.0);
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->muki = angle;
            pEnemyShot->kind = img_enemyShotSmallBall[pEnemyShotSet->kind];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double angle = pShot->param_d[0];
        double speed = pEnemyShotSet->param_d[0];
        pShot->x = pEnemyShotSet->x + speed * pShot->count * cos(angle);
        pShot->y = pEnemyShotSet->y + speed * pShot->count * sin(angle);
        pShot = pShot->next;
    }
}

static void SpawnAimedFan(double x, double y, int n, double spacing, double speed, int colorIndex)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotAimedFan;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->kind = colorIndex;
    pEnemyShotSet->param_i[0] = n;
    pEnemyShotSet->param_d[0] = speed;
    pEnemyShotSet->param_d[1] = spacing;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 敵本体のパターン
void EnemyPat_Suikawari_Claude()
{
    const int FEINT1 = 120;
    const int FEINT2 = 280;
    const int HIT = 440;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 140.0; // スイカ本体の中心
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }

    const int T = 630;
    int countT = count % T;

    if (countT == 1) {
        // スイカ本体（皮・果肉・種）
        sEnemyShotSet* pBody = new sEnemyShotSet;
        pBody->count = 0;
        pBody->patternFunc = ShotBody;
        pBody->x = enemy.x;
        pBody->y = enemy.y;

        pBody->pEnemyShotHead = new sEnemyShot;
        pBody->pEnemyShotHead->prev = pBody->pEnemyShotHead;
        pBody->pEnemyShotHead->next = pBody->pEnemyShotHead;

        pBody->prev = enemyShotSetHead.prev;
        pBody->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBody;
        enemyShotSetHead.prev = pBody;

        // バット（目隠し徘徊→フェイント→本命ヒット）
        sEnemyShotSet* pBat = new sEnemyShotSet;
        pBat->count = 0;
        pBat->patternFunc = ShotBat;
        pBat->x = 0.0;
        pBat->y = 0.0; // ShotBat内で毎フレーム座標を式から直接計算するため未使用

        pBat->pEnemyShotHead = new sEnemyShot;
        pBat->pEnemyShotHead->prev = pBat->pEnemyShotHead;
        pBat->pEnemyShotHead->next = pBat->pEnemyShotHead;

        pBat->prev = enemyShotSetHead.prev;
        pBat->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBat;
        enemyShotSetHead.prev = pBat;
    }

    // フェイントの警告リング（外れた位置に「そっちだよ」演出、黄）
    if (countT == FEINT1) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        SpawnRingBurst(enemy.x - 90.0, enemy.y - 20.0, 16*2, 10.0, 1.6, 1);
    }
    if (countT == FEINT2) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        SpawnRingBurst(enemy.x + 90.0, enemy.y - 20.0, 16*2, 10.0, 1.6, 1);
    }

    // 命中の瞬間：自機狙い3way（赤）
    if (countT == HIT) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        SpawnAimedFan(enemy.x, enemy.y, 3*3, 12.0 * DX_PI / 180.0, 3.0, 0);
    }

    // 拍手（四隅の紙吹雪リング、橙、時間差発生）
    if (countT == HIT + 40)  SpawnRingBurst(40.0, 40.0, 14*3, 6.0, 1.3, 8);
    if (countT == HIT + 60)  SpawnRingBurst(440.0, 40.0, 14*3, 6.0, 1.3, 8);
    if (countT == HIT + 80)  SpawnRingBurst(40.0, 440.0, 14*3, 6.0, 1.3, 8);
    if (countT == HIT + 100) SpawnRingBurst(440.0, 440.0, 14*3, 6.0, 1.3, 8);

    // フィナーレ：自機狙い5way（赤）
    if (countT == HIT + 160) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        SpawnAimedFan(enemy.x, enemy.y, 5*3, 10.0 * DX_PI / 180.0, 3.4, 0);
    }
}