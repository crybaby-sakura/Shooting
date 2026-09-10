// EnemyPat_Ikaruga_Qwen.cpp
// 斑鳩のような弾幕を実装した敵パターン関数

// ============================================================
// 弾幕パターン関数 (pEnemyShotSet->patternFunc にセットされる)
// ============================================================
static void ShotPattern_Ikaruga(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next; // 削除時にnextポインタが壊れるため退避

        if (pShot->param_i[0] == 1) {
            // フィールド（弾消し玉）は自機に追従して移動
            // param_d[1], param_d[2] に自機からの相対座標を保存済み
            pShot->x = player.x + pShot->param_d[1];
            pShot->y = player.y + pShot->param_d[2];
        }
        else {
            // 通常の敵弾は設定された向きと速さで移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 弾消し判定
            // 赤い弾または青い弾の場合のみ、同色のフィールドとの衝突をチェック
            if (pShot->kind == img_enemyShotSmallBall[0] || pShot->kind == img_enemyShotSmallBall[4]) {
                sEnemyShot* pField = pEnemyShotSet->pEnemyShotHead->next;
                while (pField != pEnemyShotSet->pEnemyShotHead) {
                    // フィールドかつ、敵弾と同じ色であれば消去対象
                    if (pField->param_i[0] == 1 && pField->kind == pShot->kind) {
                        double dx = pShot->x - pField->x;
                        double dy = pShot->y - pField->y;

                        // 距離判定 (半径16.0以内で衝突とみなす)
                        if (dx * dx + dy * dy < 16.0 * 16.0) {
                            // リンクリストから外して破棄
                            pShot->prev->next = pShot->next;
                            pShot->next->prev = pShot->prev;
                            delete pShot;
                            break; // 消去済みなのでフィールド探索ループを抜ける
                        }
                    }
                    pField = pField->next;
                }
            }
        }

        // 次の弾へ (pShotがdeleteされていない場合、またはdeleteされていてもnextShotは有効)
        pShot = nextShot;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Ikaruga_Qwen()
{
    static sEnemyShotSet* pMyShotSet = nullptr;
    static int muki = 1;

    // ---------------------------------------------------------
    // 初期化処理 (0秒後)
    // ---------------------------------------------------------
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 120.0;
        enemy.y = 40.0;
        enemy.x2 = 360.0;
        enemy.y2 = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        // このパターン専用の弾セットを初期化
        pMyShotSet = new sEnemyShotSet;
        pMyShotSet->count = 0;
        pMyShotSet->patternFunc = ShotPattern_Ikaruga; // パターン関数を正しくセット

        pMyShotSet->pEnemyShotHead = new sEnemyShot;
        pMyShotSet->pEnemyShotHead->prev = pMyShotSet->pEnemyShotHead;
        pMyShotSet->pEnemyShotHead->next = pMyShotSet->pEnemyShotHead;

        pMyShotSet->prev = enemyShotSetHead.prev;
        pMyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMyShotSet;
        enemyShotSetHead.prev = pMyShotSet;

        // 0秒後: 予告音
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 敵の移動処理
        enemy.x += 0.98 * (double)muki;
        enemy.x2 += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // ---------------------------------------------------------
    // 時間経過によるサウンドとフィールド制御
    // ---------------------------------------------------------
    if (count > 1) {
        int elapsed = count - 1;
        int cycle = elapsed / 180; // 3秒(180フレーム)ごとのサイクル
        int phase = elapsed % 180;

        // 3秒おきの予告音 (0秒, 3秒, 6秒...)
        if (phase == 0) {
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        // 1秒後, 4秒後, 7秒後... のフィールド生成・色切り替え
        else if (phase == 60) {
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            // フィールドがまだ存在しないかチェック
            bool fieldExists = false;
            sEnemyShot* pCheck = pMyShotSet->pEnemyShotHead->next;
            while (pCheck != pMyShotSet->pEnemyShotHead) {
                if (pCheck->param_i[0] == 1) {
                    fieldExists = true;
                    break;
                }
                pCheck = pCheck->next;
            }

            if (!fieldExists) {
                // 初回：自機を取り囲むように赤小玉を配置
                for (int i = 0; i < 8; i++) {
                    sEnemyShot* pShot = new sEnemyShot;
                    double angle = (DX_PI * 2.0 / 8.0) * i;
                    double radius = 80.0; // 自機からの距離

                    pShot->x = player.x + cos(angle) * radius;
                    pShot->y = player.y + sin(angle) * radius;

                    pShot->margin = 999.0;           // 追加されたmarginメンバを利用
                    pShot->param_d[1] = cos(angle) * radius; // 自機追従用の相対X
                    pShot->param_d[2] = sin(angle) * radius; // 自機追従用の相対Y
                    pShot->param_i[0] = 1;           // フィールド識別フラグ
                    pShot->kind = img_enemyShotSmallBall[0]; // 赤小玉

                    pShot->prev = pMyShotSet->pEnemyShotHead->prev;
                    pShot->next = pMyShotSet->pEnemyShotHead;
                    pMyShotSet->pEnemyShotHead->prev->next = pShot;
                    pMyShotSet->pEnemyShotHead->prev = pShot;
                }
            }
            else {
                // 2回目以降：既存のフィールドの色を切り替え
                sEnemyShot* pShot = pMyShotSet->pEnemyShotHead->next;
                while (pShot != pMyShotSet->pEnemyShotHead) {
                    if (pShot->param_i[0] == 1) {
                        if (pShot->kind == img_enemyShotSmallBall[0]) {
                            pShot->kind = img_enemyShotSmallBall[4]; // 赤 -> 青
                        }
                        else {
                            pShot->kind = img_enemyShotSmallBall[0]; // 青 -> 赤
                        }
                    }
                    pShot = pShot->next;
                }
            }
        }
    }

    // ---------------------------------------------------------
    // 弾の発射処理 (15フレームおき)
    // ---------------------------------------------------------
    if (count % 2 == 1) {
        double base_muki1 = atan2(player.y - enemy.y, player.x - enemy.x);
        double base_muki2 = atan2(player.y - enemy.y2, player.x - enemy.x2);

        // ボス1: 赤い弾をばら撒き
        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = enemy.x;
            pShot->y = enemy.y;
            pShot->muki = base_muki1 + i * 0.4; // 自機狙いから±0.4ラジアンずらす
            pShot->speed = 2.5;
            pShot->kind = img_enemyShotSmallBall[0]; // 赤

            pShot->prev = pMyShotSet->pEnemyShotHead->prev;
            pShot->next = pMyShotSet->pEnemyShotHead;
            pMyShotSet->pEnemyShotHead->prev->next = pShot;
            pMyShotSet->pEnemyShotHead->prev = pShot;
        }

        // ボス2: 青い弾をばら撒き
        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = enemy.x2;
            pShot->y = enemy.y2;
            pShot->muki = base_muki2 + i * 0.4; // 自機狙いから±0.4ラジアンずらす
            pShot->speed = 2.5;
            pShot->kind = img_enemyShotSmallBall[4]; // 青

            pShot->prev = pMyShotSet->pEnemyShotHead->prev;
            pShot->next = pMyShotSet->pEnemyShotHead;
            pMyShotSet->pEnemyShotHead->prev->next = pShot;
            pMyShotSet->pEnemyShotHead->prev = pShot;
        }
    }
}