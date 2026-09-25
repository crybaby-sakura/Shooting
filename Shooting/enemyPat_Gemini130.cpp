// enemyPat_pitchDrop.cpp
// 弾幕パターン：【粘性過多】ピッチ・ドロップ

// ------------------------------------------------------------
// サブ弾幕：ピッチのポタポタ垂れる小滴（自機狙い・牽制用）
// ------------------------------------------------------------
static void ShotSubDrip(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 0フレーム目の初期化
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 小さい涙型の雫（中玉 + 菱形弾）を生成して自機へ向ける
        // 1. 先端の中玉（黒）
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 2.2 + GetRand(60) / 100.0; // 2.2 ～ 2.8
        pEnemyShot->kind = img_enemyShotMediumBall[7]; // 黒
        pEnemyShot->param_i[0] = 0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 2. 尾を引く菱形弾（黒：涙型のシルエット補強）
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x - 6.0 * cos(pEnemyShotSet->muki);
        pEnemyShot->y = pEnemyShotSet->y - 6.0 * sin(pEnemyShotSet->muki);
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = pEnemyShotSet->pEnemyShotHead->next->speed; // 中玉と同速
        pEnemyShot->kind = img_enemyShotDiamond[7]; // 黒
        pEnemyShot->param_i[0] = 1;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 移動処理（直線移動）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// メイン弾幕：【粘性過多】ピッチ・ドロップ
// 極限の「溜め」→「糸の切断と急降下」→「破裂と粘液滞留」
// ------------------------------------------------------------
static void ShotPitchDrop(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    // --- 0フレーム目：ピッチの雫（大玉＋中玉）と糸（銃弾）の生成 ---
    if (pEnemyShotSet->count == 0) {
        // 予告音・溜め音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // パラメータ初期化
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x; // 雫の初期X
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y; // 雫の初期Y
        pEnemyShotSet->param_d[2] = 0.0;             // 落下速度
        pEnemyShotSet->param_i[0] = 0;               // フェーズ (0:膨張・溜め, 1:落下, 2:着弾・粘液滞留)

        // 1. 雫本体：大玉（黒）
        pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = DX_PI / 2.0; // 下向き
        pShot->speed = 0.0;
        pShot->kind = img_enemyShotLargeBall[7]; // 大玉（黒）
        pShot->param_i[0] = 0; // 種別：大玉

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;

        // 2. 雫の先端：中玉（黒）を大玉の下に重ねて「涙型（ティアドロップ）」を作る
        pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y + 14.0;
        pShot->muki = DX_PI / 2.0;
        pShot->speed = 0.0;
        pShot->kind = img_enemyShotMediumBall[7]; // 中玉（黒）
        pShot->param_i[0] = 1; // 種別：中玉

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;

        // 3. 伸ばされた粘液の「糸」：縦に重ねた銃弾（黒）
        for (int i = 0; i < 6; i++) {
            pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y - 8.0 - i * 10.0; // 漏斗の根元へ伸びる糸
            pShot->muki = DX_PI / 2.0;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotBullet[7]; // 銃弾（黒）
            pShot->param_i[0] = 2; // 種別：糸
            pShot->param_i[1] = i; // インデックス

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    int phase = pEnemyShotSet->param_i[0];

    // ============================================================
    // フェーズ0：膨張・伸張（約2秒間の重厚な溜め時間）
    // ============================================================
    if (phase == 0) {
        // 表面張力の限界を表現：微振動 + 時間経過で下へ伸びる
        double vibeX = (GetRand(200) - 100) / 100.0 * 0.4; // -0.4 ～ +0.4 の微振動
        double stretch = (pEnemyShotSet->count / 120.0) * 18.0; // 伸び効果

        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            int type = pShot->param_i[0];
            if (type == 0) { // 大玉
                pShot->x = pEnemyShotSet->param_d[0] + vibeX;
                pShot->y = pEnemyShotSet->param_d[1] + stretch * 0.5;
            }
            else if (type == 1) { // 先端の中玉
                pShot->x = pEnemyShotSet->param_d[0] + vibeX;
                pShot->y = pEnemyShotSet->param_d[1] + 14.0 + stretch;
            }
            else if (type == 2) { // 粘液の糸（銃弾）
                int idx = pShot->param_i[1];
                pShot->x = pEnemyShotSet->param_d[0] + vibeX;
                pShot->y = pEnemyShotSet->param_d[1] - 8.0 - idx * 8.0 + stretch * 0.2;
            }
            pShot = pShot->next;
        }

        // 120フレーム経過で「糸の切断」と「滴下（急降下）」へ移行
        if (pEnemyShotSet->count >= 120) {
            pEnemyShotSet->param_i[0] = 1;   // フェーズ1へ
            pEnemyShotSet->param_d[2] = 0.1; // 初速（ごくゆっくり動き出す）

            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 糸（銃弾）を消去して切断を表現
            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                sEnemyShot* pNext = pShot->next;
                if (pShot->param_i[0] == 2) { // 糸
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                }
                pShot = pNext;
            }
        }
    }
    // ============================================================
    // フェーズ1：切断・落下（重力加速度で一気に急降下）
    // ============================================================
    else if (phase == 1) {
        // 二次関数的な加速（重力）
        pEnemyShotSet->param_d[2] += 0.3;
        double dropSpeed = pEnemyShotSet->param_d[2];

        bool isHitBottom = false;
        double impactX = 0.0;
        double impactY = 0.0;

        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->y += dropSpeed;

            // 着弾判定（画面下部 y >= 440.0 に達したら）
            if (pShot->param_i[0] == 0 && pShot->y >= 480.0) {
                isHitBottom = true;
                impactX = pShot->x;
                impactY = pShot->y;
            }
            pShot = pShot->next;
        }

        // 画面最下部に達したら着弾破裂処理
        if (isHitBottom) {
            pEnemyShotSet->param_i[0] = 2; // フェーズ2へ

            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            // 大玉・中玉（雫本体）を消去
            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                sEnemyShot* pNext = pShot->next;
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = pNext;
            }

            // 着弾地点から大量の粘液（小玉）を上～斜め上方向へ飛散
            int splashCount = 32*2;
            for (int i = 0; i < splashCount; i++) {
                sEnemyShot* pSplash = new sEnemyShot;
                pSplash->x = impactX + (GetRand(24) - 12);
                pSplash->y = impactY + (GetRand(12) - 6);

                // 上および左右方向（-0.85 PI ～ -0.15 PI）へ飛散
                double angle = -DX_PI * 0.95 + (DX_PI * 0.90) * (i / (double)(splashCount - 1));
                angle += (GetRand(20) - 10) / 100.0;

                pSplash->muki = angle;
                pSplash->speed = 2.5 + GetRand(250) / 100.0; // 初期速度 2.5 ～ 5.0

                // 黒(7)を中心に、濁りとして橙(8)とマゼンタ(5)を散りばめる
                if (i % 5 == 0) {
                    pSplash->kind = img_enemyShotSmallBall[8]; // 橙
                }
                else if (i % 5 == 1) {
                    pSplash->kind = img_enemyShotSmallBall[5]; // マゼンタ
                }
                else {
                    pSplash->kind = img_enemyShotSmallBall[7]; // 黒
                }

                pSplash->param_i[0] = 3; // 飛散小玉

                pSplash->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSplash->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSplash;
                pEnemyShotSet->pEnemyShotHead->prev = pSplash;
            }
        }
    }
    // ============================================================
    // フェーズ2：飛散・高粘度滞留（急速にブレーキがかかり低速で残存）
    // ============================================================
    else if (phase == 2) {
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // ピッチの粘度抵抗によるブレーキ効果
            if (pShot->speed > 0.15) {
                pShot->speed *= 0.99; // 急激な減速
            }
            else {
                pShot->speed = 0.08; // 画面最下部に泥沼のように残存
                pShot->muki = DX_PI / 2.0; // ごくゆっくり沈下
            }

            pShot = pShot->next;
        }
    }
}

// ------------------------------------------------------------
// 敵本体パターン処理関数
// ------------------------------------------------------------
void EnemyPat_PitchDrop_Gemini()
{
    static int muki;
    static int shot_count;

    // --- 初期化処理 ---
    if (count == 1) {
        // 画面中央上部（漏斗の位置）
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // HP 200固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵本体は左右へゆっくりゆらめく
        enemy.x += 0.6 * (double)muki;
        if (count % 160 == 80) {
            muki *= -1;
        }
    }

    // --- メイン弾幕：ピッチドロップの発射（180フレーム＝3秒周期） ---
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPitchDrop;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // --- サブ弾幕：漏斗からポタポタ垂れる小滴（45フレーム周期） ---
    if (count % 45 == 25) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSubDrip;
        pEnemyShotSet->x = enemy.x + (GetRand(20) - 10);
        pEnemyShotSet->y = enemy.y + 15.0;
        // 自機へ向けた角度
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}