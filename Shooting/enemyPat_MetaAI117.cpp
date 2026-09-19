
// 前方宣言
static void ShotBalloonFragment(sEnemyShotSet* pEnemyShotSet);
static void ShotBalloon(sEnemyShotSet* pEnemyShotSet);

//--------------------------------------------------
// 割れた後にばら撒かれる弾
// pEnemyShotSet->kind : 0=赤リング / 1=青落下 / 2=黄ランダム
//--------------------------------------------------
static void ShotBalloonFragment(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 1) {
        int createNum = 0;
        if (pEnemyShotSet->kind == 0) createNum = 24; // 赤
        else if (pEnemyShotSet->kind == 1) createNum = 10; // 青
        else createNum = 16; // 黄

        for (int i = 0; i < createNum; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->count = 0;

            if (pEnemyShotSet->kind == 0) {
                // 赤：二重リング 12+12
                double base = 2.0 * DX_PI / 12.0;
                double offset = (i >= 12) ? base * 0.5 : 0.0;
                int idx = i % 12;
                pEnemyShot->muki = base * idx + offset;
                pEnemyShot->speed = 2.4 + GetRand(30) / 100.0;
                pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤小玉
            }
            else if (pEnemyShotSet->kind == 1) {
                // 青：上に舞い上がってから落下
                pEnemyShot->param_d[0] = (GetRand(200) - 100) / 100.0 * 1.5; // vx
                pEnemyShot->param_d[1] = -(1.5 + GetRand(150) / 100.0); // vy 初速上向き
                pEnemyShot->muki = 0;
                pEnemyShot->speed = 0;
                pEnemyShot->kind = img_enemyShotMediumOval[4]; // 青中楕円
            }
            else {
                // 黄：ランダムスプレー
                pEnemyShot->muki = GetRand(628) / 100.0; // 0~6.28
                pEnemyShot->speed = 2.0 + GetRand(250) / 100.0; // 2.0~4.5
                pEnemyShot->kind = img_enemyShotDiamond[1]; // 黄菱形
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->kind == 1) {
            // 青は重力落下
            pShot->param_d[1] += 0.08; // 重力
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

//--------------------------------------------------
// 風船本体
// sEnemyShot 1つが風船1つ
// param_i[0] : 0=赤 / 1=青 / 2=黄
// param_d[2] : 揺れの位相
// param_d[3] : 落下速度
//--------------------------------------------------
static void ShotBalloon(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int balloonNum = 5 + GetRand(2); // 5~7個
        for (int i = 0; i < balloonNum; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(300) - 150;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(60) - 30;
            pEnemyShot->count = 0;

            int type = GetRand(2); // 0,1,2
            pEnemyShot->param_i[0] = type;
            pEnemyShot->param_d[2] = GetRand(628) / 100.0; // 位相 0~6.28
            pEnemyShot->param_d[3] = 0.5 + GetRand(50) / 100.0; // 落下 0.5~1.0

            int col = 0;
            if (type == 0) col = 0; // 赤風船
            else if (type == 1) col = 4; // 青風船
            else col = 1; // 黄風船
            pEnemyShot->kind = img_enemyShotLargeBall[col];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 風船の移動 + 自機ショット当たり判定
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNextShot = pShot->next;

        // ふわふわ落下
        pShot->x += sin(pShot->count * 0.04 + pShot->param_d[2]) * 0.6;
        pShot->y += pShot->param_d[3];

        // 自機ショットとの衝突判定
        bool hit = false;
        sPlayerShot* pPShot = playerShotHead.next;
        while (pPShot != &playerShotHead) {
            sPlayerShot* pPNext = pPShot->next;
            double dx = pPShot->x - pShot->x;
            double dy = pPShot->y - pShot->y;
            if (dx * dx + dy * dy < 16.0 * 16.0) { // 大玉半径考慮
                // 自機ショット消去
                pPShot->prev->next = pPShot->next;
                pPShot->next->prev = pPShot->prev;
                delete pPShot;
                hit = true;
                break;
            }
            pPShot = pPNext;
        }

        if (hit) {
            // 破裂して弾ばら撒きセットを生成
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            sEnemyShotSet* pPopSet = new sEnemyShotSet;
            pPopSet->count = 0;
            pPopSet->x = pShot->x;
            pPopSet->y = pShot->y;
            pPopSet->muki = 0.0;
            pPopSet->kind = pShot->param_i[0]; // 色タイプを引き継ぎ
            pPopSet->patternFunc = ShotBalloonFragment;
            pPopSet->pEnemyShotHead = new sEnemyShot;
            pPopSet->pEnemyShotHead->prev = pPopSet->pEnemyShotHead;
            pPopSet->pEnemyShotHead->next = pPopSet->pEnemyShotHead;

            pPopSet->prev = enemyShotSetHead.prev;
            pPopSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pPopSet;
            enemyShotSetHead.prev = pPopSet;

            // 風船本体を消去
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }

        pShot = pNextShot;
    }
}

//--------------------------------------------------
// 敵本体
//--------------------------------------------------
void EnemyPat_Balloon_MetaAI()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 90Fごとに風船セットを出現
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBalloon;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count % 3;
        shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}