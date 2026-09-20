// enemyPat_Tmp.cpp

// ============================================================
// 弾幕：ブラジリアン・リップオフ
//
//  [フェーズ1] 生え際   : 毛弾(菱形弾/橙)が下側エリアにゆっくり生えて揺れる
//  [フェーズ2] ワックス : ワックス弾(中楕円弾/白)が毛の群れを横切る
//  [フェーズ3] 剥がし   : 予兆(全弾フリーズ+チャージ音)の後、
//                         毛・ワックス・レーザーが一方向へ一気に引き剥がされる
//
//  param_i[0]: 弾の状態  0=毛(揺れ中) 1=ワックス 2=レーザー 3=剥がされた毛
//  param_i[1]: 毛の揺れ位相
//  set->param_i[0]: 剥がす方向 (+1=右へ / -1=左へ)
//  set->param_i[1]: 周回数(剛毛化・高速化に使用)
// ============================================================
static void ShotBrazilianWax(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;
    const int   ripdir = pEnemyShotSet->param_i[0];   // 剥がす方向
    const int   cycle = pEnemyShotSet->param_i[1];   // 周回数
    const double ripMuki = (ripdir > 0) ? 0.0 : DX_PI;

    const int HAIR_END = 90;   // 毛を生やす最終フレーム
    const int WARN_FRAME = 230;  // 剥がし予兆開始(ためらい0.5秒)
    const int RIP_FRAME = 260;  // 剥がしの瞬間

    // ---- フェーズ1: 生え際(毛弾をばら撒く) ----
    // 周回が進むほど密度が上がる(剛毛化)
    if (pEnemyShotSet->count < HAIR_END) {
        int hairNum = 1 + (cycle >= 3 ? 1 : 0);
        for (int i = 0; i < hairNum; i++) {
            pShot = new sEnemyShot;

            // GetRand(x) は 0〜x の x+1 種類を返すので注意！
            while (true) {
                pShot->x = 30.0 + GetRand(420 + 60) - 30;
                pShot->y = 140.0 + GetRand(260 + 160) - 80;
                if (hypot(pShot->x - player.x, pShot->y - player.y) > 30) break;
            }
            pShot->muki = 0.0;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotDiamond[8]; // 菱形弾・橙(毛っぽい)
            pShot->param_i[0] = 0;                 // 状態:毛
            pShot->param_i[1] = GetRand(359);      // 揺れの位相

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // ---- フェーズ2: ワックス塗布(毛の帯を横切る白いワックス弾) ----
    if (pEnemyShotSet->count >= 40 && pEnemyShotSet->count <= 150 && pEnemyShotSet->count % 14 == 5) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pShot = new sEnemyShot;
        pShot->x = (ripdir > 0) ? -20.0 : 500.0;  // 剥がす方向とは逆の端から流す
        pShot->y = 140.0 + GetRand(260 + 120) - 60;
        pShot->muki = ripMuki;
        pShot->speed = 2.2;                        // ゆっくり塗布
        pShot->kind = img_enemyShotMediumOval[6]; // 中楕円弾・白(ワックス)
        pShot->param_i[0] = 1;                    // 状態:ワックス

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    // ---- フェーズ3 予兆: グッとためらう(全毛がフリーズ+チャージ音) ----
    if (pEnemyShotSet->count == WARN_FRAME) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ---- フェーズ3: グッときたッ!(一気に剥がし) ----
    if (pEnemyShotSet->count == RIP_FRAME) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 既存の毛・ワックスを剥がす方向へ一気に引っ張る
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                // 毛: ばらけながら高速で剥がれる
                pShot->param_i[0] = 3;
                pShot->muki = ripMuki + (GetRand(30) - 15) / 180.0 * DX_PI;
                pShot->speed = 9.0 + GetRand(6) + cycle * 0.5;
            }
            else if (pShot->param_i[0] == 1) {
                // ワックス: 毛を絡め取ったまま猛スピードで流れ去る
                pShot->muki = ripMuki + (GetRand(20) - 10) / 180.0 * DX_PI;
                pShot->speed = 13.0 + GetRand(4);
            }
            pShot = pShot->next;
        }

        // 剥がれの軌道に短レーザーを追加し、打通過レーンを危険にする
        int laserNum = 4 + (cycle >= 2 ? 2 : 0);
        for (int i = 0; i < laserNum; i++) {
            sEnemyShot* pLaser = new sEnemyShot;
            pLaser->x = (ripdir > 0) ? -40.0 : 520.0;
            pLaser->y = 140.0 + GetRand(260);
            pLaser->muki = ripMuki;
            pLaser->speed = 14.0;
            pLaser->kind = img_enemyShotLaser[5]; // 短レーザー・マゼンタ
            pLaser->param_i[0] = 2;               // 状態:レーザー
            pLaser->margin = 70;

            pLaser->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pLaser->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pLaser;
            pEnemyShotSet->pEnemyShotHead->prev = pLaser;
        }
    }

    // ---- 毎フレームの移動処理 ----
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case 0: // 毛: 揺れながら生え落ちる(予兆中は完全フリーズして「ためらう」)
            if (pEnemyShotSet->count < WARN_FRAME) {
                pShot->x += sin((pShot->count + pShot->param_i[1]) * 0.2) * 0.5;
                pShot->y += 0.15;
            }
            break;
        case 1: // ワックス
        case 2: // レーザー
        case 3: // 剥がされた毛
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_BrazilianWax_Zai()
{
    static int shot_count; // 周回カウンタ(リプレイ再現性のため GetRand は弾側のみで使用)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        shot_count = 0;
    }

    // 敵は画面上部をゆっくり左右に揺れる
    enemy.x = 240.0 + sin(count * 0.02) * 80.0;

    // 1周期(320フレーム)ごとに新しい弾幕セットを生成
    if (count % 320 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBrazilianWax;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count;

        // 剥がす方向を左右交互に切り替え / 周回数を渡して剛毛化
        pEnemyShotSet->param_i[0] = (shot_count % 2 == 0) ? 1 : -1;
        pEnemyShotSet->param_i[1] = shot_count;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        shot_count++;
    }
}