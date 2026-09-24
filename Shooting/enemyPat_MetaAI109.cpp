// enemyPat_tmp.cpp
// バラ曲線弾幕 - Rosa Pentafilla

// バラ曲線弾幕本体
static void ShotRosa(sEnemyShotSet* pEnemyShotSet)
{
    // --------------- 生成時 ---------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 花全体の回転オフセットをランダムに決定
        // GetRand(359) は 0〜359 の360種類を返す
        double rotBaseDeg = (double)GetRand(359);
        pEnemyShotSet->param_d[0] = rotBaseDeg * DX_PI / 180.0; // rad
        pEnemyShotSet->param_d[1] = 0; // 未使用

        int colorBase = pEnemyShotSet->kind % 9;
        if (colorBase < 0) colorBase += 9;
        const int k = 5; // 5枚花びら

        // θを0〜360度まで2度刻みで走査
        for (int thetaDeg = 0; thetaDeg < 360; thetaDeg += 1) {
            double thetaRad = thetaDeg * DX_PI / 180.0;
            double s = sin(k * thetaRad); // バラ曲線 r = sin(kθ)

            // s <= 0.15 の部分は隙間にする -> 5枚の花びらが分離する
            if (s <= -0.15) continue;

            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = thetaRad + pEnemyShotSet->param_d[0];
            // 花びらの先端ほど速く -> バラの形が自然に広がる
            pEnemyShot->speed = 0.8 + s * 2.6;
            pEnemyShot->margin = 480;

            // 花びらインデックスで色をずらす
            int petalIdx = thetaDeg / 72; // 0..4
            int col = (colorBase + petalIdx) % 9;

            // 先端( s > 0.85 )だけ中玉にして輪郭強調
            if (s > 0.85) {
                pEnemyShot->kind = img_enemyShotMediumBall[col];
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[col];
            }

            // 連結リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --------------- 毎フレーム更新 ---------------
    // count, pEnemyShot->count のインクリメントと画面外削除はメイン側で行われる
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->count < 90) {
            // Phase1: 開花 - そのまま外へ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pEnemyShotSet->count < 150) {
            // Phase2: 停止 - 減速して花を固定して見せる
            pShot->speed *= 0.94;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pEnemyShotSet->count == 150) {
            // Phase3: 散華 - 一斉に反転
            pShot->muki += DX_PI;
            // GetRand(60)は0〜60の61種類 -> 1.0〜1.6の速度で中心へ
            pShot->speed = 1.0 + GetRand(60) / 100.0;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 中心に向かって少し加速
            pShot->speed += 0.015;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体
void EnemyPat_RoseCurve_MetaAI()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 90フレームごとに一つのバラを咲かせる
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRosa;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++; // 色と回転の種に使う

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}