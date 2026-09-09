// ============================================================
// ファランクス弾幕
// 横一列の中玉を密集した隊列として進軍させ、
// 中盤で扇状展開→交差→再編成を行う。
// ============================================================
static void ShotPhalanx(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    const double PI2 = DX_PI * 2.0;
    const int row = pSet->param_i[0];       // 隊列番号
    const int dir = pSet->param_i[1];       // 左右進行方向
    const int gap = pSet->param_i[2];       // 隊列の切れ目
    const int count = pSet->count;

    while (pShot != pSet->pEnemyShotHead) {
        const int i = pShot->param_i[0];    // 隊列内の位置
        const int phase = pShot->param_i[1];// 0=通常 1=扇 2=交差
        const double baseX = pShot->param_d[0];
        const double baseY = pShot->param_d[1];
        const double spacing = pShot->param_d[2];

        double x = baseX;
        double y = baseY + count * 1.85;

        // 序盤：隊列を保ったままゆっくり左右に揺れる
        x += 34.0 * sin(0.018 * count + row * 0.72);

        // 中盤：隊列が扇状に展開
        if (phase == 1 && count >= 70) {
            const double t = (count - 70 < 80 ? (count - 70) / 80.0 : 1.0);
            const double spread = t * t;
            const double center = 239.5 + 78.0 * sin(0.015 * count + row * 0.55);
            const double angle = (i - 7.0) * 0.075 * spread;
            const double r = 95.0 + 2.8 * (count - 70);
            x = center + r * sin(angle);
            y = baseY + count * 1.55 + r * (1.0 - cos(angle)) * dir * 0.55;
        }

        // 後半：左右の隊列が交差して中央で再編成
        if (phase == 2 && count >= 55) {
            const double t = (count - 55 < 105 ? (count - 55) / 105.0 : 1.0);
            const double s = sin(DX_PI * t);
            const double side = (i < 7 ? -1.0 : 1.0);
            x = 239.5 + side * (156.0 - 122.0 * s) + (i - 7) * spacing * (1.0 - 0.35 * s);
            y = baseY + count * 1.7 + 28.0 * s * cos(0.06 * count + row);
        }

        // 画面端からの逃げすぎを抑え、隊列感を維持
        if (x < 18.0) x = 18.0;
        if (x > 462.0) x = 462.0;

        pShot->x = x;
        pShot->y = y;

        // 隊列内の切れ目は時間で左右に移動させる
        // （弾そのものは消さず、配置によって突破口を作る）
        if (pShot->param_i[2] == 1) {
            const int movingGap = (gap + (count / 18) * dir + 21) % 21;
            if (i == movingGap || i == movingGap + 1) {
                pShot->x = -100.0;
                pShot->y = -100.0;
            }
        }

        pShot->muki = atan2(1.0, 0.0);
        pShot->speed = 0.0;

        pShot = pShot->next;
    }
}

static void AddPhalanxSet(int row, int dir, int phase, int gap)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotPhalanx;
    pSet->x = enemy.x;
    pSet->y = enemy.y + 20.0;
    pSet->muki = DX_PI * 0.5;
    pSet->kind = row;

    pSet->param_i[0] = row;
    pSet->param_i[1] = dir;
    pSet->param_i[2] = gap;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    const double spacing = 20.0;
    const double startX = 39.5;
    const int n = 21;

    for (int i = 0; i < n; ++i) {
        // 先頭と最後尾は菱形弾、それ以外は中玉で隊列を表現
        sEnemyShot* pShot = new sEnemyShot;
        pShot->param_i[0] = i;
        pShot->param_i[1] = phase;
        pShot->param_i[2] = (row % 2 == 0 ? 1 : 0);
        pShot->param_d[0] = startX + i * spacing;
        pShot->param_d[1] = enemy.y + 26.0;
        pShot->param_d[2] = spacing;
        pShot->x = pShot->param_d[0];
        pShot->y = pShot->param_d[1];
        pShot->muki = DX_PI * 0.5;
        pShot->speed = 0.0;

        if (i == 0 || i == n - 1)
            pShot->kind = img_enemyShotDiamond[8]; // 橙の菱形弾を槍先に見立てる
        else
            pShot->kind = img_enemyShotMediumBall[1]; // 黄の中玉を兵士に見立てる
        pShot->margin = 240;

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_Phalanx_ChatGPT()
{
    static int muki;
    static int wave;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        wave = 0;
    }
    else {
        enemy.x += 1.05 * muki;
        if (enemy.x < 55.0 || enemy.x > 425.0)
            muki *= -1;
    }

    // 交互に左右へ押し出す横隊を連続投入
    if (count % 34 == 1) {
        const int row = wave++;
        const int dir = (row % 2 == 0 ? 1 : -1);
        const int phase = (row % 3 == 0 ? 1 : 2);
        const int gap = (row * 4) % 19;
        AddPhalanxSet(row, dir, phase, gap);

        if (row % 2 == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }
}
