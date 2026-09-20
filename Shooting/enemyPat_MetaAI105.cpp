// enemyPat_Tmp.cpp - スイカ割り
// gv.h で定義されている前提: count, enemy, player, enemyShotSetHead, GetRand, DX_PI

static sEnemyShotSet* g_pSuikaSet = nullptr;

// --------------------------------------
// スイカ本体: 待機中は浮遊、割れたら飛散
// --------------------------------------
static void ShotSuikaStay(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 皮 24枚
        for (int i = 0; i < 24; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotMediumBall[2]; // 緑
            p->param_i[0] = 0; // 0:皮
            p->param_i[2] = 0; // 飛散初期化済みフラグ
            p->param_d[0] = (DX_TWO_PI / 24.0) * i; // base角度
            p->param_d[1] = 38.0; // 半径
            p->param_d[2] = 0.0; // 落下用vy

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
        // 種 12個
        for (int i = 0; i < 12; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotSmallBall[7]; // 黒
            p->param_i[0] = 1; // 1:種
            p->param_i[2] = 0;
            p->param_d[0] = (DX_TWO_PI / 12.0) * i + GetRand(20) / 100.0;
            p->param_d[1] = GetRand(18) + 2; // 中心からの距離
            p->param_d[2] = 0.0;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // param_i[0]: 0=待機, 1=割れた
    if (pSet->param_i[0] == 0) {
        double floatY = sin(pSet->count * 0.03) * 3.0;
        double rot = pSet->count * 0.008;
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 0) { // 皮
                double ang = p->param_d[0] + rot;
                p->x = pSet->x + cos(ang) * p->param_d[1];
                p->y = pSet->y + floatY + sin(ang) * p->param_d[1];
                p->muki = ang;
            }
            else { // 種
                double ang = p->param_d[0] + rot * 0.5;
                p->x = pSet->x + cos(ang) * p->param_d[1];
                p->y = pSet->y + floatY + sin(ang) * p->param_d[1];
            }
            p = p->next;
        }
    }
    else {
        // 飛散モード
        if (pSet->param_i[1] == 0) { // 初回だけ速度を付ける
            pSet->param_i[1] = 1;
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[2] == 0) {
                p->param_i[2] = 1;
                double baseAng = p->param_d[0];
                if (p->param_i[0] == 0) { // 皮は外側へ速く落下
                    p->muki = baseAng + (GetRand(40) - 20) / 180.0 * DX_PI;
                    p->speed = 2.0 + GetRand(150) / 100.0;
                }
                else { // 種は遅く
                    p->muki = baseAng + (GetRand(60) - 30) / 180.0 * DX_PI;
                    p->speed = 0.8 + GetRand(120) / 100.0;
                }
            }
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki) + p->param_d[2];
            p->param_d[2] += 0.07; // 重力
            p->speed *= 0.995;
            p = p->next;
        }
    }
}

// --------------------------------------
// 棒: 5本の短レーザーを束ねて1本に見せる
// --------------------------------------
static void ShotStick(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotLaser[8]; // 橙 = 木の棒っぽい
            p->param_d[0] = 25.0 + i * 48.0; // 柄からの距離
            p->count = 0;
            p->speed = 0;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // 40Fかけて start -> end に振る
    const double duration = 40.0;
    double t = pSet->count / duration;
    if (t > 1.0) t = 1.0;
    // イージング
    double eased = 0.5 - 0.5 * cos(t * DX_PI);
    double startAng = pSet->param_d[1];
    double endAng = pSet->param_d[2];
    pSet->muki = startAng + (endAng - startAng) * eased;

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double dist = p->param_d[0];
        p->x = pSet->x + cos(pSet->muki) * dist;
        p->y = pSet->y + sin(pSet->muki) * dist;
        p->muki = pSet->muki;
        if (count % 330 >= 329)p->margin = -999;
        p = p->next;
    }
}

// --------------------------------------
// 空振り風圧
// --------------------------------------
static void ShotWind(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < 12*3; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotSmallBall[6]; // 白
            p->x = pSet->x + GetRand(200+60) - 100-30;
            p->y = pSet->y + GetRand(30) - 15;
            p->muki = pSet->muki + (GetRand(20) - 10) / 180.0 * DX_PI;
            p->speed = 2.5 + GetRand(100) / 100.0 + 1;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// --------------------------------------
// 果肉飛散 - 割れた瞬間の本体弾幕
// --------------------------------------
static void ShotFleshScatter(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 赤い果肉 32発
        for (int i = 0; i < 32*2; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotSmallBall[0]; // 赤
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = DX_TWO_PI / 32.0 * i + GetRand(20) / 180.0 * DX_PI;
            p->speed = 1.8 + GetRand(180) / 100.0;
            p->param_d[0] = 0.0; // 重力

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
        // 追加の種 10発 - 隙間を埋める遅い弾
        for (int i = 0; i < 10*2; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotSmallBall[7]; // 黒
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = DX_TWO_PI / 10.0 * i + GetRand(10) / 180.0 * DX_PI;
            p->speed = 0.9 + GetRand(80) / 100.0;
            p->param_d[0] = 0.0;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki) + p->param_d[0];
        p->param_d[0] += 0.04;
        p->speed *= 0.998;
        p = p->next;
    }
}

// --------------------------------------
// 敵本体
// --------------------------------------
void EnemyPat_Suikawari_MetaAI()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        g_pSuikaSet = nullptr;
    }

    // 敵はほぼ固定
    enemy.x += sin(count * 0.02) * 0.2;

    const int T = 330;
    int countT = count % T;

    // 1. スイカ設置
    if (countT == 10) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSuikaStay;
        pSet->x = 240.0;
        pSet->y = 200.0;
        pSet->param_i[0] = 0; // 0:待機 1:割れた
        pSet->param_i[1] = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        g_pSuikaSet = pSet;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 2. 空振り1: 左に外す
    if (countT == 80) {
        double sx = g_pSuikaSet ? g_pSuikaSet->x : 240.0;
        double sy = g_pSuikaSet ? g_pSuikaSet->y : 200.0;
        double base = atan2(sy - enemy.y, (sx - 80.0) - enemy.x);

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotStick;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->param_d[1] = base - 60.0 / 180.0 * DX_PI; // start
        pSet->param_d[2] = base + 60.0 / 180.0 * DX_PI; // end
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    if (countT == 120-20) { // 風圧
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotWind;
        pSet->x = enemy.x;
        pSet->y = 120.0;
        pSet->muki = DX_PI * 0.5 + (GetRand(20) - 10) / 180.0 * DX_PI + 0.2;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 3. 空振り2: 右に外す
    if (countT == 160) {
        double sx = g_pSuikaSet ? g_pSuikaSet->x : 240.0;
        double sy = g_pSuikaSet ? g_pSuikaSet->y : 200.0;
        double base = atan2(sy - enemy.y, (sx + 80.0) - enemy.x);
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotStick;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->param_d[1] = base + 60.0 / 180.0 * DX_PI;
        pSet->param_d[2] = base - 60.0 / 180.0 * DX_PI;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    if (countT == 200-20) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotWind;
        pSet->x = enemy.x;
        pSet->y = 120.0;
        pSet->muki = DX_PI * 0.5 + (GetRand(20) - 10) / 180.0 * DX_PI - 0.2;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 4. 命中 - スイカ割り
    if (countT == 240) {
        double sx = g_pSuikaSet ? g_pSuikaSet->x : 240.0;
        double sy = g_pSuikaSet ? g_pSuikaSet->y : 200.0;
        double base = atan2(sy - enemy.y, sx - enemy.x);
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotStick;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->param_d[1] = base - 70.0 / 180.0 * DX_PI;
        pSet->param_d[2] = base; // スイカ中心で止まる
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    if (countT == 280) {
        double sx = g_pSuikaSet ? g_pSuikaSet->x : 240.0;
        double sy = g_pSuikaSet ? g_pSuikaSet->y : 200.0;

        // スイカに割れたフラグを立てる
        if (g_pSuikaSet) g_pSuikaSet->param_i[0] = 1;

        // 果肉飛散弾幕生成
        sEnemyShotSet* pFlesh = new sEnemyShotSet;
        pFlesh->count = 0;
        pFlesh->patternFunc = ShotFleshScatter;
        pFlesh->x = sx;
        pFlesh->y = sy;
        pFlesh->pEnemyShotHead = new sEnemyShot;
        pFlesh->pEnemyShotHead->prev = pFlesh->pEnemyShotHead;
        pFlesh->pEnemyShotHead->next = pFlesh->pEnemyShotHead;
        pFlesh->prev = enemyShotSetHead.prev;
        pFlesh->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pFlesh;
        enemyShotSetHead.prev = pFlesh;
    }
}