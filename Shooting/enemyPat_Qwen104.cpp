// enemyPat_Tmp.cpp

// 弾幕：ブラジリアンワックス・ストリップ
static void ShotBrazilianWax(sEnemyShotSet* pEnemyShotSet)
{
    // count == 0 の時に初期化処理を行う
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // Phase 1: ワックス塗布 (黄色の小玉を下方へばら撒く)
        for (int i = 0; i < 12 * 2; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            // GetRand(200) は 0〜200 を返すので、-100 して -100〜100 の範囲にする
            pShot->x = pEnemyShotSet->x + (GetRand(200) - 100);
            pShot->y = pEnemyShotSet->y;

            // GetRand(40) は 0〜40 を返すので、-20 して -20〜20 の範囲にする
            pShot->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
            pShot->speed = 1.5 + GetRand(10) / 10.0 + 8; // 遅い速度

            // 1:黄 の小玉 (ワックス)
            pShot->kind = img_enemyShotSmallBall[1];

            // param_i[0]: 状態管理 (0:落下中, 1:固着中, 2:剥離発射中, 3:ストリップ)
            pShot->param_i[0] = 0;
            // param_d[0]: 固着するY座標 (画面下部 360〜440 の範囲)
            pShot->param_d[0] = 360.0 + GetRand(80);

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // Phase 3: 一気剥離トリガー (pEnemyShotSet->param_i[0] で1回だけ実行を保証)
    if (pEnemyShotSet->count == 120 && pEnemyShotSet->param_i[0] == 0) {
        pEnemyShotSet->param_i[0] = 1; // 剥離処理実行済みフラグ

        // 剥離音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 画面内の全ての弾を確認し、剥離処理を行う
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                // 固着していたワックス弾を剥離発射
                pShot->param_i[0] = 2;
                double target_muki = atan2(player.y - pShot->y, player.x - pShot->x);
                // GetRand(60) は 0〜60 を返すので、-30 して -30〜30 のブレ幅にする
                pShot->muki = target_muki + (GetRand(60) - 30) / 180.0 * DX_PI;
                pShot->speed = 2.0; // 初速は遅く、次第に加速する

                // 5:マゼンタ の小玉 (剥がれた刺激/痛みを表現)
                pShot->kind = img_enemyShotSmallBall[5];
            }
            else if (pShot->param_i[0] == 3) {
                // ストリップ弾を高速で左右に飛ばす (剥がす動作)
                pShot->speed = 15.0;
            }
            pShot = pShot->next;
        }
    }

    // 各弾の更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // Phase 1: 落下中
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 指定Y座標に到達したら「固まる」
            if (pShot->y >= pShot->param_d[0]) {
                pShot->y = pShot->param_d[0];
                pShot->speed = 0.0;
                pShot->param_i[0] = 1; // 固着状態へ移行
                // 8:橙 の小玉 (固まったワックスを表現)
                pShot->kind = img_enemyShotSmallBall[8];
            }
        }
        else if (pShot->param_i[0] == 1) {
            // Phase 2: 固着中
            // pEnemyShotSet->count == 60 でストリップを貼る
            if (pEnemyShotSet->count == 60) {
                sEnemyShot* pStrip = new sEnemyShot;
                pStrip->x = pShot->x;
                pStrip->y = pShot->y;
                pStrip->muki = 0.0; // 横向き
                pStrip->speed = 0.8; // ゆっくりスライド

                // 6:白 の短レーザー (ストリップ)
                pStrip->kind = img_enemyShotLaser[6];
                pStrip->param_i[0] = 3; // ストリップ状態
                pStrip->param_i[1] = 1; // 右方向
                // GetRand(1) は 0 または 1 を返す
                if (GetRand(1) == 0) {
                    pStrip->param_i[1] = -1; // 左方向もあり
                }

                pStrip->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pStrip->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pStrip;
                pEnemyShotSet->pEnemyShotHead->prev = pStrip;
            }
        }
        else if (pShot->param_i[0] == 2) {
            // Phase 3: 剥離発射中 (加速しながら飛ぶ)
            pShot->speed += 0.25; // 徐々に加速
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] == 3) {
            // ストリップの移動
            pShot->x += pShot->speed * pShot->param_i[1];
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_BrazilianWax_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右の往復移動
        enemy.x += 1.2 * (double)muki;
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            muki *= -1;
        }
    }

    // 一定間隔（約3秒）で弾幕セットを生成
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBrazilianWax;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = DX_PI / 2.0; // 下向き基準
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->param_i[0] = 0; // 剥離トリガーフラグ初期化

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}