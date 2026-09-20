// 弾幕：粘着剥離「ワックス・ストリップ」
static void ShotWaxStrip(sEnemyShotSet* pEnemyShotSet)
{
    // --- 第1段階：塗り拡げ（塗布） ---
    // 0～50フレームの間、10フレームごとに横一列（4個）ずつ黄色の大弾（ワックス）を射出
    if (pEnemyShotSet->count <= 50 && pEnemyShotSet->count % 10 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int row = pEnemyShotSet->count / 10;   // 0 ～ 5 行目
        int cols = 4;
        double target_y = 130.0 + row * 45.0; // 長方形グリッド状に固定する目標Y座標

        for (int col = 0; col < cols; col++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double target_x = pEnemyShotSet->x - 90.0 + col * 60.0; // 横方向に均等配置
            pEnemyShot->x = target_x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI / 2.0;               // 真下向き
            pEnemyShot->speed = 6.0;
            pEnemyShot->kind = img_enemyShotLargeBall[1]; // 黄色の大玉（ワックス）

            pEnemyShot->param_i[0] = 0;                   // 状態: 0:下降移動中, 1:固着静止, 2:超高速剥離
            pEnemyShot->param_d[0] = target_y;            // 目標Y座標

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 第2段階の前兆：チャージ音（固着の緊張感） ---
    if (pEnemyShotSet->count == 90) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --- 第3段階：ベリッ！（一括剥離＆毛の全方位散乱） ---
    if (pEnemyShotSet->count == 120) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 固着しているすべての黄色大弾に対して一括処理
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 黄色の大弾かつ静止状態（まだ剥離していない）のもの
            if (pShot->kind == img_enemyShotLargeBall[1] && pShot->param_i[0] == 1) {

                // 1. 各大弾の固定位置から黒い銃弾（毛）を自機狙い3-Wayで吹き出させる
                double base_angle = atan2(player.y - pShot->y, player.x - pShot->x);
                for (int k = -1; k <= 1; k++) {
                    sEnemyShot* pHair = new sEnemyShot;
                    pHair->x = pShot->x;
                    pHair->y = pShot->y;
                    // 角度と速度に僅かなランダム要素を混ぜる (GetRandは0〜xの整数)
                    pHair->muki = base_angle + k * (DX_PI / 12.0 * 2) + (GetRand(10) - 5) * (DX_PI / 180.0) * 1.5;
                    pHair->speed = 2.5 + GetRand(20) / 10.0; // 2.5 ～ 4.5
                    pHair->kind = img_enemyShotBullet[7];    // 黒色の銃弾（毛）

                    // 双方向リストに追加
                    pHair->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pHair->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pHair;
                    pEnemyShotSet->pEnemyShotHead->prev = pHair;
                }

                // 2. ワックス本体（黄色の大弾）を右方向へ超高速移動させて画面外へ引きちぎる
                pShot->muki = 0.0;      // 右向き
                pShot->speed = 22.0;    // 超高速
                pShot->param_i[0] = 2;  // 剥離状態に移行
            }
            pShot = pShot->next;
        }
    }

    // --- 各弾の毎フレーム更新処理 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 黄色の大弾の挙動制御
        if (pShot->kind == img_enemyShotLargeBall[1]) {
            if (pShot->param_i[0] == 0) { // 下降中
                // 目標Y座標に到達したらピタッと停止して「固着」状態へ
                if (pShot->y >= pShot->param_d[0]) {
                    pShot->y = pShot->param_d[0];
                    pShot->speed = 0.0;
                    pShot->param_i[0] = 1; // 固着
                }
            }
        }

        // 移動処理（速度が設定されている弾を動かす）
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン関数
void EnemyPat_BrazilianWax_Gemini()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200固定
        muki = 1;
    }
    else {
        // 敵本体の微小な左右移動
        enemy.x += 0.5 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 210フレーム（約3.5秒）周期で「ワックス・ストリップ」を一巡実行
    if (count % 210 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWaxStrip;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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