// 弾幕：バルーン・クラスター・ブレイク（風船の浮遊・破裂・ばら撒き）
static void ShotBalloon(sEnemyShotSet* pEnemyShotSet)
{
    // param_i[0] で状態管理: 0 = 浮遊中の風船, 1 = 破裂後のばら撒き
    if (pEnemyShotSet->param_i[0] == 0) {
        // --- 状態0: 風船として浮遊 ---

        // ふわふわとしたサインカーブ移動
        pEnemyShotSet->y -= 1.2; // 上昇
        pEnemyShotSet->x += sin(pEnemyShotSet->count * 0.05) * 1.5; // 横揺れ

        // 風船のビジュアル表現（セット内に1つだけダミー弾を保持して描画させる）
        if (pEnemyShotSet->count == 0) {
            sEnemyShot* pBalloon = new sEnemyShot;
            pBalloon->x = pEnemyShotSet->x;
            pBalloon->y = pEnemyShotSet->y;
            // 風船らしく、白(6)またはマゼンタ(5)の大玉を使用
            pBalloon->kind = img_enemyShotLargeBall[GetRand(1) == 0 ? 6 : 5];
            pBalloon->margin = 200.0; // 画面外消去を遅らせる

            pBalloon->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBalloon->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBalloon;
            pEnemyShotSet->pEnemyShotHead->prev = pBalloon;
        }
        else {
            // 風船の位置をセットの位置に追従させる
            sEnemyShot* pBalloon = pEnemyShotSet->pEnemyShotHead->next;
            if (pBalloon != pEnemyShotSet->pEnemyShotHead) {
                pBalloon->x = pEnemyShotSet->x;
                pBalloon->y = pEnemyShotSet->y;
            }
        }

        // 【自機弾との当たり判定】
        bool isHit = false;
        sPlayerShot* pPlayerShot = playerShotHead.next;
        while (pPlayerShot != &playerShotHead) {
            // 風船の中心と自機弾の距離の二乗を計算
            double dx = pEnemyShotSet->x - pPlayerShot->x;
            double dy = pEnemyShotSet->y - pPlayerShot->y;
            double distSq = dx * dx + dy * dy;

            // 風船(大玉:半径10) + 自機弾(小玉:半径2.5) の当たり判定をゆるめに設定
            // 距離の二乗が 200.0 (半径約14) 以下ならヒットとみなす
            if (distSq < 200.0) {
                isHit = true;
                break; // 1発でも当たれば破裂処理へ
            }
            pPlayerShot = pPlayerShot->next;
        }

        if (isHit) {
            pEnemyShotSet->param_i[0] = 1; // 状態1へ遷移
            pEnemyShotSet->count = 0;      // ばら撒き用のカウントをリセット

            // 破裂音
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            // 風船のビジュアル弾を画面外へ飛ばして、メインルーチンの消去処理に任せる
            sEnemyShot* pBalloon = pEnemyShotSet->pEnemyShotHead->next;
            if (pBalloon != pEnemyShotSet->pEnemyShotHead) {
                pBalloon->y = -1000.0;
            }
        }
    }
    else if (pEnemyShotSet->param_i[0] == 1) {
        // --- 状態1: 破裂して弾をばら撒く ---

        if (pEnemyShotSet->count == 1) {
            // 破裂瞬間に多数の弾を生成
            int numShots = 16 + GetRand(16); // 16〜32発

            for (int i = 0; i < numShots; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;

                // 全方位ランダムな角度 (GetRand(359) で 0〜359 の 360 種類)
                pShot->muki = GetRand(359) / 359.0 * DX_PI * 2.0;

                // 速度にムラをつける（勢いのある弾と、ヒラヒラ遅い弾を混在）
                pShot->speed = 1.5 + GetRand(300) / 100.0;

                // 風船の破片らしく、小玉か中玉をランダムな色で
                int colorIdx = GetRand(4); // 0〜4
                int colorMap[] = { 0, 1, 3, 5, 8 }; // 赤, 黄, シアン, マゼンタ, 橙
                int c = colorMap[colorIdx];

                if (GetRand(1) == 0) {
                    pShot->kind = img_enemyShotSmallBall[c];
                }
                else {
                    pShot->kind = img_enemyShotMediumBall[c];
                }

                // param_d[0] を空気抵抗（摩擦係数）として使用
                pShot->param_d[0] = 0.992 + GetRand(30) / 10000.0; // 0.92 〜 0.95

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }

        // ばら撒かれた弾の更新
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 空気抵抗による減速（風船の破片が勢いを失う表現）
            pShot->speed *= pShot->param_d[0];

            // 微弱な重力を加えて、ゆっくり落下させる
            pShot->y += 0.15;

            // 【画面端での跳ね返り】（ピンボール風の残留弾幕演出）
            // ゲーム画面は 480x480 と仮定
            if (pShot->x <= 0.0 || pShot->x >= 480.0) {
                pShot->muki = DX_PI - pShot->muki; // X軸の速度を反転
                pShot->x = (pShot->x <= 0.0) ? 1.0 : 479.0; // めり込み防止
            }
            if (pShot->y <= 0.0 || pShot->y >= 480.0) {
                pShot->muki = -pShot->muki; // Y軸の速度を反転
                pShot->y = (pShot->y <= 0.0) ? 1.0 : 479.0; // めり込み防止
            }

            pShot = pShot->next;
        }

        // 一定時間経過したらセット自体を画面外へ飛ばして消去対象にする
        //if (pEnemyShotSet->count > 240) {
        //    pEnemyShotSet->y = -1000.0;
        //}
    }
}

// 敵本体のパターン（風船を撒き散らす親玉）
void EnemyPat_Balloon_Qwen()
{
    static int spawn_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 耐久を高めに設定
        spawn_count = 0;
    }
    else {
        // 親玉は画面上部でゆったりと左右に揺れる
        enemy.x = 240.0 + sin(count * 0.015) * 120.0;
        enemy.y = 60.0 + cos(count * 0.02) * 15.0;
    }

    // 90フレームごとに風船のクラスター（群れ）を生成
    if (count % 90 == 1) {
        // 予告音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 1回の生成で 3〜5 個の風船をばら撒く
        int numBalloons = 3 + GetRand(2);

        for (int i = 0; i < numBalloons; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotBalloon;

            // 横に広がりながら出現させる
            pEnemyShotSet->x = enemy.x + (i - numBalloons / 2.0) * 35.0;
            pEnemyShotSet->y = enemy.y + 40.0 + GetRand(30) + 20;
            pEnemyShotSet->muki = DX_PI / 2.0; // 下向き（初期値として）
            pEnemyShotSet->kind = 0;

            // 状態管理用パラメータを初期化
            pEnemyShotSet->param_i[0] = 0; // 0:浮遊中
            pEnemyShotSet->alive = 600;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        spawn_count++;
    }
}