// enemyPat_suika.cpp
// 弾幕：西瓜割り・真夏の一太刀
// enemyPat_sampleForAI.cpp の既存素材のみを使用。
// 入口は void EnemyPat_Suikawari_ChatGPT()。

// ------------------------------------------------------------
// 西瓜本体
//   緑: LargeBall
//   赤: MediumOval
//   黒: SmallBall
// 割る棒
//   白: MediumBall を一直線に並べて表現
// 割れた後は各弾が左右へ飛散し、最後に再集合する。
// ------------------------------------------------------------
static void ShotWatermelon(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // count == 0 : 西瓜を生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 皮の外周
        const int RIND_N = 34;
        for (int i = 0; i < RIND_N; ++i) {
            double a = 2.0 * DX_PI * i / RIND_N;
            double x = pEnemyShotSet->x + 76.0 * cos(a);
            double y = pEnemyShotSet->y + 58.0 * sin(a);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x;
            pEnemyShot->y = y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLargeBall[2]; // 緑
            pEnemyShot->param_i[0] = 0; // 種別: 皮
            pEnemyShot->param_i[1] = i;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 果肉
        const int MEAT_N = 28;
        for (int i = 0; i < MEAT_N; ++i) {
            double a = 2.0 * DX_PI * i / MEAT_N + 0.05;
            double rr = 43.0 + 6.0 * sin(a * 3.0);
            double x = pEnemyShotSet->x + rr * cos(a);
            double y = pEnemyShotSet->y + 33.0 * sin(a);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x;
            pEnemyShot->y = y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumOval[0]; // 赤
            pEnemyShot->param_i[0] = 1; // 種別: 果肉
            pEnemyShot->param_i[1] = i;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 種
        const int SEED_N = 22;
        for (int i = 0; i < SEED_N; ++i) {
            double a = 2.0 * DX_PI * i / SEED_N;
            double rr = 18.0 + 17.0 * ((i * 7) % 11) / 10.0;
            double x = pEnemyShotSet->x + rr * cos(a);
            double y = pEnemyShotSet->y + 28.0 * sin(a);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x;
            pEnemyShot->y = y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒
            pEnemyShot->param_i[0] = 2; // 種別: 種
            pEnemyShot->param_i[1] = i;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 予備動作 -> 一刀 -> 飛散 -> 再集合
    const int t = pEnemyShotSet->count;
    const int SWING_START = 80;
    const int SPLIT_START = 96;
    const int REFORM_START = 175;

    // 西瓜の全体位置を少し揺らして「狙いを定める」感じを出す。
    double cx = pEnemyShotSet->x;
    double cy = pEnemyShotSet->y;

    if (t < SWING_START) {
        cx += 10.0 * sin(0.08 * t);
        cy += 5.0 * sin(0.12 * t);
    }

    // 白い棒を一時的に作る専用の弾集合。
    // pEnemyShotSet->param_i[0] を生成済みフラグに利用。
    if (t == SWING_START) {
        for (int i = -8; i <= 8; ++i) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + 122.0;
            pEnemyShot->y = pEnemyShotSet->y - 145.0 + i * 16.0;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白
            pEnemyShot->param_i[0] = 3; // 種別: 棒
            pEnemyShot->param_i[1] = i;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        pEnemyShotSet->param_i[0] = 1;
    }

    // 全弾を更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int kind = pShot->param_i[0];
        int id = pShot->param_i[1];

        if (kind == 3) {
            // 棒は上方から斜めに振り下ろす。
            double u = (t - SWING_START) / 18.0;
            if (u < 0.0) u = 0.0;
            if (u > 1.0) u = 1.0;
            double a = -1.15 + 2.15 * u;
            double len = 165.0;
            double off = id * 10.5;
            pShot->x = cx + cos(a) * len + sin(a) * off;
            pShot->y = cy + sin(a) * len - cos(a) * off;
            pShot->muki = a;
        }
        else if (t < SPLIT_START) {
            // 割れるまでは西瓜の形を保つ。
            if (kind == 0) {
                double a = 2.0 * DX_PI * id / 34.0;
                pShot->x = cx + 76.0 * cos(a);
                pShot->y = cy + 58.0 * sin(a);
            }
            else if (kind == 1) {
                double a = 2.0 * DX_PI * id / 28.0 + 0.05;
                double rr = 43.0 + 6.0 * sin(a * 3.0);
                pShot->x = cx + rr * cos(a);
                pShot->y = cy + 33.0 * sin(a);
            }
            else if (kind == 2) {
                double a = 2.0 * DX_PI * id / 22.0;
                double rr = 18.0 + 17.0 * ((id * 7) % 11) / 10.0;
                pShot->x = cx + rr * cos(a);
                pShot->y = cy + 28.0 * sin(a);
            }
        }
        else if (t < REFORM_START) {
            if (t == SPLIT_START) {
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            }

            // 割れた瞬間に左右へ飛び散り、その後ゆっくり内側へ戻す。
            double local = (t - SPLIT_START) / (double)(REFORM_START - SPLIT_START);
            double side = (id & 1) ? 1.0 : -1.0;
            double a = side * (0.20 + 1.10 * local);
            double push = (kind == 0 ? 95.0 : (kind == 1 ? 78.0 : 65.0));
            double back = 120.0 * local * local;
            double baseX = cx + side * (kind == 0 ? 38.0 : 28.0);
            double baseY = cy + (kind == 0 ? 44.0 : (kind == 1 ? 25.0 : 18.0));

            pShot->x = baseX + cos(a) * (push + 55.0 * local) + 18.0 * cos(0.35 * id + 0.08 * t) - side * back;
            pShot->y = baseY + sin(a) * (push + 30.0 * local) + 18.0 * sin(0.35 * id + 0.08 * t);
            pShot->muki = a;
        }
        else if (t < REFORM_START + 50) {
            // 再集合。元の西瓜へ弾が吸い込まれる。
            double targetX;
            double targetY;
            double a;

            if (kind == 0) {
                a = 2.0 * DX_PI * id / 34.0;
                targetX = cx + 76.0 * cos(a);
                targetY = cy + 58.0 * sin(a);
            }
            else if (kind == 1) {
                a = 2.0 * DX_PI * id / 28.0 + 0.05;
                double rr = 43.0 + 6.0 * sin(a * 3.0);
                targetX = cx + rr * cos(a);
                targetY = cy + 33.0 * sin(a);
            }
            else {
                a = 2.0 * DX_PI * id / 22.0;
                double rr = 18.0 + 17.0 * ((id * 7) % 11) / 10.0;
                targetX = cx + rr * cos(a);
                targetY = cy + 28.0 * sin(a);
            }

            double k = (t - REFORM_START) / 24.0;
            if (k > 1.0) k = 1.0;
            pShot->x += (targetX - pShot->x) * 0.16;
            pShot->y += (targetY - pShot->y) * 0.16;
            pShot->muki = atan2(targetY - pShot->y, targetX - pShot->x);
        }
        else {
            if (t == REFORM_START + 50) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                pShot->speed = 1.0 + GetRand(300) / 100.0;
                pShot->muki = GetRand(100) / 100.0 * DX_PI * 2.0;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }

    // 棒は短時間だけ表示し、割れた後は消す。
    if (t >= SPLIT_START) {
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* next = pShot->next;
            if (pShot->param_i[0] == 3) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
            pShot = next;
        }
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_Suikawari_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ボスは画面上部をゆっくり往復。
        enemy.x += 0.90 * (double)muki;
        if (enemy.x < 70.0 || enemy.x > 410.0)
            muki *= -1;
    }

    // 西瓜を画面中央付近に出す。
    // 一定間隔で繰り返すが、前のセットは十分長く残るため、
    // 連発しすぎず「割って再生する」一連の見せ場を維持する。
    if (count % 270 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWatermelon;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 265.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
