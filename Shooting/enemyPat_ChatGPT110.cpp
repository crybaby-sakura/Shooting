// enemyPat_nagewa.cpp

// 輪投げの「輪」を構成する弾幕
// 1つの sEnemyShotSet が1本の輪を担当し、円周上の弾を絶対座標で配置する。
// count / pEnemyShotSet->count / pEnemyShot->count の加算や画面外弾の消去はメインルーチン側で行う。
static void ShotHoop(sEnemyShotSet* pEnemyShotSet)
{
    const int ringBulletCount = 64/2;
    const int throwFrames = 72;
    const int holdFrames = 24;

    if (pEnemyShotSet->count == 0) {
        // param_i[0]: 輪の状態（0=投げる、1=一瞬静止、2=輪を崩す）
        // param_i[1]: 輪の色（0=シアン、1=青）
        // param_d[0]: 初期角度
        // param_d[1]: 狙った位置X
        // param_d[2]: 狙った位置Y
        // param_d[3]: 輪の基本半径
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_d[0] = atan2(
            pEnemyShotSet->param_d[2] - pEnemyShotSet->y,
            pEnemyShotSet->param_d[1] - pEnemyShotSet->x);

        for (int i = 0; i < ringBulletCount; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            const double t = (double)i / ringBulletCount;
            const double angle = pEnemyShotSet->param_d[0] + DX_PI * 2.0 * t;

            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * pEnemyShotSet->param_d[3];
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * pEnemyShotSet->param_d[3];
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = (pEnemyShotSet->param_i[1] == 0)
                ? img_enemyShotMediumBall[3]
                : img_enemyShotMediumBall[4];
            pEnemyShot->margin = 999.0;
            pEnemyShot->param_i[0] = i;
            pEnemyShot->param_i[1] = ringBulletCount;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 輪の中心を狙った位置へ運ぶ。
    if (pEnemyShotSet->count < throwFrames) {
        const double t = (double)pEnemyShotSet->count / throwFrames;
        // 滑らかに減速して着弾地点へ向かう。
        const double ease = 1.0 - (1.0 - t) * (1.0 - t);
        pEnemyShotSet->x = pEnemyShotSet->param_d[5]
            + (pEnemyShotSet->param_d[1] - pEnemyShotSet->param_d[5]) * ease;
        pEnemyShotSet->y = pEnemyShotSet->param_d[6]
            + (pEnemyShotSet->param_d[2] - pEnemyShotSet->param_d[6]) * ease;
    }
    else if (pEnemyShotSet->count < throwFrames + holdFrames) {
        // 輪がプレイヤー周辺に来たところで一瞬だけ形を見せる。
        pEnemyShotSet->x = pEnemyShotSet->param_d[1];
        pEnemyShotSet->y = pEnemyShotSet->param_d[2];
    }
    else {
        pEnemyShotSet->param_i[0] = 2;
    }

    const int burstCount = pEnemyShotSet->count - throwFrames - holdFrames;
    const double spin = 0.035 * pEnemyShotSet->count;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int index = pShot->param_i[0];
        const double baseAngle = pEnemyShotSet->param_d[0]
            + DX_PI * 2.0 * (double)index / pShot->param_i[1];

        if (pEnemyShotSet->count < throwFrames + holdFrames) {
            // 投げられた輪。円周を保ったままゆっくり回転する。
            const double angle = baseAngle + spin;
            const double wobble = 1.0 + 0.07 * sin(pEnemyShotSet->count * 0.08 + index * 0.25);
            const double radius = pEnemyShotSet->param_d[3] * wobble;

            pShot->x = pEnemyShotSet->x + cos(angle) * radius;
            pShot->y = pEnemyShotSet->y + sin(angle) * radius;
            pShot->muki = angle;
            pShot->speed = 0.0;
        }
        else {
            // 輪が崩れ、円周の各弾が放射状に飛び散る。
            const int localCount = burstCount;
            const double angle = baseAngle + spin * 1.7 / 10;
            const double burstEase = 0.35 + localCount * 0.055;
            const double radius = pEnemyShotSet->param_d[3] * 0.85 + burstEase * localCount/2;

            pShot->x = pEnemyShotSet->x + cos(angle) * radius;
            pShot->y = pEnemyShotSet->y + sin(angle) * radius;
            pShot->muki = angle;
            pShot->speed = 0.0;
        }

        pShot = pShot->next;
    }

    if (pEnemyShotSet->count == throwFrames - 36) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    if (burstCount == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
}

// 敵本体のパターン
void EnemyPat_RingToss_ChatGPT()
{
    static int muki;
    static int hoop_count;

    if (count == 1) {
        // ゲーム画面は480x480
        enemy.x = 240.0;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 100;
        muki = 1;
        hoop_count = 0;
    }
    else {
        enemy.x += 0.85 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 輪を連続で投げ、少しずつ違う位置・角度から迫らせる。
    if (count % 60 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHoop;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->param_i[1] = hoop_count % 2;

        // 投げ始めた瞬間のプレイヤー位置を記録。移動後も狙いがぶれない。
        pEnemyShotSet->param_d[1] = player.x + cos(hoop_count * 0.9) * (55.0 + (hoop_count % 3) * 18.0);
        pEnemyShotSet->param_d[2] = player.y + sin(hoop_count * 0.8) * (40.0 + (hoop_count % 2) * 22.0);
        pEnemyShotSet->param_d[3] = 42.0 + (hoop_count % 3) * 7.0;
        pEnemyShotSet->param_d[5] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[6] = pEnemyShotSet->y;
        hoop_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        if (hoop_count % 3 == 1) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }
}
