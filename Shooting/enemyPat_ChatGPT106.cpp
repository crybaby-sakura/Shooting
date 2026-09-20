// enemyPat_IkarugaDual.cpp
// 斑鳩風：赤のボスと青のボスが別々の属性弾幕を撃ち、
// 自機を囲む属性リングが対応する色の敵弾だけを消滅させる。

// ============================================================
//  共通：弾をセットへ追加
// ============================================================
static void AddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// ============================================================
//  赤ボス：赤弾幕
//  単独なら自機狙いの隙間を作りやすい放射＋扇。
// ============================================================
static void ShotRedBoss(sEnemyShotSet* pEnemyShotSet)
{
    const int t = pEnemyShotSet->count;
    const double cx = pEnemyShotSet->x;
    const double cy = pEnemyShotSet->y;

    // 周期的に円形の弾列を展開する。
    if (t == 0 || t % 12 == 0) {
        const double base = 0.055 * t + pEnemyShotSet->param_d[0];

        for (int i = 0; i < 18; ++i) {
            const double a = base + DX_TWO_PI * i / 18.0;
            AddShot(pEnemyShotSet, cx, cy, a,
                2.15 + 0.15 * (i % 3), img_enemyShotMediumBall[0]);
        }

        // 自機付近へ向かう扇を重ね、単色時の読みやすさを保つ。
        const double aim = atan2(player.y - cy, player.x - cx);
        for (int i = 0; i < 5; ++i) {
            const double spread = (i - 2) * 0.10;
            AddShot(pEnemyShotSet, cx, cy, aim + spread,
                3.0 + 0.15 * (i % 2), img_enemyShotBullet[0]);
        }
    }

    // 時々、逆側へ寄った扇を追加して赤弾だけでも横移動を要求する。
    if (t > 0 && t % 60 == 30) {
        const double aim = atan2(player.y - cy, player.x - cx);
        for (int i = 0; i < 9; ++i) {
            const double a = aim + ((t / 60) & 1 ? 0.42 : -0.42)
                + (i - 4) * 0.085;
            AddShot(pEnemyShotSet, cx, cy, a, 2.55, img_enemyShotBullet[0]);
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  青ボス：青弾幕
//  赤とは位相をずらし、同時に存在すると交差するようにする。
// ============================================================
static void ShotBlueBoss(sEnemyShotSet* pEnemyShotSet)
{
    const int t = pEnemyShotSet->count;
    const double cx = pEnemyShotSet->x;
    const double cy = pEnemyShotSet->y;

    if (t == 0 || t % 12 == 0) {
        const double base = -0.055 * t + pEnemyShotSet->param_d[0];

        for (int i = 0; i < 18; ++i) {
            const double a = base + DX_TWO_PI * i / 18.0;
            AddShot(pEnemyShotSet, cx, cy, a,
                2.15 + 0.15 * ((i + 1) % 3), img_enemyShotMediumBall[4]);
        }

        const double aim = atan2(player.y - cy, player.x - cx);
        for (int i = 0; i < 5; ++i) {
            const double spread = (i - 2) * 0.10;
            AddShot(pEnemyShotSet, cx, cy, aim + spread,
                3.0 + 0.15 * ((i + 1) % 2), img_enemyShotBullet[4]);
        }
    }

    if (t > 0 && t % 60 == 30) {
        const double aim = atan2(player.y - cy, player.x - cx);
        for (int i = 0; i < 9; ++i) {
            const double a = aim + ((t / 60) & 1 ? -0.42 : 0.42)
                + (i - 4) * 0.085;
            AddShot(pEnemyShotSet, cx, cy, a, 2.55, img_enemyShotBullet[4]);
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  属性リング
//  param_i[0] = 0:赤 / 1:青
// ============================================================
static void ShotAttributeRing(sEnemyShotSet* pEnemyShotSet)
{
    const int color = pEnemyShotSet->param_i[0];
    const int ringCount = 56;
    const double ringRadius = 58.0;

    // リングは常に自機を中心に固定する。
    int ringIndex = 0;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double a = DX_TWO_PI * ringIndex / (double)ringCount;
        pShot->x = player.x + ringRadius * cos(a);
        pShot->y = player.y + ringRadius * sin(a);
        pShot->muki = a;
        pShot->speed = 0.0;
        pShot->kind = color == 0 ? img_enemyShotSmallBall[0]
            : img_enemyShotSmallBall[4];
        ringIndex++;
        pShot = pShot->next;
    }

    // 現在の属性と同じ色の敵弾だけをリング接触で消す。
    // リング自身の小玉や異色弾は対象外にする。
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        sEnemyShotSet* pSetNext = pSet->next;

        if (pSet != pEnemyShotSet && pSet->kind != 100) {
            sEnemyShot* pBullet = pSet->pEnemyShotHead->next;
            while (pBullet != pSet->pEnemyShotHead) {
                sEnemyShot* pNext = pBullet->next;

                const bool matchingColor = color == 0
                    ? (pBullet->kind == img_enemyShotMediumBall[0] ||
                        pBullet->kind == img_enemyShotBullet[0])
                    : (pBullet->kind == img_enemyShotMediumBall[4] ||
                        pBullet->kind == img_enemyShotBullet[4]);

                if (matchingColor) {
                    bool hit = false;
                    for (int i = 0; i < ringCount; ++i) {
                        const double a = DX_TWO_PI * i / (double)ringCount;
                        const double rx = player.x + ringRadius * cos(a);
                        const double ry = player.y + ringRadius * sin(a);
                        const double dx = pBullet->x - rx;
                        const double dy = pBullet->y - ry;

                        // 小玉との接触を少し太めに判定して、色弾が輪を抜けにくくする。
                        if (dx * dx + dy * dy <= 11.0 * 11.0) {
                            hit = true;
                            break;
                        }
                    }

                    if (hit) {
                        pBullet->prev->next = pBullet->next;
                        pBullet->next->prev = pBullet->prev;
                        delete pBullet;
                    }
                }

                pBullet = pNext;
            }
        }

        pSet = pSetNext;
    }
}

// ============================================================
//  敵本体
// ============================================================
void EnemyPat_Ikaruga_ChatGPT()
{
    static sEnemyShotSet* pRedSet;
    static sEnemyShotSet* pBlueSet;
    static sEnemyShotSet* pRingSet;
    static int initialized;

    if (count == 1) {
        enemy.x = 125.0;
        enemy.y = 58.0;
        enemy.x2 = 355.0;
        enemy.y2 = 58.0;
        enemy.maxHp = enemy.hp = 200;

        pRedSet = nullptr;
        pBlueSet = nullptr;
        pRingSet = nullptr;
        initialized = 0;
    }

    if (!initialized) {
        initialized = 1;

        // 0秒後：チャージ予告
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 赤ボス
        pRedSet = new sEnemyShotSet;
        pRedSet->count = 0;
        pRedSet->patternFunc = ShotRedBoss;
        pRedSet->x = enemy.x;
        pRedSet->y = enemy.y + 10.0;
        pRedSet->kind = 101;
        pRedSet->param_d[0] = 0.35;
        pRedSet->pEnemyShotHead = new sEnemyShot;
        pRedSet->pEnemyShotHead->prev = pRedSet->pEnemyShotHead;
        pRedSet->pEnemyShotHead->next = pRedSet->pEnemyShotHead;
        pRedSet->prev = enemyShotSetHead.prev;
        pRedSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pRedSet;
        enemyShotSetHead.prev = pRedSet;

        // 青ボス
        pBlueSet = new sEnemyShotSet;
        pBlueSet->count = 0;
        pBlueSet->patternFunc = ShotBlueBoss;
        pBlueSet->x = enemy.x2;
        pBlueSet->y = enemy.y2 + 10.0;
        pBlueSet->kind = 102;
        pBlueSet->param_d[0] = 1.25;
        pBlueSet->pEnemyShotHead = new sEnemyShot;
        pBlueSet->pEnemyShotHead->prev = pBlueSet->pEnemyShotHead;
        pBlueSet->pEnemyShotHead->next = pBlueSet->pEnemyShotHead;
        pBlueSet->prev = enemyShotSetHead.prev;
        pBlueSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBlueSet;
        enemyShotSetHead.prev = pBlueSet;
    }

    // 1秒後：赤リングを展開
    if (count == 61 && pRingSet == nullptr) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        pRingSet = new sEnemyShotSet;
        pRingSet->count = 0;
        pRingSet->patternFunc = ShotAttributeRing;
        pRingSet->kind = 100; // リング自身を通常弾の色消去対象から除外
        pRingSet->param_i[0] = 0; // 赤
        pRingSet->pEnemyShotHead = new sEnemyShot;
        pRingSet->pEnemyShotHead->prev = pRingSet->pEnemyShotHead;
        pRingSet->pEnemyShotHead->next = pRingSet->pEnemyShotHead;

        for (int i = 0; i < 56; ++i) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            const double a = DX_TWO_PI * i / 56.0;
            pEnemyShot->x = player.x + 58.0 * cos(a);
            pEnemyShot->y = player.y + 58.0 * sin(a);
            pEnemyShot->muki = a;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotSmallBall[0];
            pEnemyShot->margin = 999.0;
            pEnemyShot->prev = pRingSet->pEnemyShotHead->prev;
            pEnemyShot->next = pRingSet->pEnemyShotHead;
            pRingSet->pEnemyShotHead->prev->next = pEnemyShot;
            pRingSet->pEnemyShotHead->prev = pEnemyShot;
        }

        pRingSet->x = player.x;
        pRingSet->y = player.y;
        pRingSet->prev = enemyShotSetHead.prev;
        pRingSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pRingSet;
        enemyShotSetHead.prev = pRingSet;
    }

    // 3秒後：次の切り替えを予告
    if (count >= 181 && (count - 181) % 180 == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 4秒後、その後3秒ごと：リングの属性を反転
    if (count >= 241 && (count - 241) % 180 == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        if (pRingSet) {
            pRingSet->param_i[0] ^= 1;
        }
    }

    // 二体のボスを少し上下左右に動かし、弾列の発射位置を変化させる。
    enemy.x = 125.0 + 32.0 * sin(0.011 * count);
    enemy.y = 58.0 + 14.0 * sin(0.017 * count);
    enemy.x2 = 355.0 + 32.0 * sin(0.011 * count + DX_PI);
    enemy.y2 = 58.0 + 14.0 * sin(0.017 * count + DX_PI);

    if (pRedSet) {
        pRedSet->x = enemy.x;
        pRedSet->y = enemy.y + 10.0;
    }
    if (pBlueSet) {
        pBlueSet->x = enemy.x2;
        pBlueSet->y = enemy.y2 + 10.0;
    }
    if (pRingSet) {
        pRingSet->x = player.x;
        pRingSet->y = player.y;
    }

    // count, pEnemyShotSet->count, pEnemyShot->count の更新や
    // 画面外弾の消去は既存のメインルーチン側で行う前提。
}
