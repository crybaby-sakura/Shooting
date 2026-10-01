// enemyPat_Tmp.cpp
// 陰蜂風弾幕パターン実装

// ---------------------------------------------------------
// 1. 土台：蜂の巣パターン（金色・低速・螺旋回転）
// ---------------------------------------------------------
static void ShotMandalaBase(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 六角形(6方向) × 10段 = 60発の曼荼羅状配置
        for (int ring = 0; ring < 10; ring++) {
            for (int i = 0; i < 6; i++) {
                sEnemyShot* pShot = new sEnemyShot;

                // param_d[0]: 初期角度, param_d[1]: 半径, param_d[2]: 角速度
                double base_angle = (i * 60 + ring * 12) * DX_PI / 180.0;
                pShot->param_d[0] = base_angle;
                pShot->param_d[1] = ring * 25.0;
                pShot->param_d[2] = 0.025 + ring * 0.005;

                // 黄色(1)の中玉で蜂の巣を表現
                pShot->kind = img_enemyShotMediumBall[1];
                pShot->margin = 480;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 螺旋運動の更新（半径拡大＋回転）
        pShot->param_d[1] += 1.2;
        pShot->param_d[0] += pShot->param_d[2];

        pShot->x = pEnemyShotSet->x + pShot->param_d[1] * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y + pShot->param_d[1] * sin(pShot->param_d[0]);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 2. 追撃：黒蓮放射（黒色・高速・急加速）
// ---------------------------------------------------------
static void ShotBlackLotus(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // プレイヤー狙い方向を基準に±30度の範囲に15発ばら撒き
        double base_muki = pEnemyShotSet->muki;
        for (int i = 0; i < 15; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = base_muki + (i - 7) * 4.0 * DX_PI / 180.0;
            pShot->speed = 1.5; // 初期速度はあえて遅く

            // 黒色(7)の銃弾で鋭い印象に
            pShot->kind = img_enemyShotBullet[7];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 時間経過とともに急加速（予測を困難にする）
        pShot->speed += 0.15;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 3. 分裂：呪縛の種（紫色・中速・分裂と簡易追尾）
// ---------------------------------------------------------
static void ShotCurseSeed(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 画面全体へ向けて12方向に中速弾を発射
        for (int i = 0; i < 12; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = (i * 30) * DX_PI / 180.0;
            pShot->speed = 3.0;
            pShot->param_i[0] = 0; // 分裂フラグ(0:未分裂)

            // マゼンタ色(5)の菱形弾で「呪い」の種を表現
            pShot->kind = img_enemyShotDiamond[5];
            pShot->margin = 240;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }
    else if (pEnemyShotSet->count == 60) {
        // 分裂処理: 既存の弾の位置から3方向に新しい弾を生成
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                pShot->param_i[0] = 1; // 分裂済みフラグ

                for (int i = 0; i < 3; i++) {
                    sEnemyShot* pNewShot = new sEnemyShot;
                    pNewShot->x = pShot->x;
                    pNewShot->y = pShot->y;
                    // 元の進行方向から -45度, 0度, +45度
                    pNewShot->muki = pShot->muki + (i - 1) * 45.0 * DX_PI / 180.0;
                    pNewShot->speed = 4.5;
                    pNewShot->param_i[0] = 1; // 分裂弾フラグ

                    // 分裂後はマゼンタ色(5)の小玉に
                    pNewShot->kind = img_enemyShotSmallBall[5];
                    pNewShot->margin = 120;

                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }
            }
            pShot = pShot->next;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 分裂弾(param_i[0]==1)はプレイヤーへ緩やかに追尾
        if (pShot->param_i[0] == 1) {
            double target_muki = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = target_muki - pShot->muki;

            // 角度差を -PI 〜 PI の範囲に正規化
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;

            // 徐々に目標角度へ近づける
            pShot->muki += diff * 0.05;
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 4. 仕上げ：蜂の羽撃（白色・変速・直前かすめ）
// ---------------------------------------------------------
static void ShotBeeWing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 画面上部から下部へ向かう弾を10発ランダム配置
        for (int i = 0; i < 10; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            // GetRand(400) は 0〜400 を返すため、x座標は 40.0 〜 440.0 になる
            pShot->x = 40.0 + GetRand(400);
            pShot->y = -20.0;
            pShot->muki = DX_PI / 2.0; // 真下
            pShot->speed = 8.0;        // 初期は高速

            // 白色(6)の中楕円弾で羽の動きを表現
            pShot->kind = img_enemyShotMediumOval[6];
            pShot->margin = 30;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 変速処理: 画面中央(y=240)付近で減速し、端に行くにつれて再加速
        double dist_to_center = abs(pShot->y - 240.0);
        double target_speed = 2.0 + (dist_to_center * 0.03); // 中央で2.0、端で約10.0

        // 現在の速度を目標速度へ徐々に近づける（滑らかな減速・加速）
        if (pShot->speed < target_speed) {
            pShot->speed += 0.2;
            if (pShot->speed > target_speed) pShot->speed = target_speed;
        }
        else {
            pShot->speed -= 0.2;
            if (pShot->speed < target_speed) pShot->speed = target_speed;
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 敵本体のパターン制御
// ---------------------------------------------------------
void EnemyPat_Inbachi_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // サンプルに準拠
    }
    else {
        // 左右にゆっくり往復し、上下にもわずかに揺れる不気味な浮遊感
        enemy.x = 240.0 + 160.0 * sin(count * 0.015);
        enemy.y = 80.0 + 20.0 * sin(count * 0.03);
    }

    // 300フレーム(約5秒)周期で4つの弾幕を順番に発動
    // phase 0: 蜂の巣, phase 1: 黒蓮放射, phase 2: 呪縛の種, phase 3: 蜂の羽撃
    int phase = (count / 60) % 4;
    int offset = count % 60;

    // 各フェーズの開始タイミング(offset == 1)で弾幕セットを生成
    if (offset == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // フェーズに応じたパターン関数を割り当て
        if (phase == 0) {
            pEnemyShotSet->patternFunc = ShotMandalaBase;
        }
        else if (phase == 1) {
            pEnemyShotSet->patternFunc = ShotBlackLotus;
        }
        else if (phase == 2) {
            pEnemyShotSet->patternFunc = ShotCurseSeed;
        }
        else {
            pEnemyShotSet->patternFunc = ShotBeeWing;
        }

        // 連結リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}