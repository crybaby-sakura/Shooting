// 風船モチーフ弾幕パターン
// 画面下から風船が浮かび上がり、自機ショットが当たると割れて弾をばら撒く

// 弾幕：風船（1セットにつき風船1個＋破裂後のばら撒き弾）
static void ShotBalloon(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 風船を1つ生成
        sEnemyShot* pBalloon = new sEnemyShot;
        pBalloon->x = pEnemyShotSet->x;
        pBalloon->y = pEnemyShotSet->y;
        pBalloon->muki = 0.0;
        pBalloon->speed = 0.4 + GetRand(40) / 100.0;   // ゆっくり上昇
        // 大きな玉を風船として使用（色はランダム）
        int col = GetRand(8);
        pBalloon->kind = img_enemyShotLargeBall[col];
        pBalloon->param_i[0] = 1;                       // 1 = 風船フラグ
        pBalloon->param_d[0] = (GetRand(200) - 100) / 50.0; // 横揺れの初期位相オフセット
        pBalloon->param_d[1] = 0.0;                     // 揺れ用位相

        pBalloon->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pBalloon->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pBalloon;
        pEnemyShotSet->pEnemyShotHead->prev = pBalloon;
    }

    // ショット処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;   // 削除に備えて次を保存

        if (pShot->param_i[0] == 1) {
            // ===== 風船の挙動 =====
            // ゆっくり上昇 + 左右に揺れる
            pShot->y -= pShot->speed;
            pShot->param_d[1] += 0.045;
            pShot->x += sin(pShot->param_d[1] + pShot->param_d[0]) * 0.9;

            // 自機ショットとの当たり判定
            bool hit = false;
            sPlayerShot* pPShot = playerShotHead.next;
            while (pPShot != &playerShotHead) {
                double dx = pPShot->x - pShot->x;
                double dy = pPShot->y - pShot->y;
                // 大玉の見た目に合わせて半径を広めに取る
                if (dx * dx + dy * dy < 22.0 * 22.0) {
                    hit = true;
                    // 当たった自機ショットをリストから外して削除
                    pPShot->prev->next = pPShot->next;
                    pPShot->next->prev = pPShot->prev;
                    delete pPShot;
                    break;
                }
                pPShot = pPShot->next;
            }

            // 自機ショットでヒットした場合のみ破裂して弾をばら撒く
            if (hit) {
                // 破裂音
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                double bx = pShot->x;
                double by = pShot->y;

                // 風船をリストから削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;

                // ばら撒き弾を生成（放射状 + 少しランダム）
                int num = 9 + GetRand(4);   // 9〜13発
                for (int i = 0; i < num; i++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = bx + (GetRand(16) - 8);
                    pNew->y = by + (GetRand(16) - 8);
                    // ほぼ均等な放射 + 少しばらつき
                    double baseAngle = (360.0 * i) / num;
                    pNew->muki = (baseAngle + (GetRand(30) - 15)) / 180.0 * DX_PI;
                    pNew->speed = 1.6 + GetRand(140) / 100.0;
                    // 小さな玉を使用（色はランダム）
                    int col = GetRand(8);
                    pNew->kind = img_enemyShotSmallBall[col];
                    pNew->param_i[0] = 0;   // 通常弾

                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }
            }
            // 画面上部へ抜けた場合は何もばら撒かず消す
            else if (pShot->y < -40.0) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        else {
            // ===== 通常のばら撒き弾の移動 =====
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_Balloon_Grok()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;   // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆるやかな左右移動
        enemy.x += 0.55 * (double)muki;
        if (enemy.x < 70.0 || enemy.x > 410.0) {
            muki *= -1;
        }
        // わずかに上下に揺れる
        enemy.y = 70.0 + 12.0 * sin(count / 50.0);
    }

    // 一定間隔で画面下から風船を放出
    if (count % 28 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBalloon;
        // 画面下端付近から出現（左右にばらけさせる）
        pEnemyShotSet->x = 40.0 + GetRand(400);
        pEnemyShotSet->y = 460.0 + GetRand(30);
        pEnemyShotSet->muki = 0.0;
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