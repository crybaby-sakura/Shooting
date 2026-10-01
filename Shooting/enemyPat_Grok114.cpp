// enemyPat_Tmp.cpp
// 流符「夜空のカーペット」
// 夜空を敷き詰めるように流れる星の絨毯弾幕

// ------------------------------------------------------------
// パターン1: 流れる星の帯（カーペットの一層）
// 画面幅いっぱいに星を並べ、ゆっくり下降しながら横に波打つ
// ------------------------------------------------------------
static void ShotNightCarpetBand(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ弾を生成
    if (pEnemyShotSet->count == 0) {
        // 予告音は控えめに
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 帯の本数と密度（kindでバリエーション）
        const int bandCount = 1 + (pEnemyShotSet->kind % 3); // 1〜3本の帯
        const int starsPerBand = 18 + (pEnemyShotSet->kind % 5); // 18〜22個程度

        for (int b = 0; b < bandCount; b++) {
            double baseY = pEnemyShotSet->y + b * 18.0;
            double phaseOffset = b * 1.2;

            for (int i = 0; i < starsPerBand; i++) {
                pEnemyShot = new sEnemyShot;

                // 画面幅をほぼ覆うように配置（少しランダム）
                double t = (double)i / (starsPerBand - 1);
                pEnemyShot->x = 00.0 + t * 480.0 + (GetRand(16) - 8);
                pEnemyShot->y = baseY + (GetRand(10) - 5);

                // 初期角度はほぼ真下（少し散らす）
                pEnemyShot->muki = DX_PI / 2.0 + (GetRand(30) - 15) / 180.0 * DX_PI;

                // 速度は遅め〜中程度（層によって差をつける）
                pEnemyShot->speed = 1.1 + b * 0.25 + GetRand(40) / 100.0;

                // 色と弾種を夜空らしく選択
                // 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
                int color;
                switch ((pEnemyShotSet->kind + i + b) % 5) {
                case 0: color = 4; break; // 青
                case 1: color = 3; break; // シアン
                case 2: color = 6; break; // 白
                case 3: color = 5; break; // マゼンタ
                default: color = 3; break; // シアン
                }

                // 弾種：小玉・菱形・鱗を中心に星っぽく
                switch ((i + b + pEnemyShotSet->kind) % 4) {
                case 0:
                    pEnemyShot->kind = img_enemyShotSmallBall[color];
                    break;
                case 1:
                    pEnemyShot->kind = img_enemyShotDiamond[color];
                    break;
                case 2:
                    pEnemyShot->kind = img_enemyShotScale[color];
                    break;
                default:
                    pEnemyShot->kind = img_enemyShotSmallBall[color];
                    break;
                }

                // 波打ち用パラメータを保存
                pEnemyShot->param_d[0] = phaseOffset;          // 位相オフセット
                pEnemyShot->param_d[1] = 0.035 + b * 0.008;    // 波の周波数
                pEnemyShot->param_d[2] = 1.2 + b * 0.4;        // 波の振幅
                pEnemyShot->param_d[3] = pEnemyShot->x;        // 基準X
                pEnemyShot->param_i[0] = b;                    // 層番号

                // リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレーム移動（波打ち下降）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本は下方向
        pShot->y += pShot->speed;

        // 横方向に正弦波で揺らす（カーペットのうねり）
        double phase = pShot->param_d[0] + pShot->count * pShot->param_d[1];
        pShot->x = pShot->param_d[3] + sin(phase) * pShot->param_d[2];

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// パターン2: 放射状に広がる星の粒（カーペットの隙間埋め・アクセント）
// ------------------------------------------------------------
static void ShotNightStarScatter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int num = 12 + (pEnemyShotSet->kind % 6); // 12〜17発

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x + (GetRand(60) - 30);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(20) - 10);

            // ほぼ全方向に散らすが、下寄りを少し多めに
            double baseAngle = (double)i / num * 2.0 * DX_PI;
            pEnemyShot->muki = baseAngle + (GetRand(40) - 20) / 180.0 * DX_PI;

            pEnemyShot->speed = 1.6 + GetRand(120) / 100.0;

            // 夜空カラー
            int color = 3 + (i % 4); // シアン〜白寄り
            if (color > 6) color = 6;

            // 小さめの弾で星を表現
            if (i % 3 == 0) {
                pEnemyShot->kind = img_enemyShotDiamond[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[color];
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 単純に直進
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// パターン3: 天の川風の斜め流星帯（カーペットに流星を混ぜる）
// ------------------------------------------------------------
static void ShotNightMeteorStream(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 斜めに流れる一筋の流星
        const int num = 8 + (pEnemyShotSet->kind % 5) + 25;

        // 左右どちらから流すか
        int dir = (pEnemyShotSet->kind % 2 == 0) ? 1 : -1;
        double startX = (dir > 0) ? -20.0 : 500.0;
        double angle = (dir > 0) ? (DX_PI / 2.0 + 0.45) : (DX_PI / 2.0 - 0.45);

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = startX + dir * i * 12.0;
            pEnemyShot->y = pEnemyShotSet->y - 30.0 + i * 8.0;

            pEnemyShot->muki = angle + (GetRand(10) - 5) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.8 + GetRand(60) / 100.0;

            // 流星らしく白・シアン・黄を混ぜる
            int color;
            switch (i % 3) {
            case 0: color = 6; break; // 白
            case 1: color = 3; break; // シアン
            default: color = 1; break; // 黄
            }

            if (i % 2 == 0) {
                pEnemyShot->kind = img_enemyShotScale[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotBullet[color];
            }

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
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// 流符「夜空のカーペット」
// ------------------------------------------------------------
void EnemyPat_NightCarpet_Grok()
{
    static int moveDir;
    static int carpetPhase;
    static int shotKind;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        moveDir = 1;
        carpetPhase = 0;
        shotKind = 0;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x += 0.7 * (double)moveDir;
        if (enemy.x > 320.0) moveDir = -1;
        if (enemy.x < 160.0) moveDir = 1;

        // わずかに上下にも呼吸するように動く
        enemy.y = 60.0 + sin(count * 0.025) * 8.0;
    }

    // --- メインのカーペット帯を定期的に展開 ---
    // 約1.2秒ごとに新しい帯を生成
    if (count >= 30 && (count - 30) % 72 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNightCarpetBand;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = shotKind++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // --- アクセントの散弾（星の粒）を時々混ぜる ---
    if (count >= 90 && (count - 90) % 95 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNightStarScatter;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - (enemy.y + 10.0), player.x - enemy.x);
        pEnemyShotSet->kind = shotKind++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // --- 流星帯を左右交互に流す ---
    if (count >= 150 && (count - 150) % 110 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNightMeteorStream;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shotKind++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}