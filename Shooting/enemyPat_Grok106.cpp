// enemyPat_Tmp.cpp
// 斑鳩風ポラリティ弾幕（赤/青切り替えバリア付き）
// ボス1（左）: 赤弾、ボス2（右）: 青弾
// 単独では回避可能、同時だと回避困難な密度
// 自機周囲の小玉が赤↔青に周期切り替えし、同色弾を吸収

// 赤弾パターン（ボス1用）
static void ShotRed(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 専用音は使わない
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 自機狙い + 小散開で密度調整（単独回避可能）
        const int num = 5;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            double baseMuki = pEnemyShotSet->muki;
            double spread = (i - (num - 1) / 2.0) * (8.0 / 180.0 * DX_PI); // ±約16度程度
            pEnemyShot->x = pEnemyShotSet->x + GetRand(10) - 5;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(6) - 3;
            pEnemyShot->muki = baseMuki + spread;
            pEnemyShot->speed = 3.2 + GetRand(40) / 100.0;
            // 赤は MediumBall[0] など SmallBall[0] 以外を使用
            pEnemyShot->kind = img_enemyShotMediumBall[0];
            pEnemyShot->param_i[0] = 0; // 色識別: 赤

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 青弾パターン（ボス2用）
static void ShotBlue(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int num = 5;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            double baseMuki = pEnemyShotSet->muki;
            double spread = (i - (num - 1) / 2.0) * (8.0 / 180.0 * DX_PI);
            pEnemyShot->x = pEnemyShotSet->x + GetRand(10) - 5;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(6) - 3;
            pEnemyShot->muki = baseMuki + spread;
            pEnemyShot->speed = 3.2 + GetRand(40) / 100.0;
            // 青は MediumBall[4] など SmallBall[4] 以外を使用
            pEnemyShot->kind = img_enemyShotMediumBall[4];
            pEnemyShot->param_i[0] = 4; // 色識別: 青

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

// 自機周囲の極性バリア小玉（吸収処理付き）
static void ShotOrbs(sEnemyShotSet* pEnemyShotSet)
{
    // 静止（speed=0）なので移動なし
    // 同色弾の吸収判定
    const double absorbRadius = 9.0; // 小玉半径2.5前後 + 余裕

    sEnemyShot* pOrb = pEnemyShotSet->pEnemyShotHead->next;
    while (pOrb != pEnemyShotSet->pEnemyShotHead) {
        int orbColor = pOrb->param_i[0]; // 0:赤 or 4:青

        // 全ショットセットを走査
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet == pEnemyShotSet) {
                pSet = pSet->next;
                continue;
            }
            sEnemyShot* pBullet = pSet->pEnemyShotHead->next;
            while (pBullet != pSet->pEnemyShotHead) {
                sEnemyShot* pNextBullet = pBullet->next; // 削除対策

                if (pBullet->param_i[0] == orbColor) {
                    double dx = pOrb->x - pBullet->x;
                    double dy = pOrb->y - pBullet->y;
                    double distSq = dx * dx + dy * dy;
                    if (distSq < absorbRadius * absorbRadius) {
                        // 同色弾を消滅
                        pBullet->prev->next = pBullet->next;
                        pBullet->next->prev = pBullet->prev;
                        delete pBullet;
                    }
                }
                pBullet = pNextBullet;
            }
            pSet = pSet->next;
        }
        pOrb = pOrb->next;
    }
}

// 敵本体パターン
void EnemyPat_Ikaruga_Grok()
{
    static int muki = 1;
    static sEnemyShotSet* pOrbSet = nullptr;
    static int orbColor = 0; // 現在のバリア色 0:赤 4:青

    // 初期化
    if (count == 1) {
        enemy.x = 120.0;
        enemy.y = 40.0;
        enemy.x2 = 360.0;
        enemy.y2 = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        pOrbSet = nullptr;
        orbColor = 0;
    }
    else {
        // 左右移動
        enemy.x += 0.98 * (double)muki;
        enemy.x2 += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // ========== 通常弾幕（赤・青） ==========
    // 密度: 約8フレームごとに各ボスから扇状5発 → 単独は隙間あり、同時は交差で厳しい
    if (count > 30 && count % 8 == 0) {
        // ボス1（左）赤
        {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotRed;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 12.0;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        // ボス2（右）青
        {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotBlue;
            pEnemyShotSet->x = enemy.x2;
            pEnemyShotSet->y = enemy.y2 + 12.0;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }

    // ========== 極性バリア（3秒周期） ==========
    // 0s: charge, 1s: 配置or変化 + extreme, 3s: charge, 4s: 変化 ...
    // 180フレーム = 3秒 を1サイクル
    const int CYCLE = 180;
    int t = count % CYCLE;

    // チャージ音（0秒, 3秒, ...）
    if (t == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 配置/変化 + ショット音（1秒, 4秒, ...）
    if (t == 60) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        if (pOrbSet == nullptr) {
            // 初回: 赤小玉で自機を取り囲む配置
            pOrbSet = new sEnemyShotSet;
            pOrbSet->count = 0;
            pOrbSet->patternFunc = ShotOrbs;
            pOrbSet->x = player.x;
            pOrbSet->y = player.y;
            pOrbSet->muki = 0.0;
            pOrbSet->pEnemyShotHead = new sEnemyShot;
            pOrbSet->pEnemyShotHead->prev = pOrbSet->pEnemyShotHead;
            pOrbSet->pEnemyShotHead->next = pOrbSet->pEnemyShotHead;
            pOrbSet->prev = enemyShotSetHead.prev;
            pOrbSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pOrbSet;
            enemyShotSetHead.prev = pOrbSet;

            const int NUM_ORBS = 24;
            const double RADIUS = 90.0;
            for (int i = 0; i < NUM_ORBS; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                double ang = (double)i / NUM_ORBS * 2.0 * DX_PI;
                pShot->x = player.x + RADIUS * cos(ang);
                pShot->y = player.y + RADIUS * sin(ang);
                pShot->muki = 0.0;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotSmallBall[0]; // 赤
                pShot->param_i[0] = 0;                   // 色

                pShot->prev = pOrbSet->pEnemyShotHead->prev;
                pShot->next = pOrbSet->pEnemyShotHead;
                pOrbSet->pEnemyShotHead->prev->next = pShot;
                pOrbSet->pEnemyShotHead->prev = pShot;
            }
            orbColor = 0;
        }
        else {
            // 以降: 色を反転
            orbColor = (orbColor == 0) ? 4 : 0;
            int newKind = (orbColor == 0) ? img_enemyShotSmallBall[0] : img_enemyShotSmallBall[4];

            sEnemyShot* pShot = pOrbSet->pEnemyShotHead->next;
            while (pShot != pOrbSet->pEnemyShotHead) {
                pShot->kind = newKind;
                pShot->param_i[0] = orbColor;
                // margin は既に 999.0 のまま
                pShot = pShot->next;
            }
        }
    }
}