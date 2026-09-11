// enemyPat_Tmp.cpp
// 弾幕：雨下の雷（雷雨の降臨）
// 既存素材のみ使用：小玉・中玉・大玉・短レーザー など

// 雨：画面上部から降り続く小さな弾
static void ShotRain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    // セット生成時に一度だけ雨粒をばら撒く
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 5〜8粒の雨を画面上部から降らせる
        int num = 5 + GetRand(3);
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            // 画面幅いっぱいにランダム配置（雨らしく）
            pEnemyShot->x = (double)GetRand(480);
            pEnemyShot->y = -10.0 - (double)GetRand(30);  // 画面外上部から
            // ほぼ真下、わずかに揺らぎ
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(16) - 8) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.2 + GetRand(120) / 100.0;  // 2.2〜3.4
            // 雨の色：シアン寄り（3）または青（4）
            int col = (GetRand(1) == 0) ? 3 : 4;
            pEnemyShot->kind = img_enemyShotSmallBall[col];
            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 移動のみ（消去はメインルーチン）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 雷撃：予兆 → 稲妻ジグザグ → 着弾放射
static void ShotThunder(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // フェーズ管理（param_i[0] を使用）
    // 0: 予兆中, 1: 稲妻落下中, 2: 着弾済み
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = 0;  // 予兆フェーズ
        // 予兆音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ===== 予兆フェーズ（count 0〜24）：縦の警告線 =====
    if (pEnemyShotSet->param_i[0] == 0 && pEnemyShotSet->count == 1) {
        // 落雷予定位置に短い縦レーザーを数本置いて警告
        double baseX = pEnemyShotSet->x;
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = baseX + (i - 1) * 18.0;
            pEnemyShot->y = 40.0 + i * 30.0;
            pEnemyShot->muki = DX_PI / 2.0;  // 真下向き
            pEnemyShot->speed = 0.4;         // ゆっくり落下（画面外で消去される）
            // 黄色の短レーザーで予兆
            pEnemyShot->kind = img_enemyShotLaser[1];  // 黄
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ===== 稲妻本体発生（count == 25） =====
    if (pEnemyShotSet->param_i[0] == 0 && pEnemyShotSet->count == 25) {
        pEnemyShotSet->param_i[0] = 1;  // 稲妻フェーズへ
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double baseX = pEnemyShotSet->x;
        double startY = 20.0;
        // ジグザグ状に中玉〜大玉を配置して一気に落とす
        for (int i = 0; i < 9; i++) {
            pEnemyShot = new sEnemyShot;
            // 左右に振ってジグザグ
            double offsetX = ((i % 2 == 0) ? -1.0 : 1.0) * (22.0 + GetRand(10));
            pEnemyShot->x = baseX + offsetX;
            pEnemyShot->y = startY + i * 22.0;
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(10) - 5) / 180.0 * DX_PI;
            pEnemyShot->speed = 5.5 + GetRand(80) / 100.0;  // 速め
            // 色は黄〜橙、種類は中玉と大玉を混ぜる
            if (i % 3 == 0) {
                pEnemyShot->kind = img_enemyShotLargeBall[1];  // 黄・大
            }
            else if (i % 3 == 1) {
                pEnemyShot->kind = img_enemyShotMediumBall[8];  // 橙・中
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[1];  // 黄・中
            }
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ===== 着弾爆発（count == 48 付近） =====
    if (pEnemyShotSet->param_i[0] == 1 && pEnemyShotSet->count == 48) {
        pEnemyShotSet->param_i[0] = 2;  // 着弾済み
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 着弾位置（セットのx、やや下）から放射状
        double cx = pEnemyShotSet->x;
        double cy = 280.0 + GetRand(40);
        int num = 14;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx;
            pEnemyShot->y = cy;
            pEnemyShot->muki = (DX_PI * 2.0 * i) / num + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.0 + GetRand(150) / 100.0;
            // 放射は中玉・黄と白を中心に
            int col = (i % 3 == 0) ? 6 : 1;  // 白 or 黄
            pEnemyShot->kind = img_enemyShotMediumBall[col];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        // 追加で少し大玉も
        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + (GetRand(40) - 20);
            pEnemyShot->y = cy + (GetRand(30) - 15);
            pEnemyShot->muki = (DX_PI * 2.0 * i) / 4.0;
            pEnemyShot->speed = 2.5 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotLargeBall[1];  // 黄・大
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾の移動（警告弾はspeed=0なのでその場）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_ThunderInRain_Grok()
{
    static int muki;
    static int rain_timer;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        rain_timer = 0;
    }
    else {
        // ゆっくり左右移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 80.0) {
            enemy.x = 80.0;
            muki = 1;
        }
        if (enemy.x > 400.0) {
            enemy.x = 400.0;
            muki = -1;
        }
        // たまに向きを変える
        if (count % 90 == 0) {
            muki *= -1;
        }
    }

    // ===== 雨の生成（高頻度で小さな雨セットを出す） =====
    rain_timer++;
    if (rain_timer >= 7) {  // 約7フレームごとに雨粒セット
        rain_timer = 0;
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRain;
        pEnemyShotSet->x = enemy.x;          // 位置はあまり使わないがセット
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ===== 雷の生成（一定間隔で強い一撃） =====
    // count 100 から 160 フレームごとに発生（再現性を優先）
    if (count >= 100 && (count - 100) % 160 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotThunder;
        // プレイヤーのx付近を狙う（少しランダム）
        double targetX = player.x + (GetRand(80) - 40);
        if (targetX < 40.0) targetX = 40.0;
        if (targetX > 440.0) targetX = 440.0;
        pEnemyShotSet->x = targetX;
        pEnemyShotSet->y = 30.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}