// enemyPat_brazilianWax.cpp
// 弾幕：剥離！ブラジリアンワックス

// ワックスの帯を「塗る -> 貼り付ける -> 一気に剥がす -> 飛散」させる。
// 弾の種類を組み合わせて、中央のワックス片と付着した小さな粒を表現する。
static void ShotBrazilianWax(sEnemyShotSet* pEnemyShotSet)
{
    const double PI2 = DX_PI * 2.0;
    const int phasePull = 78;
    const int phasePeel = 118;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const double dir = (pEnemyShotSet->kind & 1) ? 1.0 : -1.0;
        pEnemyShotSet->param_d[0] = dir;

        // ワックス本体：横長の帯を中楕円弾で形成。
        const int rows = 4*2;
        const int cols = 12*2;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                sEnemyShot* p = new sEnemyShot;
                const double u = -1.0 + 2.0 * c / (double)(cols - 1);
                const double v = -1.0 + 2.0 * r / (double)(rows - 1);

                p->param_i[0] = 0; // ワックス本体
                p->param_d[0] = u;
                p->param_d[1] = v;
                p->param_d[2] = (r & 1) ? 0.035 : -0.035;
                p->muki = 0.0;
                p->speed = 0.0;
                p->kind = img_enemyShotMediumOval[(r + c) % 8];

                p->x = pEnemyShotSet->x + u * 142.0/2;
                p->y = pEnemyShotSet->y + v * 22.0/2;
                p->margin = 240;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }

        // 付着している小さな粒。剥離後に「巻き込まれる毛」のように飛散する。
        for (int i = 0; i < 36*2; ++i) {
            sEnemyShot* p = new sEnemyShot;
            const double a = PI2 * i / 36.0/2;
            const double edge = (i & 1) ? 25.0 : -25.0;
            const double u = cos(a) * 132.0;
            const double v = sin(a) * 1.0 + edge * 0.25;

            p->param_i[0] = 1; // 付着粒
            p->param_d[0] = u;
            p->param_d[1] = v;
            p->param_d[2] = a;
            p->muki = a;
            p->speed = 0.0;
            p->kind = img_enemyShotSmallBall[i % 8];

            p->x = pEnemyShotSet->x + u;
            p->y = pEnemyShotSet->y + v;
            p->margin = 240;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }

        // 剥離の先端に相当する大玉。
        for (int i = 0; i < 4; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->param_i[0] = 2; // 剥離先端
            p->param_d[0] = (i & 1) ? 1.0 : -1.0;
            p->param_d[1] = -1.0 + 2.0 * (i / 2.0);
            p->param_d[2] = 0.0;
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = img_enemyShotLargeBall[i % 8];
            p->x = pEnemyShotSet->x + p->param_d[0] * 150.0;
            p->y = pEnemyShotSet->y + p->param_d[1] * 18.0;
            p->margin = 240;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 剥離開始時に強い効果音。
    if (pEnemyShotSet->count == phasePeel) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    const double dir = pEnemyShotSet->param_d[0];
    const double t = (double)pEnemyShotSet->count;
    const double pull = (t < phasePull) ? 0.0 : (t < phasePeel ? (t - phasePull) / (double)(phasePeel - phasePull) : 1.0);
    const double peelT = t - phasePeel;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double u = pShot->param_d[0];
        const double v = pShot->param_d[1];

        if (pShot->param_i[0] == 0) {
            // 帯そのもの。貼り付くまではほぼ一枚のシートとして動かす。
            const double baseX = pEnemyShotSet->x + u * 142.0;
            const double baseY = pEnemyShotSet->y + v * 22.0;
            const double sway = 5.0 * sin(0.07 * t + u * 2.5);
            const double peelBend = 52.0 * pull * u * u;

            pShot->x = baseX + dir * (92.0 * pull + peelBend);
            pShot->y = baseY + sway + dir * 18.0 * pull * u;

            if (t >= phasePeel) {
                // 剥がれたワックスが弧を描いて飛んでいく。
                const double local = peelT;
                const double a = dir * (0.60 + 0.75 * u) + pShot->param_d[2];
                const double dx = dir * 2.15 * local + 0.42 * local * local * sin(a);
                const double dy = 1.10 * local * (0.4 + fabs(u)) * sin(a) + 0.018 * local * local * (v > 0.0 ? 1.0 : -1.0);
                const double peelX = pEnemyShotSet->x + dir * (92.0 + 52.0 * u * u);
                const double peelY = pEnemyShotSet->y + v * 22.0 + 18.0 * dir * u + 5.0 * sin(0.07 * phasePeel + u * 2.5);
                pShot->x = peelX + dx;
                pShot->y = peelY + dy;
                pShot->muki = a;
            }
        }
        else if (pShot->param_i[0] == 1) {
            // 小粒は貼り付いたまま帯に追従し、剥離時に一斉に吹き飛ぶ。
            const double a0 = pShot->param_d[2];
            const double baseX = pEnemyShotSet->x + u;
            const double baseY = pEnemyShotSet->y + v + 7.0 * sin(a0 * 3.0 + 0.05 * t);

            if (t < phasePeel) {
                pShot->x = baseX + dir * 92.0 * pull;
                pShot->y = baseY + dir * 13.0 * pull * sin(a0);
            }
            else {
                const double s = peelT;
                const double launch = a0 + dir * (0.45 + 0.9 * sin(a0));
                const double speed = 2.8 + 0.75 * (0.5 + 0.5 * sin(a0 * 2.0));
                const double wave = 22.0 * sin(0.16 * s + a0 * 2.0);
                pShot->x = baseX + dir * 92.0 + cos(launch) * speed * s + dir * wave * 0.35;
                pShot->y = baseY + dir * 13.0 + sin(launch) * speed * s + wave;
                pShot->muki = launch;
            }
        }
        else {
            // 大玉の先端は剥離の軌道を強調するアクセント。
            const double side = pShot->param_d[0];
            const double row = pShot->param_d[1];
            if (t < phasePeel) {
                pShot->x = pEnemyShotSet->x + side * (150.0 + 75.0 * pull);
                pShot->y = pEnemyShotSet->y + row * 18.0 + 28.0 * pull * side;
            }
            else {
                const double s = peelT;
                const double ang = dir * (row > 0.0 ? 0.82 : -0.82);
                pShot->x = pEnemyShotSet->x + side * 225.0 + cos(ang) * (2.6 + 0.04 * s) * s;
                pShot->y = pEnemyShotSet->y + row * 30.0 + sin(ang) * (2.6 + 0.04 * s) * s;
                pShot->muki = ang;
            }
        }

        pShot = pShot->next;
    }
}

void EnemyPat_BrazilianWax_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 95.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.82 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 剥離方向が交互に変わるよう、一定間隔でワックス片を生成。
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBrazilianWax;
        pEnemyShotSet->x = enemy.x + (shot_count & 1 ? -100 : 100);
        pEnemyShotSet->y = enemy.y + 38.0;
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