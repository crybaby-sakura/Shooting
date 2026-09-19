// ============================================================
//  弾幕：竹やぶ焼けた
//  enemyPat_takeyabu.cpp
//  既存素材のみで構成：
//    img_enemyShotSmallBall  （竹・葉・火の粉・灰）
//    img_enemyShotMediumBall （竹の節・延焼の中玉）
//    img_enemyShotLargeBall  （火種）
//    img_enemyShotLaser      （倒壊する竹）
//  色: 2=緑(竹), 1=黄(葉), 8=橙(火), 6=白(灰)
// ============================================================

// ============================================================
//  ヘルパー
// ============================================================

// 弾を1つセットに追加（末尾＝head の直前）
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// 弾を1つ削除
static void RemoveShot(sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev->next = pEnemyShot->next;
    pEnemyShot->next->prev = pEnemyShot->prev;
    delete pEnemyShot;
}

// 新しい弾幕セットを生成してリストに登録
static sEnemyShotSet* NewSet(sEnemyShotSet::PatternFunc func,
    double x, double y, double muki)
{
    sEnemyShotSet* s = new sEnemyShotSet;
    s->x = x;
    s->y = y;
    s->muki = muki;
    s->count = 0;
    s->kind = 0;
    s->patternFunc = func;

    s->pEnemyShotHead = new sEnemyShot;
    s->pEnemyShotHead->prev = s->pEnemyShotHead;
    s->pEnemyShotHead->next = s->pEnemyShotHead;

    s->prev = enemyShotSetHead.prev;
    s->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = s;
    enemyShotSetHead.prev = s;
    return s;
}

// 全弾を等速直線移動（個々の muki / speed を使用）
static void MoveShots(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 音を重ねずに鳴らす小ヘルパー
static void PlaySE(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ============================================================
//  拍1「た」：竹立て
//   縦に緑の小玉が降下。5個ごとに中玉（節）を混ぜる。
// ============================================================
static void ShotBambooColumn(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;

    // 10フレームごとに竹の節を追加（最大20節）
    if (t % 10 == 0 && t / 10 < 20) {
        int idx = t / 10;
        int isJoint = (idx % 5 == 4);

        sEnemyShot* p = AddShot(pEnemyShotSet);
        p->x = pEnemyShotSet->x;
        p->y = pEnemyShotSet->y;
        p->muki = DX_PI / 2;   // 下向き
        p->speed = 1.8;
        p->kind = isJoint
            ? img_enemyShotMediumBall[2]  // 緑・節
            : img_enemyShotSmallBall[2];  // 緑・竹
        p->param_i[0] = isJoint ? 1 : 0; // 1=節
        p->param_i[1] = 0;               // 分裂済みフラグ
    }

    // 移動と節割れ
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 拍2「け」：節が画面中段に来たら4方向に分裂
        if (pShot->param_i[0] == 1 && pShot->param_i[1] == 0
            && pShot->y > 160.0)
        {
            pShot->param_i[1] = 1;
            for (int i = 0; i < 4; i++) {
                sEnemyShot* sp = AddShot(pEnemyShotSet);
                sp->x = pShot->x;
                sp->y = pShot->y;
                sp->muki = i * DX_PI / 2 + DX_PI / 4;
                sp->speed = 2.5;
                sp->kind = img_enemyShotSmallBall[2];
            }
            PlaySE(sound_enemyShot_light);
        }
        pShot = next;
    }
}

// ============================================================
//  拍3「や」：竹揺れ
//   サイン波でXを振りながら竹を伸ばし、自機狙い3way（葉）を追加。
// ============================================================
static void ShotSwaying(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;
    int col = pEnemyShotSet->param_i[0];

    // 揺れる竹
    if (t % 8 == 0 && t / 8 < 25) {
        double sx = pEnemyShotSet->x + 50.0 * sin(t * 0.06 + col * 1.5);
        sEnemyShot* p = AddShot(pEnemyShotSet);
        p->x = sx;
        p->y = pEnemyShotSet->y;
        p->muki = DX_PI / 2;
        p->speed = 1.5;
        p->kind = img_enemyShotSmallBall[2];
    }

    // 30フレームごとに自機狙い3way（葉っぱ）
    if (t > 0 && t % 30 == 0) {
        double base = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        for (int i = -1; i <= 1; i++) {
            sEnemyShot* p = AddShot(pEnemyShotSet);
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = base + i * 0.18;
            p->speed = 3.0;
            p->kind = img_enemyShotSmallBall[1]; // 黄（葉）
        }
    }

    MoveShots(pEnemyShotSet);
}

// ============================================================
//  拍4「ぶ」＆拍5「や」：発火 → 延焼
//   大玉の火種が降下し火の粉を撒く。中段で停止して
//   8方向に中玉を放射、各中玉は20F後に4分裂。
// ============================================================
static void ShotIgnition(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;

    // 最初に火種を1つ生成
    if (t == 0) {
        sEnemyShot* p = AddShot(pEnemyShotSet);
        p->x = pEnemyShotSet->x;
        p->y = pEnemyShotSet->y;
        p->muki = DX_PI / 2;
        p->speed = 1.5;
        p->kind = img_enemyShotLargeBall[8]; // 橙
        p->param_i[0] = 0; // 0=火種降下中
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;

        if (pShot->param_i[0] == 0) {
            // --- 火種：降下 ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 火の粉を撒く
            if (pShot->count % 5 == 0) {
                sEnemyShot* sp = AddShot(pEnemyShotSet);
                sp->x = pShot->x + (GetRand(10) - 5);
                sp->y = pShot->y;
                sp->muki = DX_PI / 2 + (GetRand(30) - 15) / 180.0 * DX_PI;
                sp->speed = 0.8 + GetRand(60) / 100.0;
                sp->kind = img_enemyShotSmallBall[8];
                sp->param_i[0] = 3; // 3=火の粉
            }

            // 中段で停止 → 延焼（8方向に中玉放射）
            if (pShot->y > 200.0) {
                for (int i = 0; i < 8; i++) {
                    sEnemyShot* sp = AddShot(pEnemyShotSet);
                    sp->x = pShot->x;
                    sp->y = pShot->y;
                    sp->muki = i * DX_PI / 4;
                    sp->speed = 1.5;
                    sp->kind = img_enemyShotMediumBall[8];
                    sp->param_i[0] = 2; // 2=延焼中玉
                    sp->param_i[1] = 0;
                }
                PlaySE(sound_enemyShot_medium);
                RemoveShot(pShot);
            }
        }
        else if (pShot->param_i[0] == 2) {
            // --- 延焼中玉：移動 → 20F後に4分裂 ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            if (pShot->count > 20 && pShot->param_i[1] == 0) {
                pShot->param_i[1] = 1;
                for (int i = 0; i < 4; i++) {
                    sEnemyShot* sp = AddShot(pEnemyShotSet);
                    sp->x = pShot->x;
                    sp->y = pShot->y;
                    sp->muki = i * DX_PI / 2 + DX_PI / 4;
                    sp->speed = 2.0;
                    sp->kind = img_enemyShotSmallBall[8];
                    sp->param_i[0] = 3;
                }
            }
        }
        else {
            // --- 火の粉：そのまま移動 ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = next;
    }
}

// ============================================================
//  拍6「け」：倒壊
//   各竹列の位置から短レーザーを縦向きに高速降下。
// ============================================================
static void ShotCollapse(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;

    // 6Fごとに5列を順番に発射
    if (t % 6 == 0 && t < 60) {
        int col = (t / 6) % 5;
        double x = 80.0 + col * 80.0;

        sEnemyShot* p = AddShot(pEnemyShotSet);
        p->x = x;
        p->y = 10.0;
        p->muki = DX_PI / 2;        // 下向き
        p->speed = 7.0;
        p->kind = img_enemyShotLaser[8]; // 橙・短レーザー
    }

    MoveShots(pEnemyShotSet);
}

// ============================================================
//  拍7「た」：灰
//   低速の白小弾（灰）が画面全体に舞い、最後に大爆発。
// ============================================================
static void ShotAsh(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;

    // 灰を上から撒く
    if (t < 180) {
        for (int i = 0; i < 2-1; i++) {
            sEnemyShot* p = AddShot(pEnemyShotSet);
            p->x = (double)GetRand(480);
            p->y = -10.0;
            p->muki = DX_PI / 2 + (GetRand(40) - 20) / 180.0 * DX_PI;
            p->speed = 0.5 + GetRand(80) / 100.0;
            p->kind = img_enemyShotSmallBall[6]; // 白（灰）
        }
    }

    // 最後の爆発
    if (t == 200) {
        PlaySE(sound_enemyShot_heavy);
        for (int i = 0; i < 24; i++) {
            sEnemyShot* p = AddShot(pEnemyShotSet);
            p->x = 240.0;
            p->y = 240.0;
            p->muki = i * DX_PI / 12;
            p->speed = 2.0;
            p->kind = img_enemyShotMediumBall[8];
        }
    }

    MoveShots(pEnemyShotSet);
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_TakeyabuYaketa_DeepSeek()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x = 240.0 + 80.0 * sin(count * 0.015);
    }

    int t = (count - 1) % 1200;

    // 拍1「た」：竹立て（5列）
    if (t == 0) {
        PlaySE(sound_enemyShot_medium);
        for (int c = 0; c < 5; c++) {
            sEnemyShotSet* s =
                NewSet(ShotBambooColumn, 80.0 + c * 80.0, 10.0, 0.0);
            s->param_i[0] = c;
        }
    }

    // 拍3「や」：竹揺れ（3列）
    if (t == 180) {
        PlaySE(sound_enemyShot_light);
        for (int c = 0; c < 3; c++) {
            sEnemyShotSet* s =
                NewSet(ShotSwaying, 120.0 + c * 120.0, 10.0, 0.0);
            s->param_i[0] = c;
        }
    }

    // 拍4「ぶ」：発火（5列）
    if (t == 270) {
        PlaySE(sound_enemyShot_heavy);
        for (int c = 0; c < 5; c++) {
            NewSet(ShotIgnition, 80.0 + c * 80.0, 20.0, 0.0);
        }
    }

    // 拍6「け」：倒壊
    if (t == 450) {
        PlaySE(sound_enemyShot_extreme);
        NewSet(ShotCollapse, 0.0, 0.0, 0.0);
    }

    // 拍7「た」：灰
    if (t == 540) {
        PlaySE(sound_enemyCharge);
        NewSet(ShotAsh, 0.0, 0.0, 0.0);
    }
}