// enemyPat_Tmp.cpp

// 弾を生成してリストに登録するヘルパー関数
static void AddPopupShot(sEnemyShotSet* pSet, double dx, double dy, int kind, int type, double muki = 0.0)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = pSet->x + dx;
    pShot->y = pSet->y + dy;
    pShot->muki = muki;
    pShot->speed = 0.0;
    pShot->kind = kind;

    // 中心からのオフセット位置を記憶（拡大ギミック用）
    pShot->param_d[0] = dx;
    pShot->param_d[1] = dy;

    // 役割タイプ(0:通常構成弾, 1:ボタン本体, 2:ボタン装飾)
    pShot->param_i[0] = type;

    if (type == 1) {
        // ボタン本体（赤大玉）の場合は耐久力(HP)を設定。ショット5発分。
        pShot->param_i[1] = 3;
    }

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 弾幕：ポップアップ広告
static void PopupAdPattern(sEnemyShotSet* pEnemyShotSet)
{
    // param_i[0] の状態:
    // 0 = 展開中（待機・拡大中）
    // 1 = [X]ボタン破壊済（広告を閉じる）
    // 2 = ペナルティ状態（タイムオーバーでバラマキ）

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double w = 180.0; // ウィンドウ幅
        double h = 140.0; // ウィンドウ高さ

        // 枠の生成（青の小玉）
        for (double x = -w / 2; x <= w / 2; x += 15.0) {
            AddPopupShot(pEnemyShotSet, x, -h / 2, img_enemyShotSmallBall[4], 0);
            AddPopupShot(pEnemyShotSet, x, h / 2, img_enemyShotSmallBall[4], 0);
        }
        for (double y = -h / 2 + 15.0; y <= h / 2 - 15.0; y += 15.0) {
            AddPopupShot(pEnemyShotSet, -w / 2, y, img_enemyShotSmallBall[4], 0);
            AddPopupShot(pEnemyShotSet, w / 2, y, img_enemyShotSmallBall[4], 0);
        }

        // 中身の生成（黄の中玉）
        for (double x = -w / 2 + 20.0; x <= w / 2 - 20.0; x += 20.0) {
            for (double y = -h / 2 + 20.0; y <= h / 2 - 20.0; y += 20.0) {
                // 右上の[X]ボタン付近は被らないように空けておく
                if (x > w / 2 - 40.0 && y < -h / 2 + 40.0) continue;

                AddPopupShot(pEnemyShotSet, x, y, img_enemyShotMediumBall[1], 0);
            }
        }

        // [X]ボタンの生成（右上に配置）
        double btnX = w / 2;
        double btnY = -h / 2;
        // 本体（当たり判定を持つ赤大玉）
        AddPopupShot(pEnemyShotSet, btnX, btnY, img_enemyShotLargeBall[0], 1);
        // 装飾のバツ印（赤の短レーザーを斜めに交差）
        AddPopupShot(pEnemyShotSet, btnX, btnY, img_enemyShotLaser[0], 2, DX_PI / 4.0);
        AddPopupShot(pEnemyShotSet, btnX, btnY, img_enemyShotLaser[0], 2, -DX_PI / 4.0);
    }

    // --- 状態ごとの処理 ---

    // 状態0: 展開中（[X]ボタン判定とウィンドウ拡大）
    if (pEnemyShotSet->param_i[0] == 0) {

        // 1. ボタンと自機ショットの当たり判定（簡易実装）
        sEnemyShot* pBtn = nullptr;
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) { // ボタン本体を探す
                pBtn = pShot;
                break;
            }
            pShot = pShot->next;
        }

        if (pBtn) {
            sPlayerShot* pPShot = playerShotHead.next;
            while (pPShot != &playerShotHead) {
                double dx = pPShot->x - pBtn->x;
                double dy = pPShot->y - pBtn->y;

                // 距離判定（大玉半径約20 + 自機ショット半径約5 = 25）
                if (dx * dx + dy * dy < 25.0 * 25.0) {
                    pBtn->param_i[1]--; // ボタンのHPを減らす
                    pBtn->kind = img_enemyShotLargeBall[3 - pBtn->param_i[1]];

                    // 当たった自機ショットを削除（リストから外してdelete）
                    sPlayerShot* delShot = pPShot;
                    pPShot = pPShot->next;
                    delShot->prev->next = delShot->next;
                    delShot->next->prev = delShot->prev;
                    delete delShot;

                    // ボタン破壊完了
                    if (pBtn->param_i[1] <= 0) {
                        pEnemyShotSet->param_i[0] = 1; // 破壊フラグON
                        break;
                    }
                }
                else {
                    pPShot = pPShot->next;
                }
            }
        }

        // 2. ウィンドウの圧迫（ジワジワ拡大する）
        double scale = 1.0 + pEnemyShotSet->count * 0.003;
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x = pEnemyShotSet->x + pShot->param_d[0] * scale;
            pShot->y = pEnemyShotSet->y + pShot->param_d[1] * scale;
            pShot = pShot->next;
        }

        // 3. ペナルティ判定（約2.5秒 = 150カウントで時間切れ）
        if (pEnemyShotSet->count >= 150) {
            pEnemyShotSet->param_i[0] = 2; // ペナルティ状態へ移行

            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                // 自機方向に向かって拡散する弾に変化
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x) + (GetRand(60) - 30) / 180.0 * DX_PI;
                pShot->speed = (150 + GetRand(250)) / 100.0;
                pShot = pShot->next;
            }
        }
    }

    // 状態1: [X]ボタン破壊済（全弾消去して広告を閉じる）
    else if (pEnemyShotSet->param_i[0] == 1) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* del = pShot;
            pShot = pShot->next;
            del->prev->next = del->next;
            del->next->prev = del->prev;
            delete del; // プールアロケータに返却される
        }
        pEnemyShotSet->alive = 0;
        return; // 全弾消去したためここで処理終了
    }

    // 状態2: ペナルティ発動中（バラマキ移動）
    else if (pEnemyShotSet->param_i[0] == 2) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン（メイン呼出用）
void EnemyPat_PopUpAds_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右にゆらゆら移動
        enemy.x += 1.0 * (double)muki;
        if (enemy.x > 380.0) muki = -1;
        if (enemy.x < 100.0) muki = 1;
    }

    // 200カウント周期で、短い間隔で連続してポップアップ広告を発生させる（ウザさの演出）
    int c = count % 200;
    if (c == 20 || c == 50 || c == 80) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = PopupAdPattern;

        // 広告の出現位置（画面内に収まりつつ、自機を直接潰さないよう適度にランダム）
        pEnemyShotSet->x = 120.0 + GetRand(240);
        pEnemyShotSet->y = 100.0 + GetRand(200);

        pEnemyShotSet->param_i[0] = 0; // 初期状態

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}