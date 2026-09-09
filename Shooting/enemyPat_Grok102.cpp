// enemyPat_Tmp.cpp
// ポップアップ広告モチーフ弾幕「強制重ね表示ウィンドウ」

// 矩形枠・内部埋め・偽×ボタンを生成するポップアップウィンドウ用パターン
// param_i[0] : 発動済みフラグ (0=未発動, 1=増殖済み)
// param_i[1] : 寿命カウンタ用（参考）
// param_d[0] : ウィンドウ中心X
// param_d[1] : ウィンドウ中心Y
// param_d[2] : 半幅
// param_d[3] : 半高
// param_d[4] : ×ボタン中心X
// param_d[5] : ×ボタン中心Y
static void ShotPopupWindow(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ生成
    if (pEnemyShotSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // ウィンドウサイズ（ランダムでバリエーション）
        double halfW = 55.0 + GetRand(25);   // 半幅 55〜80
        double halfH = 40.0 + GetRand(20);   // 半高 40〜60
        double cx = pEnemyShotSet->x;
        double cy = pEnemyShotSet->y;

        // パラメータ保存
        pEnemyShotSet->param_d[0] = cx;
        pEnemyShotSet->param_d[1] = cy;
        pEnemyShotSet->param_d[2] = halfW;
        pEnemyShotSet->param_d[3] = halfH;
        pEnemyShotSet->param_d[4] = cx + halfW - 12.0;  // ×位置（右上寄り）
        pEnemyShotSet->param_d[5] = cy - halfH + 12.0;
        pEnemyShotSet->param_i[0] = 0;  // 未増殖

        // ---------- 1. 枠（菱形弾で矩形を描く） ----------
        // 上辺・下辺
        int borderNumX = (int)(halfW * 2 / 7.0) + 1;
        for (int i = 0; i <= borderNumX; i++) {
            double t = (double)i / borderNumX;
            double bx = cx - halfW + t * halfW * 2.0;

            // 上
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = bx;
            pEnemyShot->y = cy - halfH;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotDiamond[6]; // 白
            pEnemyShot->param_i[0] = 1; // 枠フラグ
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 下
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = bx;
            pEnemyShot->y = cy + halfH;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotDiamond[6]; // 白
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        // 左辺・右辺
        int borderNumY = (int)(halfH * 2 / 7.0) + 1;
        for (int i = 1; i < borderNumY; i++) { // 角は既に上で置いたので内側のみ
            double t = (double)i / borderNumY;
            double by = cy - halfH + t * halfH * 2.0;

            // 左
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx - halfW;
            pEnemyShot->y = by;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotDiamond[6]; // 白
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 右
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + halfW;
            pEnemyShot->y = by;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotDiamond[6]; // 白
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ---------- 2. 内部埋め（小玉を格子状に） ----------
        int fillNumX = (int)(halfW * 2 / 14.0);
        int fillNumY = (int)(halfH * 2 / 14.0);
        if (fillNumX < 2) fillNumX = 2;
        if (fillNumY < 2) fillNumY = 2;
        for (int iy = 1; iy < fillNumY; iy++) {
            for (int ix = 1; ix < fillNumX; ix++) {
                double fx = cx - halfW + (halfW * 2.0) * ix / fillNumX;
                double fy = cy - halfH + (halfH * 2.0) * iy / fillNumY;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = fx;
                pEnemyShot->y = fy;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                // 色を少しバラけさせて広告感
                int col = (ix + iy) % 3;
                if (col == 0) pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤
                else if (col == 1) pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙
                else pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄
                pEnemyShot->param_i[0] = 2; // 埋めフラグ
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // ---------- 3. 偽の閉じるボタン「×」（短レーザー2本を交差） ----------
        double xx = pEnemyShotSet->param_d[4];
        double xy = pEnemyShotSet->param_d[5];

        // ／ 方向
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = xx;
        pEnemyShot->y = xy;
        pEnemyShot->muki = DX_PI / 4.0;          // 45度
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotLaser[5]; // マゼンタ
        pEnemyShot->param_i[0] = 3; // ×フラグ
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // ＼ 方向
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = xx;
        pEnemyShot->y = xy;
        pEnemyShot->muki = -DX_PI / 4.0;         // -45度
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotLaser[5]; // マゼンタ
        pEnemyShot->param_i[0] = 3;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // ---------- 毎フレーム処理 ----------
    // プレイヤーが×に近づいたら増殖（1回のみ）
    if (pEnemyShotSet->param_i[0] == 0) {
        double dx = player.x - pEnemyShotSet->param_d[4];
        double dy = player.y - pEnemyShotSet->param_d[5];
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < 38.0) {
            pEnemyShotSet->param_i[0] = 1; // 発動済み

            // 効果音
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 近くに2〜3個の新しいウィンドウを生成
            int spawnNum = 2 + GetRand(1); // 2 or 3
            for (int s = 0; s < spawnNum; s++) {
                sEnemyShotSet* pNew = new sEnemyShotSet;
                pNew->count = 0;
                pNew->patternFunc = ShotPopupWindow;
                // 元ウィンドウ周辺にランダム配置（画面内に収める）
                double nx = pEnemyShotSet->param_d[0] + (GetRand(160) - 80);
                double ny = pEnemyShotSet->param_d[1] + (GetRand(120) - 60);
                if (nx < 70.0) nx = 70.0;
                if (nx > 410.0) nx = 410.0;
                if (ny < 60.0) ny = 60.0;
                if (ny > 320.0) ny = 320.0;
                pNew->x = nx;
                pNew->y = ny;
                pNew->muki = 0.0;
                pNew->kind = 0;
                pNew->pEnemyShotHead = new sEnemyShot;
                pNew->pEnemyShotHead->prev = pNew->pEnemyShotHead;
                pNew->pEnemyShotHead->next = pNew->pEnemyShotHead;
                pNew->prev = enemyShotSetHead.prev;
                pNew->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pNew;
                enemyShotSetHead.prev = pNew;
            }
        }
    }

    // 弾の移動・寿命処理
    // 一定時間後に外側へ飛び散らせてクリア
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 寿命が来たら外側へ加速
        if (pEnemyShotSet->count >= 210) {
            if (pShot->speed < 0.1) {
                // 中心からの方向を計算して飛ばす
                double dx = pShot->x - pEnemyShotSet->param_d[0];
                double dy = pShot->y - pEnemyShotSet->param_d[1];
                double len = sqrt(dx * dx + dy * dy);
                if (len < 1.0) len = 1.0;
                pShot->muki = atan2(dy, dx);
                pShot->speed = 1.5 + (len / 80.0); // 外側ほど速く
            }
        }
        // 通常移動（speed==0の間は静止）
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 微振動を加えて「広告がチラつく」感じ（埋め弾のみ）
        if (pShot->param_i[0] == 2 && pEnemyShotSet->count < 210) {
            pShot->x += 0.15 * sin(pEnemyShotSet->count * 0.15 + pShot->y * 0.05);
            pShot->y += 0.12 * cos(pEnemyShotSet->count * 0.13 + pShot->x * 0.04);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_PopUpAds_Grok()
{
    static int muki;
    static int popup_timer;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        popup_timer = 0;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 80.0) {
            enemy.x = 80.0;
            muki = 1;
        }
        if (enemy.x > 400.0) {
            enemy.x = 400.0;
            muki = -1;
        }
        // 時々反転
        if (count % 150 == 75) muki *= -1;
    }

    // 一定間隔でポップアップウィンドウを出現させる
    // 序盤は控えめ、中盤以降多めに
    int interval = 95-10;
    if (count > 300) interval = 70-10;
    if (count > 600) interval = 55-10;

    if (count % interval == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPopupWindow;

        // 出現位置：画面上半分〜中央寄りをランダム（プレイヤーのすぐ上は避ける）
        double px = 70.0 + GetRand(340);
        double py = 50.0 + GetRand(180);
        // プレイヤーから極端に近い位置は避ける
        double dx = px - player.x;
        double dy = py - player.y;
        if (dx * dx + dy * dy < 90.0 * 90.0) {
            py = player.y - 100.0 - GetRand(40);
            if (py < 50.0) py = 50.0;
        }
        pEnemyShotSet->x = px;
        pEnemyShotSet->y = py;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 予告音をたまに鳴らす
        if (GetRand(2) == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }
}