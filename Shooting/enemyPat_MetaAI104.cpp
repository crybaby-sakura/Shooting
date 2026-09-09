// enemyPat_Tmp.cpp
// モチーフ: ブラジリアンワックス

static void ShotBrazilianWax(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 3枚のシートの目標Y
    const double SHEET_TARGET_Y[3] = { 315.0, 360.0, 405.0 };
    const int SHEET_NUM = 3;
    const int LASER_PER_SHEET = 5; // 1枚を5本のレーザーで構成
    const int PEEL_INTERVAL = 20; // 剥がす間隔

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 毛: 銃弾型 黒
        for (int i = 0; i < 60-10; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 80.0 + GetRand(320);
            pEnemyShot->y = 295.0 + GetRand(130) + 50;
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.0;
            pEnemyShot->count = 0;
            pEnemyShot->kind = img_enemyShotBullet[7]; // 黒
            pEnemyShot->param_i[0] = 0; // type 毛
            pEnemyShot->param_i[1] = 0; // 付着フラグ
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ワックスシート3枚: 黄色レーザーを横に並べて1枚にする
        for (int s = 0; s < SHEET_NUM; s++) {
            for (int j = 0; j < LASER_PER_SHEET; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = 80.0 + 32.0 + j * 64.0;
                pEnemyShot->y = 230.0 - s * 10.0 + 50; // 初期は少し上にスタック
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->count = 0;
                pEnemyShot->kind = img_enemyShotLaser[1]; // 黄 ワックス
                pEnemyShot->param_i[0] = 1; // type シート
                pEnemyShot->param_i[2] = s; // 何枚目のシートか 0が一番上
                pEnemyShot->param_d[0] = SHEET_TARGET_Y[s] + 50; // 目標Y

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    else if (pEnemyShotSet->count < 60) {
        // 圧着: 3枚同時に目標Yへ
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                double ty = pShot->param_d[0];
                pShot->y += (ty - pShot->y) * 0.08;
            }
            pShot = pShot->next;
        }
    }
    else if (pEnemyShotSet->count < 90) {
        if (pEnemyShotSet->count == 60) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }
    else {
        // 上から順に短い間隔で剥がす
        int peelIdx = -1;
        if (pEnemyShotSet->count == 90) peelIdx = 0;
        else if (pEnemyShotSet->count == 90 + PEEL_INTERVAL) peelIdx = 1;
        else if (pEnemyShotSet->count == 90 + PEEL_INTERVAL * 2) peelIdx = 2;

        if (peelIdx != -1) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            double peelMuki = -DX_PI * 0.37; // 右上へ
            if (peelIdx & 1) peelMuki = DX_PI - peelMuki;

            // 該当シートを飛ばす
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[0] == 1 && pShot->param_i[2] == peelIdx) {
                    pShot->muki = peelMuki + (GetRand(10) - 5) / 180.0 * DX_PI;
                    pShot->speed = (580 + GetRand(80)) / 100.0;
                }
                pShot = pShot->next;
            }

            // そのシートに重なっている毛だけを一緒に剥がす
            double targetY = SHEET_TARGET_Y[peelIdx] + 50;
            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[0] == 0 && pShot->param_i[1] == 0) {
                    if (pShot->x >= 70.0 && pShot->x <= 410.0 && fabs(pShot->y - targetY) < 26.0) {
                        pShot->param_i[1] = 1;
                        pShot->muki = peelMuki + (GetRand(30) - 15) / 180.0 * DX_PI;
                        pShot->speed = (560 + GetRand(100)) / 100.0;
                    }
                }
                pShot = pShot->next;
            }
        }

        // 剥がされたものの移動
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->speed > 0.0) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }

        // 全て剥がし終わった後にツルツル演出
        if (pEnemyShotSet->count == 160) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 20; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = 100.0 + GetRand(280);
                pEnemyShot->y = 310.0 + GetRand(110);
                pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
                pEnemyShot->speed = (80 + GetRand(120)) / 100.0;
                pEnemyShot->count = 0;
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
                pEnemyShot->param_i[0] = 2;
                pEnemyShot->param_d[0] = 0.9 + 0.11;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        if (pEnemyShotSet->count > 160) {
            sEnemyShot* pS = pEnemyShotSet->pEnemyShotHead->next;
            while (pS != pEnemyShotSet->pEnemyShotHead) {
                if (pS->param_i[0] == 2) {
                    pS->x += pS->speed * cos(pS->muki);
                    pS->y += pS->speed * sin(pS->muki);
                    pS->speed *= pS->param_d[0];
                }
                pS = pS->next;
            }
        }
    }
}

void EnemyPat_BrazilianWax_MetaAI()
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
        enemy.x += 0.8 * (double)muki;
        if (count % 140 == 70) muki *= -1;
    }

    if (count % 240 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBrazilianWax;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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