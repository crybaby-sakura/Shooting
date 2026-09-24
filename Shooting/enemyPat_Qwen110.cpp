// enemyPat_Tmp.cpp

// 弾幕：輪廻の輪投げ
static void ShotRingToss(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 効果音再生
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 3層の輪の初期パラメータを sEnemyShotSet に保存
        pEnemyShotSet->param_d[0] = 40.0*2;  // 層0 初期半径
        pEnemyShotSet->param_d[1] = 80.0*2;  // 層1 初期半径
        pEnemyShotSet->param_d[2] = 120.0*2; // 層2 初期半径

        int layerCounts[3] = { 12, 18, 24 };
        double rotSpeed[3] = { 0.05, -0.03, 0.02 }; // 層ごとに回転方向と速度を変える
        int colors[3] = { 4, 2, 1 }; // 色: 4=青, 2=緑, 1=黄

        // 輪を構成する弾の生成
        for (int layer = 0; layer < 3; layer++) {
            for (int i = 0; i < layerCounts[layer]; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                double angle = (DX_PI * 2.0 / layerCounts[layer]) * i;

                pShot->param_d[0] = angle;                    // [0] 現在角度
                pShot->param_d[1] = (double)layer;            // [1] 層番号 (>=0 で輪の弾として更新対象にする)
                pShot->param_d[2] = rotSpeed[layer];          // [2] 回転速度
                pShot->param_d[3] = pEnemyShotSet->param_d[layer]; // [3] 現在半径
                pShot->param_d[4] = 0.0;                      // [4] 収縮・拡大用の位相
                pShot->margin = 240;

                // 弾の種類: 中玉(7.0x7.0)
                pShot->kind = img_enemyShotMediumBall[colors[layer]];

                // 連結リストに追加
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 隙間からの攻撃 (45フレームごとに発射)
    if (pEnemyShotSet->count % 45 == 0) {
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double baseAngle = pEnemyShotSet->muki;
            double offset = (i - 1) * (DX_PI / 6.0); // 基準角度から -30度, 0度, +30度

            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = baseAngle + offset;
            pShot->speed = 3.5;

            // 弾の種類: 鱗弾(4.0x3.0), 色: 6=白
            pShot->kind = img_enemyShotScale[6];

            // 層番号を -1 にすることで、下の「輪の更新処理」の対象外にする
            pShot->param_d[1] = -1.0;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の位置更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    double initRadius[3] = { pEnemyShotSet->param_d[0], pEnemyShotSet->param_d[1], pEnemyShotSet->param_d[2] };

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_d[1] >= 0.0) {
            // 【輪の弾の更新】
            int layer = (int)pShot->param_d[1];

            // 角度の更新 (回転)
            pShot->param_d[0] += pShot->param_d[2];

            // 半径の更新 (収縮・拡大)
            pShot->param_d[4] += 0.05; // 位相を進める
            double radiusOscillation = 20.0 * sin(pShot->param_d[4]); // ±20.0 の範囲で振動
            pShot->param_d[3] = initRadius[layer] + radiusOscillation;

            // 極座標から直交座標へ変換して位置を更新
            pShot->x = pEnemyShotSet->x + pShot->param_d[3] * cos(pShot->param_d[0]);
            pShot->y = pEnemyShotSet->y + pShot->param_d[3] * sin(pShot->param_d[0]);
        }
        else {
            // 【隙間からの攻撃弾の更新】 (直進運動)
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_RingToss_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右への往復移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 120フレームごとに輪投げパターンを発動
    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRingToss;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        // プレイヤー方向を基準角度として設定（隙間からの攻撃の基準になる）
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        // ダミーノードの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 連結リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}