// enemyPat_Tmp.cpp
// ブラジリアンワックスをモチーフにした弾幕パターン
// 細い短レーザー = 毛
// 中玉 = ワックス
// 小玉 = 剥がした後の痛み・飛び散り

// 弾幕：ブラジリアンワックス
static void ShotWax(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 開始時：予告音
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // -----------------------------------------------
    // フェーズ1：毛が生える（count 0〜59）
    // 細い短レーザーを密集して固定配置（speed=0でその場に留める）
    // -----------------------------------------------
    if (pEnemyShotSet->count < 60) {
        if (pEnemyShotSet->count % 4 == 0) {
            // 軽めの発射音
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            // 6本ずつ生やす
            for (int i = 0; i < 6 / 6; i++) {
                pEnemyShot = new sEnemyShot;
                // 中央付近に密集（幅約80）
                pEnemyShot->x = pEnemyShotSet->x + (GetRand(240+120) - 120-60);
                // 縦に少しばらけさせて毛の生えている感じを出す
                pEnemyShot->y = pEnemyShotSet->y + 25.0 + GetRand(55);
                pEnemyShot->muki = DX_PI / 2.0;          // 下向き（後で上に変える）
                pEnemyShot->speed = 0.0;                 // 固定（生えている状態）
                // 黒の短レーザーを毛に見立てる
                pEnemyShot->kind = img_enemyShotLaser[7];
                pEnemyShot->param_i[0] = 0;              // 0 = 毛

                // リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // -----------------------------------------------
    // フェーズ2：ワックスを塗る（count 60〜99）
    // 中玉を毛の上に被せるように配置（やはりspeed=0）
    // -----------------------------------------------
    if (pEnemyShotSet->count >= 60 && pEnemyShotSet->count < 100) {
        if (pEnemyShotSet->count % 5 == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            // 4個ずつ塗布
            for (int i = 0; i < 4/2; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + (GetRand(260+120) - 130-60);
                pEnemyShot->y = pEnemyShotSet->y + 20.0 + GetRand(50);
                pEnemyShot->muki = DX_PI / 2.0;
                pEnemyShot->speed = 0.0;                 // 固定
                // 白の中玉をワックスに見立てる
                pEnemyShot->kind = img_enemyShotMediumBall[3];
                pEnemyShot->param_i[0] = 1;              // 1 = ワックス

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // -----------------------------------------------
    // フェーズ3：一気に剥がす（count == 100）
    // 毛とワックスを上方向へ高速で飛ばす + 痛みの小玉を散らす
    // -----------------------------------------------
    if (pEnemyShotSet->count == 100) {
        // 重い発射音で「ビリッ」感を出す
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 既存の毛・ワックス弾をすべて上方向へ加速
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
                // 上方向（-π/2）に少しばらつかせる
                pShot->muki = -DX_PI / 2.0 + (GetRand(50) - 25) / 180.0 * DX_PI;
                pShot->speed = 5.5 + GetRand(150) / 100.0;  // 高速で飛び去る
            }
            pShot = pShot->next;
        }

        // 痛み・残毛のイメージで赤い小玉を散らす
        for (int i = 0; i < 14; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(70) - 35);
            pEnemyShot->y = pEnemyShotSet->y + 30.0 + GetRand(40);
            // 全方向にランダム
            pEnemyShot->muki = (GetRand(360)) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.2 + GetRand(180) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[0];   // 赤の小玉
            pEnemyShot->param_i[0] = 2;                     // 2 = 痛み

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // -----------------------------------------------
    // 毎フレーム：全弾の移動処理
    // （count / pEnemyShot->count のインクリメントと画面外消去はメイン側）
    // -----------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_BrazilianWax_Grok()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;          // 少し下めでパターンが見やすく
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆっくり左右に往復
        enemy.x += 0.7 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 一定間隔でワックスパターンを発動
    if (count % 220 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWax;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = 415.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}