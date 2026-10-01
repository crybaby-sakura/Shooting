// ============================================================
// 弾幕：風船割れ「フワフワ気球隊」
// ------------------------------------------------------------
// 画面上部から風船が複数、左右にゆらゆら揺れながらゆっくり降下する。
// 風船本体はHPを持ち、自機ショットが既定回数命中すると
// その場で割れて、内側(ゴム片・低速多数)と外側(鋭い破片・高速少数)
// の2重リング弾幕をばら撒く。割れる前は自機狙いの息漏れ弾を
// ときどき漏らし、放置すると降下してくる圧力にもなる。
//
// ※以下はenemyPat_sampleForAI.cppから読み取れなかった部分についての
//   仮実装です。実際のエンジン仕様と異なる場合は適宜調整してください。
//   ・自機ショットとの当たり判定(距離判定)をこのファイル内で独自に実装。
//     共通の当たり判定処理が既にある場合はそちらに差し替えてください。
//   ・命中した自機ショットはその場で風船に吸収されて消える仕様。
//     貫通させたい場合はplayerShotHeadからの削除処理を外してください。
//   ・当たり半径は仕様コメントの「大玉(20.0x20.0)」等の数値を
//     そのまま半径とみなして使用しています(BALLOON_RADIUS等で調整可)。
//   ・pEnemyShotSetの弾リストが空になった後の解放は、
//     メインルーチン側の既存仕様に委ねています。
//   ・img_enemyShot*系/sound_enemyShot*系のextern宣言は、
//     enemyPat_sampleForAI.cppと同じ共通ヘッダから供給される前提です。
// ============================================================

static constexpr int    BALLOON_COUNT = 6;
static constexpr int    BALLOON_HP_MAX = 3;
static constexpr double BALLOON_RADIUS = 10.0;
static constexpr double PLAYER_SHOT_RADIUS = 3.0;
static constexpr double BALLOON_SPAWN_Y = -20.0;
static constexpr double BALLOON_FALL_SPEED = 0.35;

// 風船ごとの色(弾の色一覧: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙)
static const int BALLOON_COLORS[BALLOON_COUNT] = { 0, 1, 2, 3, 4, 8 };


// 割れた風船の破片弾幕(内側リング=ゴム片、外側リング=鋭い破片)
static void ShotBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int innerNum = 32;
        const int outerNum = 8;
        double baseMuki = pEnemyShotSet->muki;
        int color = pEnemyShotSet->kind;

        // 内側リング：低速・多数(ゴム片)
        for (int i = 0; i < innerNum; i++) {
            pEnemyShot = new sEnemyShot;
            double a = baseMuki + DX_PI * 2.0 * i / innerNum;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = a;
            pEnemyShot->speed = 1.5 + GetRand(150) / 100.0; // 1.50~3.00
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 外側リング：高速・少数(鋭い破片)。内側の隙間を埋めるように半歩ずらす。
        for (int i = 0; i < outerNum; i++) {
            pEnemyShot = new sEnemyShot;
            double a = baseMuki + DX_PI * 2.0 * i / outerNum + DX_PI / outerNum;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = a;
            pEnemyShot->speed = 4.5 + GetRand(150) / 100.0; // 4.50~6.00
            pEnemyShot->kind = img_enemyShotDiamond[color];

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

// 破裂弾幕(ShotBurst)を発生させる新しいShotSetを生成する
static void SpawnBalloonBurst(double x, double y, int color)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotBurst;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = GetRand(359) / 180.0 * DX_PI; // 破裂ごとにリングの初期角度をランダムにずらす
    pEnemyShotSet->kind = color;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 風船本体：ふわふわ浮遊 + 自機ショットとの当たり判定 + 被弾管理
// pEnemyShotSet->kind に風船の色番号(0~8)を、
// param_d[0]に横ゆれの位相オフセット、param_d[1]に振幅を格納する。
static void BalloonBody(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 風船本体を1つだけ生成。param_i[0]=1が「本体」の目印。
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = DX_PI / 2.0; // 下向きに漂う風船の向きとして設定
        pEnemyShot->speed = 0.0;        // 本体は下のサイン波移動で直接x,yを書き換えるため未使用
        pEnemyShot->kind = img_enemyShotLargeBall[pEnemyShotSet->kind];
        pEnemyShot->param_i[0] = 1;             // 本体フラグ
        pEnemyShot->param_i[1] = BALLOON_HP_MAX; // 残りHP

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    double baseX = pEnemyShotSet->x;
    double phase = pEnemyShotSet->param_d[0];
    double amp = pEnemyShotSet->param_d[1];
    double offsetX = amp * sin((pEnemyShotSet->count + phase) * 0.02);
    double curY = pEnemyShotSet->y + pEnemyShotSet->count * BALLOON_FALL_SPEED;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNextShot = pShot->next; // 破裂で消す可能性があるので先に保存しておく

        if (pShot->param_i[0] == 1) {
            // --- 風船本体 ---
            pShot->x = baseX + offsetX;
            pShot->y = curY;

            // 自機ショットとの当たり判定
            sPlayerShot* pPlayerShot = playerShotHead.next;
            while (pPlayerShot != &playerShotHead) {
                sPlayerShot* pNextPlayerShot = pPlayerShot->next;

                double dx = pPlayerShot->x - pShot->x;
                double dy = pPlayerShot->y - pShot->y;
                double hitDist = BALLOON_RADIUS + PLAYER_SHOT_RADIUS;

                if (dx * dx + dy * dy <= hitDist * hitDist) {
                    // 命中：自機ショットは風船に吸収されて消える
                    pPlayerShot->prev->next = pPlayerShot->next;
                    pPlayerShot->next->prev = pPlayerShot->prev;
                    delete pPlayerShot;

                    pShot->param_i[1]--;

                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }

                pPlayerShot = pNextPlayerShot;
            }

            // 息漏れ弾：割れる前の風船からときどき遅い弾を自機狙いで漏らす
            if (pEnemyShotSet->count % 90 == 45) {
                sEnemyShot* pLeak = new sEnemyShot;
                pLeak->x = pShot->x;
                pLeak->y = pShot->y;
                pLeak->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pLeak->speed = 1.2;
                pLeak->kind = img_enemyShotBullet[pEnemyShotSet->kind];
                pLeak->param_i[0] = 0; // 本体ではない通常弾

                pLeak->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pLeak->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pLeak;
                pEnemyShotSet->pEnemyShotHead->prev = pLeak;
            }

            // HPが尽きたら破裂
            if (pShot->param_i[1] <= 0) {
                SpawnBalloonBurst(pShot->x, pShot->y, pEnemyShotSet->kind);

                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        else {
            // --- 息漏れ弾など、本体以外の通常弾は直進 ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNextShot;
    }
}

// 敵本体のパターン
void EnemyPat_Balloon_Claude()
{
    if (count == 1) {
        // ゲーム画面は480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }

    if (count % 240 == 1) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < BALLOON_COUNT; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = BalloonBody;
            pEnemyShotSet->x = 40.0 + 80.0 * i;
            pEnemyShotSet->y = BALLOON_SPAWN_Y;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = BALLOON_COLORS[i];
            pEnemyShotSet->param_d[0] = i * 15.0;             // 横ゆれの位相をずらして群れ感を出す
            pEnemyShotSet->param_d[1] = 20.0 + (i % 3) * 8.0; // 横ゆれの振幅にもばらつきを持たせる

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}