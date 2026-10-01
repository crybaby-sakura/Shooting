// 弾幕：風船爆裂祭
// 風船に自機ショットが当たると、その場で破裂して大量の弾をばら撒く。

static void ShotBalloon(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        // param_i[0]
        //   0 : 風船
        //   1 : 破裂後の散弾
        //   2 : 破裂済みの風船本体
        if (pShot->param_i[0] == 0) {
            // 風船はゆっくり下降しながら左右にふわふわ揺れる。
            // count はメインルーチン側で自動的に増加する。
            pShot->x = pShot->param_d[0]
                + 42.0 * sin(pShot->count * 0.045 + pShot->param_d[2]);
            pShot->y = pShot->param_d[1]
                + pShot->count * 0.65
                + 10.0 * sin(pShot->count * 0.09 + pShot->param_d[2]);

            // 自機ショットが風船に触れたら破裂する。
            sPlayerShot* pPlayerShot = playerShotHead.next;
            while (pPlayerShot != &playerShotHead) {
                const double dx = pPlayerShot->x - pShot->x;
                const double dy = pPlayerShot->y - pShot->y;

                if (dx * dx + dy * dy <= 16.0 * 16.0) {
                    const double burstX = pShot->x;
                    const double burstY = pShot->y;
                    const int shotNum = 36 + GetRand(15); // 36～51発
                    const double baseAngle = GetRand(359) / 360.0 * 2.0 * DX_PI;

                    pShot->param_i[0] = 2;
                    pShot->kind = img_enemyShotSmallBall[6];
                    pShot->speed = 0.0;

                    if (CheckSoundMem(sound_enemyShot_heavy))
                        StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                    // 風船の破裂位置から放射状に大量の弾を生成する。
                    for (int i = 0; i < shotNum; i++) {
                        sEnemyShot* pBurst = new sEnemyShot;

                        pBurst->x = burstX;
                        pBurst->y = burstY;
                        pBurst->muki = baseAngle
                            + 2.0 * DX_PI * i / (double)shotNum
                            + (GetRand(20) - 10) * DX_PI / 180.0;
                        pBurst->speed = 2.0 + GetRand(130) / 100.0;

                        // 小玉を主軸にして、ところどころ菱形弾を混ぜる。
                        if (i % 5 == 0)
                            pBurst->kind = img_enemyShotDiamond[8];
                        else
                            pBurst->kind = img_enemyShotSmallBall[1];

                        pBurst->margin = 40.0;
                        pBurst->param_i[0] = 1;

                        pBurst->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pBurst->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pBurst;
                        pEnemyShotSet->pEnemyShotHead->prev = pBurst;
                    }

                    break;
                }

                pPlayerShot = pPlayerShot->next;
            }
        }
        else if (pShot->param_i[0] == 1) {
            // 破裂した弾は通常の直線運動。
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_Balloon_ChatGPT()
{
    static int muki;
    static int balloonCount;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        balloonCount = 0;
    }
    else {
        enemy.x += 0.92 * (double)muki;
        if (count % 140 == 70)
            muki *= -1;
    }

    // 上部から風船の群れを送り込む。
    if (count % 28 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBalloon;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = balloonCount++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 1～3個の風船を横に並べて出現させる。
        const int balloonNum = 1 + GetRand(2);
        for (int i = 0; i < balloonNum; i++) {
            sEnemyShot* pBalloon = new sEnemyShot;

            const double offset = (i - (balloonNum - 1) * 0.5) * 70.0;
            pBalloon->x = pEnemyShotSet->x + offset;
            pBalloon->y = pEnemyShotSet->y;
            pBalloon->muki = 0.0;
            pBalloon->speed = 0.0;
            pBalloon->margin = 240;

            // 中楕円弾を風船として使用する。色違いを交互に配置。
            if ((balloonCount + i) % 2 == 0)
                pBalloon->kind = img_enemyShotMediumOval[8];
            else
                pBalloon->kind = img_enemyShotMediumOval[5];

            pBalloon->margin = 40.0;
            pBalloon->param_i[0] = 0;
            pBalloon->param_i[1] = balloonCount;
            pBalloon->param_d[0] = pBalloon->x;
            pBalloon->param_d[1] = pBalloon->y;
            pBalloon->param_d[2] = (balloonCount + i) * 1.7;

            pBalloon->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBalloon->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBalloon;
            pEnemyShotSet->pEnemyShotHead->prev = pBalloon;
        }
    }
}
