// enemyPat_Tmp.cpp
// 流符「夜空のカーペット」天川星羅

// ----------------------------------------------------
// 1. 星屑カーペット本体 - ばら撒いて停止→横に流れる
// ----------------------------------------------------
static void ShotCarpet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    // 生成時
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 36; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // セットの向き + 36way + 少しだけランダム
            pEnemyShot->muki = pEnemyShotSet->muki + (2.0 * DX_PI / 36.0) * i;

            double initSpeed = 1.6 + GetRand(30) / 100.0; // 1.6 ~ 1.9
            pEnemyShot->speed = initSpeed;
            pEnemyShot->count = 0;
            pEnemyShot->param_d[0] = initSpeed; // 初速保存
            pEnemyShot->param_d[1] = 0.8 + GetRand(40) / 100.0; // 流れ時の速度 0.8~1.2
            pEnemyShot->param_i[0] = 0; // 0:まだ流れてない 1:流れ中
            pEnemyShot->margin = 80.0; // ワープ用に広めに取る

            // 黄と白を交互で星空っぽく
            if (i % 2 == 0) pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄
            else pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->count < 75) {
            // 0~75F: 減速してカーペットを形成
            double t = (double)pShot->count / 75.0;
            pShot->speed = pShot->param_d[0] * (1.0 - t) + 0.08 * t;
        }
        else if (pShot->count < 75 + 60) {
            // 75~255F: ほぼ停止・滞留
            pShot->speed = 0.08;
        }
        else {
            // 255F~: 一斉に横に流す
            if (pShot->param_i[0] == 0) {
                pShot->param_i[0] = 1;
                double var = (GetRand(60) - 30) / 180.0 * DX_PI * 0.3; // 左右に少しばらつき
                pShot->muki = DX_PI + var; // 右から左へ
                pShot->speed = pShot->param_d[1];
            }
            // 自機方向へごく弱い誘導で包む
            double target = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = target - pShot->muki;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            pShot->muki += diff * 0.006;
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 流れフェーズのみ画面端でワープさせて消去を回避
        if (pShot->count >= 255) {
            if (pShot->x < -40.0) pShot->x = 520.0;
            if (pShot->x > 520.0) pShot->x = -40.0;
        }

        pShot = pShot->next;
    }
}

// ----------------------------------------------------
// 2. 青大玉 - プレイヤーを動かすための自機狙い
// ----------------------------------------------------
static void ShotBigBlue(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 2.0 + GetRand(50) / 100.0;
        pEnemyShot->count = 0;
        pEnemyShot->kind = img_enemyShotLargeBall[4]; // 青
        pEnemyShot->margin = 30.0;

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

// ----------------------------------------------------
// 3. 米粒差し込み - カーペットの隙間を潰す
// ----------------------------------------------------
static void ShotRice(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            // 敵付近から少しランダムに散らして隙間っぽく見せる
            pEnemyShot->x = pEnemyShotSet->x + GetRand(160) - 80;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(60) - 30;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(40) - 20) / 180.0 * DX_PI * 0.5;
            pEnemyShot->speed = 2.2 + GetRand(60) / 100.0;
            pEnemyShot->count = 0;
            pEnemyShot->kind = img_enemyShotBullet[3]; // シアン銃弾
            pEnemyShot->margin = 20.0;

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

// ----------------------------------------------------
// 敵本体
// ----------------------------------------------------
void EnemyPat_NightCarpet_MetaAI()
{
    static int rot;
    static int sway_dir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 300;
        rot = 0;
        sway_dir = 1;
    }
    else {
        // 天の川らしくゆらゆら横揺れ、画面中央付近を維持
        enemy.x = 240.0 + sin(count * 0.02) * 25.0;
    }

    // 12F(0.2秒)ごとにカーペットを1層敷く
    if (count % 24 == 1 && count < 1400) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotCarpet;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = rot * (1.5 / 180.0 * DX_PI); // 毎回1.5度回転
        rot++;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 50Fごとに青大玉でプレイヤーを動かす
    if (count % 50 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBigBlue;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x) + (GetRand(20) - 10) / 180.0 * DX_PI;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // カーペット形成後、90Fごとに米粒で抜け道を潰す
    if (count > 300 && count % 90 == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotRice;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}