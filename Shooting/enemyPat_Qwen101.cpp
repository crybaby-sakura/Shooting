// enemyPat_sampleForAI.cpp

// ============================================================
// ファランクス弾幕用：ヘルパー関数（弾セット生成）
// ============================================================
static void createShotSet(sEnemyShotSet::PatternFunc func, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0; // メインルーチンでインクリメントされるため0で初期化
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 15.0;
    pEnemyShotSet->muki = DX_PI / 2.0;
    pEnemyShotSet->kind = kind;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ============================================================
// ファランクス弾幕用：各パターン関数
// ============================================================

// 1. 槍の壁（5列の細長い直線弾を高密度で下へ突進）
static void ShotPhalanxSpear(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double offsets[] = { -32.0, -16.0, 0.0, 16.0, 32.0 }; // 5列並列で密度向上
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + offsets[i];
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI / 2.0;
            pEnemyShot->speed = 5.5; // 速度を上げて回避猶予を減らす
            pEnemyShot->kind = img_enemyShotBullet[6]; // 銃弾、白

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

// 2. 盾の壁（広範囲に中楕円弾を展開し、直線弾の隙間を塞ぐ）
static void ShotPhalanxShield(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 左斜め下から右斜め下まで広く3発
        double angles[] = { DX_PI / 2.0 - DX_PI / 3.0, DX_PI / 2.0, DX_PI / 2.0 + DX_PI / 3.0 };
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angles[i];
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotMediumOval[8]; // 中楕円弾、橙

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

// 3. 重圧突進（自機狙い成分を持つ中玉で、単純な縦回避を封じる）
static void ShotPhalanxHeavy(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        // 自機狙いにわずかな誤差(GetRand)を加えて予測を困難にする
        double target_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShot->muki = target_muki + (GetRand(20) - 10) / 100.0;
        pEnemyShot->speed = 3.5;
        pEnemyShot->kind = img_enemyShotMediumBall[4]; // 中玉、青

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 4. 散開・再編成（高速で広範囲にばら撒き、次のループへ繋げる）
static void ShotPhalanxScatter(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 16; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            double spread = (GetRand(30) - 15) / 100.0;
            pEnemyShot->muki = (DX_PI * 2.0 / 16.0) * i + spread;
            pEnemyShot->speed = 4.5; // 速くして回避を厳しくする
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 大玉、赤

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

// ============================================================
// 敵本体のパターン：ファランクス（高難易度・ループ対応）
// ============================================================
void EnemyPat_Phalanx_Qwen()
{
    static int shot_count = 0;
    const int CYCLE = 480; // 8秒(480フレーム)で1ループ
    int cycle_count = count % CYCLE;

    if (count == 1) {
        enemy.maxHp = enemy.hp = 200;
    }

    if (cycle_count == 1) {
        // ループ開始時の初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        shot_count = 0;
    }
    else {
        // フェーズごとの敵の動き
        if (cycle_count <= 60) {
            // フェーズ1: 隊形形成（中央へ集結し、微細に揺れる）
            if (enemy.x > 240.0) enemy.x -= 2.0;
            else if (enemy.x < 240.0) enemy.x += 2.0;
        }
        else if (cycle_count <= 180) {
            // フェーズ2: 盾の壁（左右に大きく揺れながら位置を安定させる）
            enemy.x = 240.0 + sin(cycle_count / 20.0) * 40.0;
        }
        else if (cycle_count <= 360) {
            // フェーズ3: 前進突撃（画面中央付近まで一気に圧迫）
            enemy.y += 1.2;
            enemy.x = 240.0 + sin(cycle_count / 15.0) * 20.0;
        }
        else {
            // フェーズ4: 散開・撤退（画面上部へ素早く戻り、次のループへ）
            enemy.y -= 4.0;
            if (enemy.y < 40.0) enemy.y = 40.0;
        }
    }

    // --- 弾幕発射タイミング制御 (高頻度化で難易度向上) ---

    // フェーズ1 (1〜60): 8フレームごとに高密度の槍
    if (cycle_count > 0 && cycle_count <= 60 && cycle_count % 8 == 1) {
        createShotSet(ShotPhalanxSpear, shot_count++);
    }

    // フェーズ2 (61〜180): 6フレームごとに槍、12フレームごとに盾
    if (cycle_count > 60 && cycle_count <= 180) {
        if (cycle_count % 6 == 1) {
            createShotSet(ShotPhalanxSpear, shot_count++);
        }
        if (cycle_count % 12 == 1) {
            createShotSet(ShotPhalanxShield, shot_count++);
        }
    }

    // フェーズ3 (181〜360): 6フレームごとに槍、15フレームごとに自機狙い重弾
    if (cycle_count > 180 && cycle_count <= 360) {
        if (cycle_count % 6 == 1) {
            createShotSet(ShotPhalanxSpear, shot_count++);
        }
        if (cycle_count % 15 == 1) {
            createShotSet(ShotPhalanxHeavy, shot_count++);
        }
    }

    // フェーズ4 (361〜480): 20フレームごとに広範囲散開弾
    if (cycle_count > 360 && cycle_count <= CYCLE) {
        if (cycle_count % 20 == 1) {
            createShotSet(ShotPhalanxScatter, shot_count++);
        }
    }
}