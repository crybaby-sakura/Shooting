// enemyPat_Tmp.cpp
// スイカ割りをモチーフにした弾幕パターン
// 使用素材：
//   - 中玉・大玉（緑=皮、赤=果肉）
//   - 小玉（黒=種）
//   - 短レーザー（棒）
//   - 効果音：sound_enemyCharge / sound_enemyShot_heavy / sound_enemyShot_medium

// スイカ割りショットセットのパターン関数
static void ShotWatermelonSmash(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // -------------------------------------------------------
    // count == 0 : スイカ本体と棒の弾を生成
    // -------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const double cx = pEnemyShotSet->x;
        const double cy = pEnemyShotSet->y;

        // ---- スイカの皮（緑の中玉を円形に配置） ----
        const int skinNum = 14*5;
        for (int i = 0; i < skinNum; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = (double)i / skinNum * 2.0 * DX_PI;
            // 少し縦長の楕円形にする
            pEnemyShot->x = cx + 42.0 * cos(ang);
            pEnemyShot->y = cy + 36.0 * sin(ang);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[2]; // 緑
            pEnemyShot->param_i[0] = 0; // 0=皮
            pEnemyShot->param_d[0] = ang; // 初期角度を記憶

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ---- スイカの果肉（赤の大玉・中玉を内側に） ----
        // 中心大玉
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = cx;
        pEnemyShot->y = cy;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤
        pEnemyShot->param_i[0] = 1; // 1=果肉
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 周囲の中玉
        for (int i = 0; i < 6*5; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = (double)i / 6.0/5 * 2.0 * DX_PI + DX_PI / 6.0;
            pEnemyShot->x = cx + 18.0 * cos(ang);
            pEnemyShot->y = cy + 15.0 * sin(ang);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ---- 種（黒の小玉） ----
        for (int i = 0; i < 9*5; i++) {
            pEnemyShot = new sEnemyShot;
            // 内側にランダム気味に配置
            double ang = (double)i / 9.0/5 * 2.0 * DX_PI + GetRand(30) / 180.0 * DX_PI;
            double r = 8.0 + GetRand(12);
            pEnemyShot->x = cx + r * cos(ang);
            pEnemyShot->y = cy + r * 0.85 * sin(ang);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒
            pEnemyShot->param_i[0] = 2; // 2=種
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ---- 棒（短レーザーを数本つないで1本の棒に見せる） ----
        // セグメント数
        const int stickSeg = 5;
        for (int i = 0; i < stickSeg; i++) {
            pEnemyShot = new sEnemyShot;
            // 初期位置はスイカの真上（後で毎フレーム再計算）
            pEnemyShot->x = cx;
            pEnemyShot->y = cy - 180.0 - i * 55.0;
            pEnemyShot->muki = DX_PI / 2.0; // 下向き
            pEnemyShot->speed = 0.0;
            // 黄色〜オレンジ系で木の棒っぽく
            pEnemyShot->kind = img_enemyShotLaser[1]; // 黄
            pEnemyShot->param_i[0] = 3; // 3=棒
            pEnemyShot->param_i[1] = i; // セグメント番号
            pEnemyShot->param_d[0] = 40.0 + i * 55.0; // ピボットからの距離
            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // フェーズ管理用
        // param_i[0] : 0=待機, 1=振り下ろし中, 2=割れた後
        pEnemyShotSet->param_i[0] = 0;
        // 振り下ろしの進行度（0.0〜1.0）
        pEnemyShotSet->param_d[0] = 0.0;
    }

    // -------------------------------------------------------
    // 毎フレームの更新
    // -------------------------------------------------------
    const double cx = pEnemyShotSet->x;
    const double cy = pEnemyShotSet->y;

    // フェーズ遷移
    if (pEnemyShotSet->count == 70) {
        // 振り下ろし開始
        pEnemyShotSet->param_i[0] = 1;
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
    if (pEnemyShotSet->count == 115) {
        // ヒット！スイカ爆発
        pEnemyShotSet->param_i[0] = 2;
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 振り下ろし進行度（70〜115フレームで0→1）
    if (pEnemyShotSet->param_i[0] == 1) {
        double t = (pEnemyShotSet->count - 70) / 45.0;
        if (t > 1.0) t = 1.0;
        // イージングで少し加速感を出す
        pEnemyShotSet->param_d[0] = t * t;
    }

    // 全弾の位置・速度更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int type = pShot->param_i[0];

        if (type == 3) {
            // ========== 棒の更新 ==========
            // ピボットはスイカの少し上
            double pivotX = cx;
            double pivotY = cy - 90.0;

            // 振り下ろし角度：真上(-PI/2) から 真下(PI/2) へ
            // 少し左右にオフセットして「振り下ろす」感じを出す
            double baseAng = -DX_PI / 2.0;
            double swing = pEnemyShotSet->param_d[0] * DX_PI; // 0〜π
            double ang = baseAng + swing;

            // 少し横方向にずらす（右から左へ振り下ろすイメージ）
            double side = 60.0 * (1.0 - pEnemyShotSet->param_d[0]);

            double dist = pShot->param_d[0];
            pShot->x = pivotX + side + dist * cos(ang);
            pShot->y = pivotY + dist * sin(ang);
            pShot->muki = ang; // レーザーの向きも合わせる

            // 割れた後は棒を画面外へ飛ばす
            if (pEnemyShotSet->param_i[0] >= 2) {
                pShot->speed = 8.0 + pShot->param_i[1] * 0.5;
                pShot->muki = ang + (GetRand(40) - 20) / 180.0 * DX_PI;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else if (pEnemyShotSet->param_i[0] >= 2) {
            // ========== スイカ破片の爆発 ==========
            // まだ止まっている弾に初速を与える
            if (pShot->speed < 0.01) {
                // 中心からの放射方向 + 少しランダム
                double dx = pShot->x - cx;
                double dy = pShot->y - cy;
                double baseAng = atan2(dy, dx);
                pShot->muki = baseAng + (GetRand(50) - 25) / 180.0 * DX_PI;

                if (type == 2) {
                    // 種は速めに飛ぶ
                    pShot->speed = 2.8 + GetRand(120) / 100.0;
                    pShot->kind = img_enemyShotSmallBall[7]; // 黒のまま
                }
                else if (type == 1) {
                    // 果肉は中速
                    pShot->speed = 1.8 + GetRand(100) / 100.0;
                    // 少し小さく見せるために中玉のまま、または小玉に変えても良い
                    pShot->kind = img_enemyShotMediumBall[0];
                }
                else {
                    // 皮
                    pShot->speed = 1.4 + GetRand(80) / 100.0;
                    pShot->kind = img_enemyShotMediumBall[2];
                }
            }
            // 移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // わずかに減速（自然な感じ）
            pShot->speed *= 0.992;
        }
        // type 0/1/2 でフェーズ0,1の間は speed=0 のまま静止

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Suikawari_Grok()
{
    static int muki;
    static int nextSmashFrame;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        nextSmashFrame = 90; // 最初のスイカ割り開始タイミング
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
    }

    // 一定間隔でスイカ割りパターンを発動
    if (count == nextSmashFrame) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWatermelonSmash;

        // スイカの出現位置（画面中央寄り、少しランダム）
        pEnemyShotSet->x = 180.0 + GetRand(120);
        pEnemyShotSet->y = 160.0 + GetRand(80);
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 次の発動タイミング（約4〜5秒後）
        nextSmashFrame = count + 240 + GetRand(60);
    }
}