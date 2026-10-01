// enemyPat_Tmp.cpp
// 弾幕「風船割り・祝砲」
// ・風船(大型弾)は無害でふわふわ自機へ接近してくる
// ・風船に自機ショットを当てると割れて、色に応じた子弾がばら撒かれる
//   赤: 16方向の円形弾 / 青: 自機狙い5Way高速弾 / 黄: 低速大型弾の残留
// ※ count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//    画面外の弾の消去はメインルーチンで行われる前提。

// --------------------------------------------------
// 弾をセットのリンクトリストに追加するヘルパー
// --------------------------------------------------
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int img)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = img;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// --------------------------------------------------
// 風船が割れたときの処理(子弾のばら撒き+風船の削除)
// --------------------------------------------------
static void BalloonBurst(sEnemyShotSet* pSet, sEnemyShot* pBalloon)
{
    int color = pBalloon->param_i[1];
    double bx = pBalloon->x;
    double by = pBalloon->y;

    // 割れる音(色ごとに使い分け)
    // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy,
    //                   sound_enemyShot_extreme, sound_enemyCharge(予告音)
    int sound = sound_enemyShot_light;
    switch (color) {
    case 0: sound = sound_enemyShot_heavy;  break; // 赤
    case 4: sound = sound_enemyShot_medium; break; // 青
    case 1: sound = sound_enemyShot_light;  break; // 黄
    }
    if (CheckSoundMem(sound)) StopSoundMem(sound);
    PlaySoundMem(sound, DX_PLAYTYPE_BACK);

    switch (color) {
    case 0:
        // 赤風船: 全16方向に円形弾 → 割る位置を誤ると自機が巻き込まれる
        for (int i = 0; i < 16*3; i++) {
            AddShot(pSet, bx, by, i * DX_PI * 2.0 / 16.0/3, 2.2, img_enemyShotSmallBall[0]);
        }
        break;

    case 4:
        // 青風船: 自機方向へ扇状5Wayの高速弾 → 遠くで割るほど避けやすい
    {
        double aim = atan2(player.y - by, player.x - bx);
        for (int i = -2-5; i <= 2+5; i++) {
            AddShot(pSet, bx, by, aim + i * 0.12, 4.5, img_enemyShotSmallBall[4]);
        }
    }
    break;

    case 1:
        // 黄風船: 低速の大型弾(破片)を残留させ、回避スペースを狭める
        for (int i = 0; i < 4; i++) {
            // GetRand(3) は 0〜3 の4通りを返すので注意
            double m = i * DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
            AddShot(pSet, bx, by, m, 0.6, img_enemyShotLargeBall[1]);
        }
        // ついでに小弾も少し撒く
        for (int i = 0; i < 4*3; i++) {
            AddShot(pSet, bx, by, (GetRand(360) - 180) / 180.0 * DX_PI, 1.8, img_enemyShotSmallBall[1]);
        }
        break;
    }

    // 風船本体をリストから外して削除
    // ※子弾はリスト末尾に追加済み。呼び出し元では削除前に next を保存してあるので安全
    pBalloon->prev->next = pBalloon->next;
    pBalloon->next->prev = pBalloon->prev;
    delete pBalloon;
}

// --------------------------------------------------
// 風船1個(と、それから生まれた子弾)を管理するパターン関数
// --------------------------------------------------
static void BalloonPat(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 風船を1個生成
        // param_i[0]: 1=風船フラグ / param_i[1]: 色(0:赤 1:黄 4:青)
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 1.2;
        pEnemyShot->param_i[0] = 1;
        pEnemyShot->param_i[1] = pEnemyShotSet->kind;
        pEnemyShot->kind = img_enemyShotLargeBall[pEnemyShotSet->kind];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // ループ中に削除される場合に備えて保存

        if (pShot->param_i[0] == 1) {
            // ----- 風船の挙動 -----

            // 自機ショットとの当たり判定(自機ショットが当たったら割れる)
            bool hit = false;
            sPlayerShot* pPS = playerShotHead.next;
            while (pPS != &playerShotHead) {
                double dx = pPS->x - pShot->x;
                double dy = pPS->y - pShot->y;
                if (dx * dx + dy * dy < 18.0 * 18.0) { // 風船(半径18)にショットが接触
                    hit = true;
                    break;
                }
                pPS = pPS->next;
            }

            if (hit) {
                BalloonBurst(pEnemyShotSet, pShot);
                pShot = pNext;
                continue;
            }

            // ふわふわ移動: ゆるく自機を追尾し、横に揺れる
            double aim = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = aim - pShot->muki;
            while (diff > DX_PI)  diff -= DX_PI * 2.0;
            while (diff < -DX_PI) diff += DX_PI * 2.0;
            if (pShot->count < 300) pShot->muki += diff * 0.015; // 序盤だけ追尾

            pShot->x += pShot->speed * cos(pShot->muki) + sin(pShot->count * 0.07) * 1.0;
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // ----- 子弾(割れた風船から出た弾)は等速直進 -----
            // ※運動はここで行うが、消去はメインルーチンに任せる
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// --------------------------------------------------
// 敵本体のパターン
// --------------------------------------------------
void EnemyPat_Balloon_Zai()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (enemy.x < 60.0 || enemy.x > 420.0) muki *= -1;
    }

    // 一定間隔で風船を放出
    if (count % 40 == 30) {
        // 予告音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = BalloonPat;

        // 放出位置は敵の周辺にランダム
        // GetRand(120) は 0〜120 の121通りなので、-60〜+60 にする
        pEnemyShotSet->x = enemy.x + GetRand(120) - 60;
        pEnemyShotSet->y = enemy.y + 10.0;

        // 初期方向は自機方向+ランダム偏差
        // GetRand(60) は 0〜60 の61通りなので、-30〜+30 にする
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x)
            + (GetRand(60) - 30) / 180.0 * DX_PI;

        // 色の抽選(0〜3 の4通り): 赤50% / 青25% / 黄25%
        int r = GetRand(3);
        int color;
        if (r <= 1)      color = 0; // 赤
        else if (r == 2) color = 4; // 青
        else             color = 1; // 黄
        pEnemyShotSet->kind = color;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}