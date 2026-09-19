// 弾幕：燃え上がる竹やぶの狂詩曲
static void PatternBambooFire(sEnemyShotSet* pEnemyShotSet)
{
    int c = pEnemyShotSet->count;

    // 330フレームでパターンをリセットしてループ
    if (c >= 330) {
        pEnemyShotSet->count = -1; // 次フレームのメインルーチンで0になる
        pEnemyShotSet->param_d[0] = 0.0;
        c = -1;
    }

    if (c == 0) {
        pEnemyShotSet->param_d[0] = 0.0; // 螺旋角度の初期化
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 追加: フェーズ1開始予告音
    }

    // フェーズ1: 「ドンドコ、ドンドコ」 (0〜119フレーム)
    if (c < 120) {
        int cycle = c % 60;
        if (cycle == 0) {
            // 「ドン！」: 自機狙い 緑の短レーザー
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x;
            shot->y = pEnemyShotSet->y;
            shot->muki = atan2(player.y - shot->y, player.x - shot->x);
            shot->speed = 7.0;
            shot->kind = img_enemyShotLaser[2];
            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
        else if (cycle == 15) {
            // 「ド」: 緑の鱗弾 3Way
            double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            for (int i = -3; i <= 3; ++i) {
                sEnemyShot* shot = new sEnemyShot;
                shot->x = pEnemyShotSet->x;
                shot->y = pEnemyShotSet->y;
                shot->muki = base_muki + i * 0.3;
                shot->speed = 3.5;
                shot->kind = img_enemyShotScale[2];
                shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                shot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = shot;
                pEnemyShotSet->pEnemyShotHead->prev = shot;
            }
        }
        else if (cycle == 30 || cycle == 35 || cycle == 40) {
            // 「コ」: 赤い小玉 (火の粉)
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x;
            shot->y = pEnemyShotSet->y;
            shot->muki = (GetRand(359) / 359.0) * 2.0 * DX_PI;
            shot->speed = 2.0 + GetRand(20) / 10.0;
            shot->kind = img_enemyShotSmallBall[0];
            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }
    // フェーズ2: 「ドンドコドンドコ」 (120〜239フレーム)
    else if (c < 240) {
        if (c == 120) { // 追加: フェーズ2開始時の効果音
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        int phase2_c = c - 120;
        int cycle = phase2_c % 40;

        if (cycle == 0 || cycle == 20) {
            // 「ドンド」: 橙の中楕円弾 全方向16way (燃え広がる炎の輪)
            for (int i = 0; i < 16*2; ++i) {
                sEnemyShot* shot = new sEnemyShot;
                shot->x = pEnemyShotSet->x;
                shot->y = pEnemyShotSet->y;
                shot->muki = (i / 16.0/2) * 2.0 * DX_PI;
                shot->speed = 1.5;
                shot->kind = img_enemyShotMediumOval[8];
                shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                shot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = shot;
                pEnemyShotSet->pEnemyShotHead->prev = shot;
            }
        }
        else if (cycle == 10 || cycle == 30) {
            // 「コ」: 赤い小玉 自機狙い
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x;
            shot->y = pEnemyShotSet->y;
            shot->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            shot->speed = 4.5;
            shot->kind = img_enemyShotSmallBall[0];
            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }
    // フェーズ3: 「ドンドコドンドコドンドコドン」 (240〜329フレーム)
    else {
        if (c == 240) { // 追加: フェーズ3開始時の効果音
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        int phase3_c = c - 240;
        int cycle = phase3_c % 90;

        if (cycle == 0 || cycle == 30) {
            // 「ドン」「ド」: 緑の菱形弾 螺旋
            pEnemyShotSet->param_d[0] += 0.3;
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x;
            shot->y = pEnemyShotSet->y;
            shot->muki = pEnemyShotSet->param_d[0];
            shot->speed = 3.0;
            shot->kind = img_enemyShotDiamond[2];
            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
        else if (cycle == 10 || cycle == 40) {
            // 「ド」: 赤い小玉 螺旋
            pEnemyShotSet->param_d[0] += 0.3;
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x;
            shot->y = pEnemyShotSet->y;
            shot->muki = pEnemyShotSet->param_d[0] + 0.15;
            shot->speed = 3.5;
            shot->kind = img_enemyShotSmallBall[0];
            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
        else if (cycle == 20 || cycle == 50) {
            // 「コ」: 緑の鱗弾 螺旋
            pEnemyShotSet->param_d[0] += 0.3;
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x;
            shot->y = pEnemyShotSet->y;
            shot->muki = pEnemyShotSet->param_d[0] + 0.3;
            shot->speed = 2.8;
            shot->kind = img_enemyShotScale[2];
            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
        else if (cycle == 60) {
            // 「ドン！」: 赤い大玉 爆発 (全方向24way)
            for (int i = 0; i < 24*2; ++i) {
                sEnemyShot* shot = new sEnemyShot;
                shot->x = pEnemyShotSet->x;
                shot->y = pEnemyShotSet->y;
                shot->muki = (i / 24.0/2) * 2.0 * DX_PI;
                shot->speed = 2.0 + GetRand(15) / 10.0;
                shot->kind = img_enemyShotLargeBall[0];
                shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                shot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = shot;
                pEnemyShotSet->pEnemyShotHead->prev = shot;
            }
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_TakeyabuYaketa_Qwen()
{
    static sEnemyShotSet* pMyShotSet = nullptr;
    static int muki = 1;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        pMyShotSet = new sEnemyShotSet;
        pMyShotSet->count = 0;
        pMyShotSet->patternFunc = PatternBambooFire;
        pMyShotSet->x = enemy.x;
        pMyShotSet->y = enemy.y + 10.0;

        pMyShotSet->pEnemyShotHead = new sEnemyShot;
        pMyShotSet->pEnemyShotHead->prev = pMyShotSet->pEnemyShotHead;
        pMyShotSet->pEnemyShotHead->next = pMyShotSet->pEnemyShotHead;

        pMyShotSet->prev = enemyShotSetHead.prev;
        pMyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMyShotSet;
        enemyShotSetHead.prev = pMyShotSet;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    if (pMyShotSet) {
        pMyShotSet->x = enemy.x;
        pMyShotSet->y = enemy.y + 10.0;
    }
}