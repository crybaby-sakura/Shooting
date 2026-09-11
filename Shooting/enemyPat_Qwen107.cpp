// ============================================================
// 弾幕：雨下のかみなり (専用実装)
// ============================================================

// 1. 豪雨（ベース弾幕）
static void ShotRain(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 雨は頻繁に発生するため、効果音は軽めまたは無音とする
        int num = 5 + GetRand(5) - 3; // 1回あたり5〜10発
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 画面幅全体から、画面上部少し上から出現
            pEnemyShot->x = (double)GetRand(480);
            pEnemyShot->y = -20.0 - (double)GetRand(40);

            // ほぼ真下(DX_PI/2)に、わずかな揺らぎを持たせる
            pEnemyShot->muki = DX_PI / 2.0 + ((double)GetRand(20) - 10.0) / 100.0;
            pEnemyShot->speed = 6.0 + (double)GetRand(200) / 50.0 - 3; // 高速

            // 小玉の青(4)またはシアン(3)
            int color = (GetRand(1) == 0) ? 4 : 3;
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 2. 風雨（追加効果）
static void ShotWindRain(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int num = 8 + GetRand(8) - 4;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = (double)GetRand(480);
            pEnemyShot->y = -10.0;

            // 斜め下（左下または右下）に流れる
            double base_muki = (GetRand(1) == 0) ? (DX_PI / 2.0 + DX_PI / 6.0) : (DX_PI / 2.0 - DX_PI / 6.0);
            pEnemyShot->muki = base_muki + ((double)GetRand(20) - 10.0) / 100.0;
            pEnemyShot->speed = 4.0 + (double)GetRand(100) / 50.0 - 2;

            // 鱗弾または中玉のシアン(3)
            int kind = (GetRand(1) == 0) ? img_enemyShotScale[3] : img_enemyShotMediumBall[3];
            pEnemyShot->kind = kind;

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

// 3. 稲妻と落雷（メインギミック）
static void ShotLightning(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 稲妻の落下位置（プレイヤーのX座標付近に狙い撃ち、ただし少しずらす）
        double target_x = player.x + (double)(GetRand(10) - 5);
        if (target_x < 20.0) target_x = 20.0;
        if (target_x > 460.0) target_x = 460.0;

        // 落雷発生位置を後で参照できるように保存
        pEnemyShotSet->param_d[0] = target_x;

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = target_x;
        pEnemyShot->y = 0.0;
        pEnemyShot->muki = DX_PI / 2.0; // 真下
        pEnemyShot->speed = 15.0 + (double)GetRand(50) / 10.0; // 超高速

        // 短レーザーまたは銃弾の白(6)
        pEnemyShot->kind = img_enemyShotLaser[6];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 稲妻弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }

    // 落雷の発生 (count が特定の値になったら、メインルーチンで自動インクリメントされる仕様を利用)
    if (pEnemyShotSet->count == 25) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double rx = pEnemyShotSet->param_d[0]; // 保存しておいた落下位置X
        double ry = 460.0; // 画面下部付近

        // 周囲8方向〜16方向へ放射状に放出
        int num = 12 + GetRand(4); // 12〜16方向
        for (int i = 0; i < num; i++) {
            sEnemyShot* pThunder = new sEnemyShot;
            pThunder->x = rx;
            pThunder->y = ry;
            pThunder->muki = (DX_PI * 2.0 / (double)num) * (double)i;
            pThunder->speed = 3.0 + (double)GetRand(200) / 100.0;

            // 大玉または中楕円弾の黄(1)または白(6)
            int color = (GetRand(1) == 0) ? 1 : 6;
            pThunder->kind = (GetRand(1) == 0) ? img_enemyShotLargeBall[color] : img_enemyShotMediumOval[color];

            pThunder->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pThunder->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pThunder;
            pEnemyShotSet->pEnemyShotHead->prev = pThunder;
        }
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_ThunderInRain_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右へのゆっくりした移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 1. 豪雨セットの生成 (頻繁に)
    if (count % 8 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 2. 風雨セットの生成 (定期的に)
    if (count % 45 == 15) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWindRain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 3. 稲妻・落雷セットの生成 (一定間隔で)
    if (count % 150 == 75) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLightning;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}