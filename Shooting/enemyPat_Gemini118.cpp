// 弾幕：回文「竹やぶ焼けた」
static void ShotTakeyabuYAKETA(sEnemyShotSet* pEnemyShotSet)
{
    // ----------------------------------------------------
    // 1. 往路「た・け・や・ぶ」：竹の伸長
    // ----------------------------------------------------
    if (pEnemyShotSet->count < 45) {
        // 3フレーム毎に弾を生成し、直線状の竹を形成する
        if (pEnemyShotSet->count % 3 == 0) {
            // 発射音は少し間引いて鳴らす
            if (pEnemyShotSet->count % 12 == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki;
            pEnemyShot->speed = 6.0+4;

            // 15フレーム周期で「節」を作る、それ以外は「幹」
            if (pEnemyShotSet->count % 15 == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[2]; // 緑の中玉(節)
                pEnemyShot->param_i[0] = 1;                    // 節フラグ
            }
            else {
                pEnemyShot->kind = img_enemyShotLaser[2];      // 緑の短レーザー(幹)
                pEnemyShot->param_i[0] = 0;                    // 幹フラグ
            }

            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ----------------------------------------------------
    // 2. 転換準備：一時停止
    // ----------------------------------------------------
    if (pEnemyShotSet->count == 45) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->speed = 0.0; // 竹の伸長を止める
            pShot = pShot->next;
        }
    }

    // ----------------------------------------------------
    // 3. 転換「焼けた」：着火と色反転
    // ----------------------------------------------------
    if (pEnemyShotSet->count == 65) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 予告音で着火を表現

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 弾の色を緑[2]から赤[0]へ一斉に変更
            if (pShot->param_i[0] == 1) {
                pShot->kind = img_enemyShotMediumBall[0]; // 赤の中玉
            }
            else if (pShot->param_i[0] == 0) {
                pShot->kind = img_enemyShotLaser[0];      // 赤の短レーザー
            }
            pShot = pShot->next;
        }
    }

    // ----------------------------------------------------
    // 4. 復路「た・け・や・ぶ」：逆再生と火の粉の散布
    // ----------------------------------------------------
    if (pEnemyShotSet->count == 90) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        sEnemyShot* pEnd = pEnemyShotSet->pEnemyShotHead;

        // リストに新しい弾を追加するため、ループの終端(pEnd)をあらかじめ決めておく
        while (pShot != pEnd) {
            // 竹の構成弾(幹と節)は速度をマイナスにして逆走させる（回文の表現）
            if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
                pShot->speed = -6.0;
            }

            // 節だった弾からは、全方位に火の粉を撒き散らす
            if (pShot->param_i[0] == 1) {
                for (int i = 0; i < 8*2; i++) {
                    sEnemyShot* fire = new sEnemyShot;
                    fire->x = pShot->x;
                    fire->y = pShot->y;

                    // 8方向に均等分割 ＋ わずかなランダムなブレ (-0.1 ～ 0.1 ラジアン)
                    fire->muki = (DX_PI * 2.0 / 8/2) * i + (GetRand(20) - 10) / 100.0;
                    // 速度も少しばらけさせる (1.5 ～ 3.0)
                    fire->speed = 1.5 + GetRand(150) / 100.0;

                    fire->kind = img_enemyShotSmallBall[8]; // 橙の小玉(火の粉)
                    fire->param_i[0] = 2;                   // 火の粉フラグ

                    // リストへ追加 (末尾に入るため今回の走査には影響しない)
                    fire->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    fire->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = fire;
                    pEnemyShotSet->pEnemyShotHead->prev = fire;
                }
            }
            pShot = pShot->next;
        }
    }

    // ----------------------------------------------------
    // 5. 座標更新 (全てのフレームで実行)
    // ----------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // speedがマイナスになった逆走弾もここで正しく戻っていく
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_TakeyabuYaketa_Gemini()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 敵は左右にゆっくり動く
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 200フレーム（約3.3秒）間隔で「竹やぶ」を発射
    if (count % 200 == 1) {
        // 自機狙いの角度を基準にする
        double base_angle = atan2(player.y - enemy.y, player.x - enemy.x);

        // 扇状に3本の竹を伸ばす
        for (int i = -1; i <= 1; i++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotTakeyabuYAKETA;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            // 自機狙いからそれぞれ左右に22.5度(DX_PI / 8)ずらす
            pSet->muki = base_angle + i * (DX_PI / 8.0);
            pSet->kind = 0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}
