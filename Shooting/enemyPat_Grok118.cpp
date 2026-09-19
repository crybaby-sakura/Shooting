// 弾幕パターン：『竹やぶ焼けた』
// 既存弾のみを組み合わせて実装
// 竹の幹   : 緑の菱形弾・鱗弾を縦に積む
// 燃焼部分 : 橙・赤の小玉が下から這い上がる
// 破裂     : 中玉＋小玉の放射
// 最終崩壊 : 細かい弾の雨＋自機狙い

// ---------------------------------------------------------------
// 内部用：弾をリストに追加するヘルパー
// ---------------------------------------------------------------
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->count = 0;
    // paramは0クリア済み（構造体初期化）
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// ---------------------------------------------------------------
// 竹一本分のショットセット
// param_i[0] : 竹の番号（0〜4）
// param_i[1] : フェーズ (0:形成 1:燃焼中 2:破裂開始済み)
// param_d[0] : 竹の基準X
// param_d[1] : 竹の先端Y（上端）
// param_d[2] : 竹の長さ
// ---------------------------------------------------------------
static void ShotBamboo(sEnemyShotSet* pSet)
{
    const int BAMBOO_SEGMENTS = 14;          // 竹の節の数
    const double SEG_HEIGHT = 18.0;        // 節の間隔
    const double BASE_Y = 460.0;       // 竹の根本（画面下寄り）

    // 初回のみ竹を形成
    if (pSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double baseX = pSet->param_d[0];
        double topY = BASE_Y - BAMBOO_SEGMENTS * SEG_HEIGHT;
        pSet->param_d[1] = topY;
        pSet->param_d[2] = BAMBOO_SEGMENTS * SEG_HEIGHT;
        pSet->param_i[1] = 0;   // フェーズ：形成

        // 緑の菱形弾・鱗弾で竹の幹を作る（速度0で固定）
        for (int i = 0; i < BAMBOO_SEGMENTS; i++) {
            double y = BASE_Y - i * SEG_HEIGHT;
            int col = (i % 3 == 0) ? 2 : 3;   // 緑とシアンを混ぜる
            int imgKind = (i % 2 == 0) ? img_enemyShotDiamond[col] : img_enemyShotScale[col];
            sEnemyShot* p = AddShot(pSet, baseX + (GetRand(6) - 3), y, -DX_PI / 2.0, 0.0, imgKind);
            // param_i[0] に「これは竹の幹」フラグ、param_i[1] に節番号を入れておく
            p->param_i[0] = 1;          // 幹フラグ
            p->param_i[1] = i;          // 節番号（下から0）
            p->param_d[0] = baseX;      // 基準Xを記憶
        }
        return;
    }

    // 燃焼フェーズ開始（形成後少し待つ）
    if (pSet->count == 40 && pSet->param_i[1] == 0) {
        pSet->param_i[1] = 1;   // 燃焼開始
    }

    // 燃焼中：下から橙・赤の小玉を這い上がらせる
    if (pSet->param_i[1] == 1) {
        // 一定間隔で炎弾を生成
        if (pSet->count % 6 == 0) {
            double baseX = pSet->param_d[0];
            // 根本付近から上方向へ
            int col = (GetRand(1) == 0) ? 8 : 0;   // 橙 or 赤
            sEnemyShot* fire = AddShot(pSet,
                baseX + (GetRand(8) - 4),
                BASE_Y + 10.0,
                -DX_PI / 2.0,                   // 真上
                3.2 + GetRand(80) / 100.0,
                img_enemyShotSmallBall[col]);
            fire->param_i[0] = 2;               // 炎フラグ
        }

        // 炎弾が一定の高さに達したら破裂を起こす
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 2) {   // 炎弾
                // 節の高さに近づいたら破裂トリガー
                int seg = (int)((BASE_Y - p->y) / SEG_HEIGHT);
                if (seg >= 0 && seg < BAMBOO_SEGMENTS && p->param_i[2] == 0) {
                    // この炎弾は破裂済みにする
                    p->param_i[2] = 1;

                    // 破裂音
                    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                    // 放射弾（中玉）
                    int num = 10 + GetRand(4)-10;
                    for (int k = 0; k < num; k++) {
                        double ang = (DX_PI * 2.0 * k) / num + (GetRand(20) - 10) / 180.0 * DX_PI;
                        double spd = 1.8 + GetRand(120) / 100.0;
                        int col = (k % 3 == 0) ? 0 : (k % 3 == 1) ? 8 : 1; // 赤・橙・黄
                        sEnemyShot* frag = AddShot(pSet, p->x, p->y, ang, spd, img_enemyShotMediumBall[col]);
                        frag->param_i[0] = 3;   // 破片フラグ
                    }
                    // 細かい破片も追加
                    for (int k = 0; k < 6-4; k++) {
                        double ang = GetRand(360) / 180.0 * DX_PI;
                        double spd = 2.5 + GetRand(100) / 100.0;
                        AddShot(pSet, p->x, p->y, ang, spd, img_enemyShotSmallBall[GetRand(1) == 0 ? 0 : 8]);
                    }

                    // 対応する幹の節を外側へ吹き飛ばす
                    sEnemyShot* stem = pSet->pEnemyShotHead->next;
                    while (stem != pSet->pEnemyShotHead) {
                        if (stem->param_i[0] == 1 && stem->param_i[1] == seg) {
                            stem->speed = 2.5 + GetRand(100) / 100.0;
                            stem->muki = (GetRand(1) == 0 ? -1.0 : 1.0) * (0.6 + GetRand(40) / 100.0);
                            stem->param_i[0] = 4;  // 吹き飛ばされた幹
                            break;
                        }
                        stem = stem->next;
                    }
                }
            }
            p = p->next;
        }

        // 燃焼が十分進んだらフェーズ移行
        if (pSet->count > 160) {
            pSet->param_i[1] = 2;
        }
    }

    // 最終崩壊フェーズ
    if (pSet->param_i[1] == 2 && pSet->count == 170) {
        // 残っている幹を一斉に散らす
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 1) {   // まだ残っている幹
                p->speed = 1.5 + GetRand(150) / 100.0;
                p->muki = (GetRand(120) - 60) / 180.0 * DX_PI - DX_PI / 2.0;
                p->param_i[0] = 4;
            }
            p = p->next;
        }
        // 追加の雨弾
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 18; i++) {
            double x = pSet->param_d[0] + GetRand(60) - 30;
            double y = pSet->param_d[1] - 20.0;
            double ang = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;  // 下方向寄り
            double spd = 2.0 + GetRand(120) / 100.0;
            int col = (GetRand(2) == 0) ? 2 : 3;
            AddShot(pSet, x, y, ang, spd, img_enemyShotSmallBall[col]);
        }
    }

    // 毎フレーム：弾の移動（共通）
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------
// 敵本体パターン
// ---------------------------------------------------------------
void EnemyPat_TakeyabuYaketa_Grok()
{
    static int swayDir;
    static int bambooSpawned;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        swayDir = 1;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x += 0.7 * swayDir;
        if (enemy.x < 100.0 || enemy.x > 380.0) {
            swayDir *= -1;
        }
    }

    const int T = 450;
    int countT = count % T;

    if (countT == 1) {
        bambooSpawned = 0;
    }

    // 竹を5本、時間差で生やす
    if (bambooSpawned < 5 && countT == 20 + bambooSpawned * 25) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBamboo;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = bambooSpawned;

        // 竹の基準X座標（画面をバランスよく配置）
        const double positions[5] = { 80.0, 160.0, 240.0, 320.0, 400.0 };
        pSet->param_d[0] = positions[bambooSpawned];
        pSet->param_i[0] = bambooSpawned;
        pSet->param_i[1] = 0;

        // リスト初期化
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // ショットセットを敵のリストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        bambooSpawned++;
    }
}