// EnemyPat_Ikaruga_MetaAI.cpp - 斑鳩風吸収弾幕

//------------------------------------------------------------
// 定数
//------------------------------------------------------------
static const int SHIELD_NUM = 12;
static const double SHIELD_R = 45.0;
static const double ABSORB_R2 = 12.0 * 12.0; // (小玉2.5+中玉7.0 +余裕)
static const int WAY = 9;
static const double TOTAL_SPREAD = 72.0 * DX_PI / 180.0;

//------------------------------------------------------------
// 赤弾 - ボス1
//------------------------------------------------------------
static void ShotRed(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double base = pSet->muki;
        for (int i = 0; i < WAY; i++) {
            sEnemyShot* p = new sEnemyShot;
            double t = (WAY == 1) ? 0.0 : (double)i / (WAY - 1);
            double offset = -TOTAL_SPREAD * 0.5 + TOTAL_SPREAD * t;
            offset += (GetRand(10) - 5) / 180.0 * DX_PI * 0.5; // 微ランダム

            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = base + offset;
            p->speed = 3.0 + GetRand(40) / 100.0; // 3.0-3.4
            p->kind = img_enemyShotMediumBall[0]; // 赤中玉
            p->param_i[0] = 0; // 赤識別
            p->count = 0;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
    }
}

//------------------------------------------------------------
// 青弾 - ボス2
//------------------------------------------------------------
static void ShotBlue(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double base = pSet->muki;
        for (int i = 0; i < WAY; i++) {
            sEnemyShot* p = new sEnemyShot;
            double t = (WAY == 1) ? 0.0 : (double)i / (WAY - 1);
            double offset = -TOTAL_SPREAD * 0.5 + TOTAL_SPREAD * t + (TOTAL_SPREAD / WAY * 0.5); // 赤と半分ずらす
            offset += (GetRand(10) - 5) / 180.0 * DX_PI * 0.5;

            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = base + offset;
            p->speed = 3.2 + GetRand(40) / 100.0;
            p->kind = img_enemyShotMediumBall[4]; // 青中玉
            p->param_i[0] = 4; // 青識別
            p->count = 0;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
    }
}

//------------------------------------------------------------
// 吸収シールド - 自機周囲
// param_i[0]: 0=赤シールド / 1=青シールド
// param_i[1]: 0=未生成 / 1=生成済み
// param_d[1]: 配置角度
//------------------------------------------------------------
static void ShotShield(sEnemyShotSet* pSet)
{
    // 0秒と3秒おきの予告
    if (pSet->count == 0 || (pSet->count % 180 == 0)) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 1秒後と4秒おきの展開/反転
    if (pSet->count % 180 == 60) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        if (pSet->param_i[1] == 0) {
            // 初回生成 - 赤
            for (int i = 0; i < SHIELD_NUM; i++) {
                sEnemyShot* p = new sEnemyShot;
                double ang = 2.0 * DX_PI * i / SHIELD_NUM;
                p->x = player.x + SHIELD_R * cos(ang);
                p->y = player.y + SHIELD_R * sin(ang);
                p->kind = img_enemyShotSmallBall[0];
                p->margin = 999.0;
                p->param_d[1] = ang;
                p->count = 0;

                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
            pSet->param_i[0] = 0;
            pSet->param_i[1] = 1;
        }
        else {
            // 極性反転
            pSet->param_i[0] = 1 - pSet->param_i[0];
            int newKind = (pSet->param_i[0] == 0) ? img_enemyShotSmallBall[0] : img_enemyShotSmallBall[4];
            for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
                s->kind = newKind;
                s->margin = 999.0;
            }
        }
    }

    // 自機追従・回転
    for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
        s->param_d[1] += 0.02; // ゆっくり回転
        s->x = player.x + SHIELD_R * cos(s->param_d[1]);
        s->y = player.y + SHIELD_R * sin(s->param_d[1]);
    }

    // 吸収判定
    bool isRedShield = (pSet->param_i[0] == 0);
    for (sEnemyShotSet* pOther = enemyShotSetHead.next; pOther != &enemyShotSetHead; pOther = pOther->next) {
        if (pOther == pSet) continue;
        if (pOther->pEnemyShotHead == nullptr) continue;

        sEnemyShot* cur = pOther->pEnemyShotHead->next;
        while (cur != pOther->pEnemyShotHead) {
            sEnemyShot* nxt = cur->next;
            bool isRedBullet = (cur->kind == img_enemyShotMediumBall[0]);
            bool isBlueBullet = (cur->kind == img_enemyShotMediumBall[4]);

            bool shouldAbsorb = (isRedShield && isRedBullet) || (!isRedShield && isBlueBullet);
            if (shouldAbsorb) {
                for (sEnemyShot* sh = pSet->pEnemyShotHead->next; sh != pSet->pEnemyShotHead; sh = sh->next) {
                    double dx = cur->x - sh->x;
                    double dy = cur->y - sh->y;
                    if (dx * dx + dy * dy < ABSORB_R2) {
                        cur->prev->next = cur->next;
                        cur->next->prev = cur->prev;
                        delete cur;
                        break;
                    }
                }
            }
            cur = nxt;
        }
    }
}

//------------------------------------------------------------
// 敵本体
//------------------------------------------------------------
void EnemyPat_Ikaruga_MetaAI()
{
    static int muki;

    if (count == 1) {
        enemy.x = 120.0;
        enemy.y = 40.0;
        enemy.x2 = 360.0;
        enemy.y2 = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // シールドSetを1つだけ生成
        sEnemyShotSet* pShield = new sEnemyShotSet;
        pShield->count = 0;
        pShield->patternFunc = ShotShield;
        pShield->x = player.x;
        pShield->y = player.y;
        pShield->param_i[0] = 0;
        pShield->param_i[1] = 0;
        pShield->pEnemyShotHead = new sEnemyShot;
        pShield->pEnemyShotHead->prev = pShield->pEnemyShotHead;
        pShield->pEnemyShotHead->next = pShield->pEnemyShotHead;

        pShield->prev = enemyShotSetHead.prev;
        pShield->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pShield;
        enemyShotSetHead.prev = pShield;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        enemy.x2 += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 10Fおきに赤と青を同時発射
    if (count % 10 == 1) {
        // 赤 - ボス1
        sEnemyShotSet* pRed = new sEnemyShotSet;
        pRed->count = 0;
        pRed->patternFunc = ShotRed;
        pRed->x = enemy.x;
        pRed->y = enemy.y + 10.0;
        pRed->muki = atan2(player.y - pRed->y, player.x - pRed->x);
        pRed->param_i[0] = 0;
        pRed->pEnemyShotHead = new sEnemyShot;
        pRed->pEnemyShotHead->prev = pRed->pEnemyShotHead;
        pRed->pEnemyShotHead->next = pRed->pEnemyShotHead;
        pRed->prev = enemyShotSetHead.prev;
        pRed->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pRed;
        enemyShotSetHead.prev = pRed;

        // 青 - ボス2
        sEnemyShotSet* pBlue = new sEnemyShotSet;
        pBlue->count = 0;
        pBlue->patternFunc = ShotBlue;
        pBlue->x = enemy.x2;
        pBlue->y = enemy.y2 + 10.0;
        pBlue->muki = atan2(player.y - pBlue->y, player.x - pBlue->x);
        pBlue->param_i[0] = 4;
        pBlue->pEnemyShotHead = new sEnemyShot;
        pBlue->pEnemyShotHead->prev = pBlue->pEnemyShotHead;
        pBlue->pEnemyShotHead->next = pBlue->pEnemyShotHead;
        pBlue->prev = enemyShotSetHead.prev;
        pBlue->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBlue;
        enemyShotSetHead.prev = pBlue;
    }
}