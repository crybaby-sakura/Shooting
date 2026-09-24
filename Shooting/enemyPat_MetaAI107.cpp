// EnemyPat_ThunderInRain_MetaAI.cpp - 雨下のかみなり「蓄雷積雨」
// gv.h / DxLib.h 環境用

//--------------------------------------------------
// 1. 常時降る雨 - しとしと
//--------------------------------------------------
static void ShotRain(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int num = 10 + GetRand(4); // 10-14本
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(500) - 250;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(30) - 15;
            // ほぼ真下 + 少しだけ左右にばらけ
            pEnemyShot->muki = DX_PI * 0.5 + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.0 + GetRand(100) / 100.0; // 2.0-3.0
            pEnemyShot->param_d[0] = (GetRand(10) - 5) / 1000.0; // 微風ドリフト

            // 青とシアンを混ぜて雨の濃淡を表現
            if (GetRand(3) == 0) pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン
            else pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki) + pShot->param_d[0];
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//--------------------------------------------------
// 2. 横殴りの突風
//--------------------------------------------------
static void ShotGust(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int fromLeft = GetRand(1); // 0:左から, 1:右から
        for (int i = 0; i < 10; i++) {
            sEnemyShot* p = new sEnemyShot;
            if (fromLeft == 0) {
                p->x = -10.0;
                p->muki = (GetRand(20) - 10) / 180.0 * DX_PI; // ほぼ0度
            }
            else {
                p->x = 490.0;
                p->muki = DX_PI + (GetRand(20) - 10) / 180.0 * DX_PI; // ほぼ180度
            }
            p->y = 60.0 + GetRand(380);
            p->speed = 4.5 + GetRand(100) / 100.0;
            p->kind = img_enemyShotBullet[4]; // 銃弾・青 細くて速い雨の筋

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki) * 0.15; // 少しだけ落下成分
        pShot = pShot->next;
    }
}

//--------------------------------------------------
// 3. 帯電する雨雲 - 雷のチャージリング
//--------------------------------------------------
static void ShotChargeCloud(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_d[0] = 30.0; // 初期半径
        for (int i = 0; i < 20; i++) {
            sEnemyShot* p = new sEnemyShot;
            double ang = (DX_TWO_PI / 20.0) * i;
            p->param_d[0] = ang; // 個別の角度を保持
            p->param_d[1] = 0.04 + (i % 2 == 0 ? 0.015 : -0.015); // 回転速度
            p->x = pEnemyShotSet->x + pEnemyShotSet->param_d[0] * cos(ang);
            p->y = pEnemyShotSet->y + pEnemyShotSet->param_d[0] * sin(ang);
            p->kind = img_enemyShotSmallBall[1]; // 黄
            p->speed = 0;
            p->margin = 120;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 敵に追従
    pEnemyShotSet->x += (enemy.x - pEnemyShotSet->x) * 0.12;
    pEnemyShotSet->y += (enemy.y - pEnemyShotSet->y) * 0.12;
    double radius = 30.0 + pEnemyShotSet->count * 0.22; // 徐々に膨張
    pEnemyShotSet->param_d[0] = radius;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->param_d[0] += pShot->param_d[1];
        pShot->x = pEnemyShotSet->x + radius * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y + radius * sin(pShot->param_d[0]);

        // 260F超えたら白く発光して雷直前を演出
        if (pEnemyShotSet->count > 260) {
            pShot->kind = img_enemyShotSmallBall[6]; // 白
        }
        if (pEnemyShotSet->count > 260 + 60) {
            pShot->margin = -999;
        }
        pShot = pShot->next;
    }
}

//--------------------------------------------------
// 4. 落雷本体
//--------------------------------------------------
static void ShotThunderBolt(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int baseX = pEnemyShotSet->param_i[0]; // プレイヤーXをEnemy側から渡す
        for (int k = 0; k < 3; k++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = baseX + (k - 1) * 28/2 + GetRand(12) - 6;
            p->y = pEnemyShotSet->y;
            // 真下に落ちるが少しだけジグザグ
            p->muki = DX_PI * 0.5 + (GetRand(12) - 6) / 180.0 * DX_PI;
            p->speed = 11.0 + GetRand(120) / 100.0; // 11.0-12.2 超高速
            p->param_d[0] = k; // 枝番号
            p->param_i[0] = GetRand(100); // ジグザグ位相用
            if (k == 1) p->kind = img_enemyShotLaser[6]; // 真ん中は白で主雷
            else p->kind = img_enemyShotLaser[1]; // 枝は黄
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
        // 放電火花
        for (int i = 0; i < 12; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = DX_TWO_PI / 12.0 * i + GetRand(10) / 100.0;
            p->speed = 2.5 + GetRand(150) / 100.0;
            p->kind = img_enemyShotSmallBall[6]; // 白火花
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 雷のジグザグ演出
        double zig = sin(pShot->count * 0.45 + pShot->param_i[0]) * 0.08;
        pShot->muki += zig;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//--------------------------------------------------
// 5. 着弾後の水しぶき
//--------------------------------------------------
static void ShotSplash(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 14; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = 470.0;
            // 上向き扇状
            p->muki = -DX_PI * 0.5 + (GetRand(120) - 60) / 180.0 * DX_PI;
            p->speed = 1.8 + GetRand(150) / 100.0;
            p->param_d[0] = p->speed * sin(p->muki); // vy
            p->param_d[1] = 0.11; // 重力
            p->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[3] : img_enemyShotSmallBall[6];
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->param_d[0] += pShot->param_d[1];
        pShot->y += pShot->param_d[0];
        pShot = pShot->next;
    }
}

//--------------------------------------------------
// 敵本体
//--------------------------------------------------
void EnemyPat_ThunderInRain_MetaAI()
{
    static int muki;
    static int lastImpactX;
    static int splashTimer;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        lastImpactX = 240;
        splashTimer = 0;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 雨：常時 4F毎
    if (count % 20 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotRain;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 突風：120F毎
    if (count % 120 == 60) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotGust;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 帯電リング：360Fサイクルの最初に生成
    if (count % 360 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotChargeCloud;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 落雷：チャージの最後 300F目
    if (count % 360 == 300) {
        lastImpactX = (int)player.x;
        if (lastImpactX < 30) lastImpactX = 30;
        if (lastImpactX > 450) lastImpactX = 450;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotThunderBolt;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->param_i[0] = lastImpactX; // 着弾点を渡す
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        splashTimer = 18; // 18F後に水しぶき
    }

    if (splashTimer > 0) {
        splashTimer--;
        if (splashTimer == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSplash;
            pSet->x = lastImpactX;
            pSet->y = 470.0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}