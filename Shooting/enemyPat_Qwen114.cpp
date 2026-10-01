// enemyPat_nightSkyCarpet.cpp
// 流符「夜空のカーペット」実装ファイル

// 弾幕：流符「夜空のカーペット」
static void ShotNightSkyCarpet(sEnemyShotSet* pEnemyShotSet)
{
    // 発動直後の予告音
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 【星空のカーペット】波打ちながら画面を下へ敷き詰める弾幕
    // 40フレームごとに1列ずつ生成し、400フレーム(約6.6秒)で生成終了
    if (pEnemyShotSet->count % 70 == 0 && pEnemyShotSet->count <= 400) {
        // 時間経過で下へ進む波の基準Y座標
        double wave_y = (pEnemyShotSet->count / 70) * 70.0;

        for (int i = 0; i <= 16; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            // 横方向は等間隔(30ピクセルごと)、Y座標は正弦波でずらして「敷物」の波打ちを表現
            pShot->x = 0.0 + i * 30.0;
            pShot->y = wave_y + sin(i * 0.6 + pEnemyShotSet->count * 0.05) * 25.0;
            pShot->margin = 120;

            // 夜空らしい色：3:シアン、4:青、6:白 からランダム選択
            // GetRand(2) は 0, 1, 2 を返すため、要素数3の配列に安全にアクセス可能
            int colors[] = { 3, 4, 6 };
            int color = colors[GetRand(2)];

            // 星の輝きのバラつきを表現するため、小玉と鱗弾をランダムに混在させる
            if (GetRand(1) == 0) {
                pShot->kind = img_enemyShotSmallBall[color];
            }
            else {
                pShot->kind = img_enemyShotScale[color];
            }

            pShot->muki = DX_PI / 2.0; // 基本は下向き
            // GetRand(10)は0~10を返す。0.0~0.5が加算され、速度は0.6~1.1のゆっくりした値に
            pShot->speed = 0.6 + GetRand(10) / 20.0;

            // 弾の種類を識別するためのフラグ（移動処理で使用）
            pShot->param_i[0] = 1; // 1:星空の弾

            // 循環双方向リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 【流れ星】画面外から斜めに高速で貫く弾
    // 星空の生成タイミング(40の倍数)とずらし、90フレームごとに発射
    if (pEnemyShotSet->count % 90 == 45 && pEnemyShotSet->count <= 400) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = new sEnemyShot;

        // 画面上部(40~440の範囲)から出現
        pShot->x = 40.0 + GetRand(400);
        pShot->y = -30.0;
        pShot->margin = 40;

        // 下向き(DX_PI/2)を基準に、左右どちらかに傾く（-30度〜+30度の範囲）
        // GetRand(60)は0~60を返す。-30すると-30~30になり、180で割ってPIを掛けることでラジアンに変換
        double angle_offset = (GetRand(60) - 30) / 180.0 * DX_PI;
        pShot->muki = DX_PI / 2.0 + angle_offset;

        // 流れ星は白または青の短レーザーで表現
        // GetRand(1)は0または1を返す
        int laser_colors[] = { 4, 6 };
        pShot->kind = img_enemyShotLaser[laser_colors[GetRand(1)]];

        // GetRand(20)は0~20を返す。0.0~2.0が加算され、速度は7.0~9.0の高速値に
        pShot->speed = 7.0 + GetRand(20) / 10.0;

        pShot->param_i[0] = 0; // 0:流れ星

        // 循環双方向リストに追加
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 星空の弾：横揺れを加えてふわふわした夢幻的な動きに
            pShot->x += sin(pShot->count * 0.08) * 0.8;
            pShot->y += pShot->speed;
        }
        else {
            // 流れ星：設定された角度へ直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_NightCarpet_Qwen()
{
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 6ボスとしての耐久力
        shot_count = 0;
    }
    else {
        // 「星舞譚」の名にふさわしく、優雅に横方向に大きく、縦方向に小さく揺れ動く
        enemy.x = 240.0 + sin(count * 0.015) * 160.0;
        enemy.y = 80.0 + cos(count * 0.025) * 40.0;
    }

    // 弾幕セットの生成（5秒 = 300フレームごとに発動）
    if (count % 300 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNightSkyCarpet;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        // 弾リストのヘッダ初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾幕セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}