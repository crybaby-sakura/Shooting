// enemyPat_Tmp.cpp
// くるくるくるりん風・回転棒弾パターン

// 弾幕：くるりん風回転棒
// 小型弾で棒本体、両端だけ中型弾。一定時間剛体回転した後に分解して飛散する。
static void ShotKuruBar(sEnemyShotSet* pEnemyShotSet)
{
    // 棒の構成パラメータ
    const int   NUM_SHOTS = 21;          // 弾数（奇数推奨）
    const double SPACING = 7.5;         // 弾間隔
    const double HALF_LEN = SPACING * ((NUM_SHOTS - 1) / 2.0);

    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 初期角度・角速度をセットに保存
        // param_d[0] : 現在角度
        // param_d[1] : 角速度（符号で回転方向）
        pEnemyShotSet->param_d[0] = (GetRand(360) * DX_PI) / 180.0;           // 0〜360度
        pEnemyShotSet->param_d[1] = 0.022 * (GetRand(1) == 0 ? 1.0 : -1.0);   // 約1.26度/frame

        // 棒を構成する弾を生成
        for (int i = 0; i < NUM_SHOTS; i++) {
            pEnemyShot = new sEnemyShot;

            // 中心からの符号付き距離を param_d[0] に保存
            double dist = (i - (NUM_SHOTS - 1) / 2.0) * SPACING;
            pEnemyShot->param_d[0] = dist;

            // 初期位置
            double ang = pEnemyShotSet->param_d[0];
            pEnemyShot->x = pEnemyShotSet->x + dist * cos(ang);
            pEnemyShot->y = pEnemyShotSet->y + dist * sin(ang);

            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;

            // 本体は小型弾（シアン）、両端だけ中型弾で棒感を強調
            if (i == 0 || i == NUM_SHOTS - 1) {
                pEnemyShot->kind = img_enemyShotMediumBall[3];  // シアン中玉
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[3];    // シアン小玉
            }
            pEnemyShot->margin = 240;

            // リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ----- 毎フレーム更新 -----
    const int LIFE_RIGID = 300;  // 剛体回転を続けるフレーム数

    if (pEnemyShotSet->count < LIFE_RIGID) {
        // 剛体回転フェーズ
        pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[1];  // 角度更新

        // 中心をわずかに下へ（くるりんがゆっくり降りてくるイメージ）
        pEnemyShotSet->y += 0.35 * 4;

        // 全弾を現在角度で再配置
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            double dist = pEnemyShot->param_d[0];
            double ang = pEnemyShotSet->param_d[0];
            pEnemyShot->x = pEnemyShotSet->x + dist * cos(ang);
            pEnemyShot->y = pEnemyShotSet->y + dist * sin(ang);
            pEnemyShot = pEnemyShot->next;
        }
    }
    else {
        // 分解フェーズ（最初の1フレームだけ速度を与える）
        if (pEnemyShotSet->count == LIFE_RIGID) {
            pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
                // 棒の向きに沿って外側へ飛び散らせる
                double dist = pEnemyShot->param_d[0];
                double ang = pEnemyShotSet->param_d[0];
                if (dist >= 0.0) {
                    pEnemyShot->muki = ang;
                }
                else {
                    pEnemyShot->muki = ang + DX_PI;
                }
                // 少しばらつきを持たせる
                pEnemyShot->muki += (GetRand(40) - 20) / 180.0 * DX_PI;
                pEnemyShot->speed = 1.4 + GetRand(120) / 100.0;
                pEnemyShot = pEnemyShot->next;
            }
        }

        // 以降は通常の直進
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            pEnemyShot = pEnemyShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_KuruKuruKururin_Grok()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // 左右にゆっくり揺れる
        enemy.x += 0.7 * (double)muki;
        if (count % 160 == 80) {
            muki *= -1;
        }
    }

    // 一定間隔で回転棒を生成（最大2本程度が同時に存在するくらいの間隔）
    if (count % 110 == 1) {
        for (int i = -1; i <= 1; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotKuruBar;
            pEnemyShotSet->x = enemy.x + 150 * i;;
            pEnemyShotSet->y = enemy.y + 12.0;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = 0;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            // リンクリストに追加
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}