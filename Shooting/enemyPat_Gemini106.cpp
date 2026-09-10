// ----------------------------------------------------
// ボス1(赤)の弾幕：右回転の16way
// ----------------------------------------------------
static void ShotRed(sEnemyShotSet* pEnemyShotSet)
{
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // 定期的に発射
    if (pEnemyShotSet->count % 6 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 16; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            // 回転させながら発射
            pShot->muki = (i * DX_PI * 2.0 / 16.0) + (pEnemyShotSet->count * 0.02);
            pShot->speed = 2.0;
            pShot->kind = img_enemyShotMediumBall[0]; // 赤中玉
            pShot->param_i[0] = 0; // 色判定用のフラグ（赤=0）

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ----------------------------------------------------
// ボス2(青)の弾幕：左回転の16way
// ----------------------------------------------------
static void ShotBlue(sEnemyShotSet* pEnemyShotSet)
{
    pEnemyShotSet->x = enemy.x2;
    pEnemyShotSet->y = enemy.y2;

    if (pEnemyShotSet->count % 6 == 0) {
        for (int i = 0; i < 16; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            // 逆回転させながら発射
            pShot->muki = (i * DX_PI * 2.0 / 16.0) - (pEnemyShotSet->count * 0.02);
            pShot->speed = 2.0;
            pShot->kind = img_enemyShotMediumBall[4]; // 青中玉
            pShot->param_i[0] = 4; // 色判定用のフラグ（青=4）

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ----------------------------------------------------
// 属性バリアの制御
// ----------------------------------------------------
static void ShotBarrier(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;

    // 3秒(180フレーム)ごとに予告音を鳴らす (0秒, 3秒, 6秒...)
    if (t % 180 == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 予告音の1秒後(60フレーム後)に属性変化 (1秒, 4秒, 7秒...)
    if (t % 180 == 60) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 0:赤, 1:青
        int phase = (t / 180) % 2;
        int newKind = (phase == 0) ? img_enemyShotSmallBall[0] : img_enemyShotSmallBall[4];

        if (pEnemyShotSet->pEnemyShotHead->next == pEnemyShotSet->pEnemyShotHead) {
            // 初回生成時：自機を取り囲むように4つ配置
            for (int i = 0; i < 4; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->kind = newKind;
                pShot->param_d[1] = i * (DX_PI * 2.0 / 4.0); // 自機周囲の配置角度
                pShot->margin = 999.0;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
        else {
            // 既に配置されている場合は色だけ変化させる
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                pShot->kind = newKind;
                pShot = pShot->next;
            }
        }
    }

    // バリア玉の追従と、同色敵弾の消滅判定処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 自機を取り囲むように少しずつ回転させる
        pShot->param_d[1] += 0.15;
        pShot->x = player.x + 40.0 * cos(pShot->param_d[1]);
        pShot->y = player.y + 40.0 * sin(pShot->param_d[1]);

        int barrierColor = (pShot->kind == img_enemyShotSmallBall[0]) ? 0 : 4;

        // すべての敵弾セットを走査して当たり判定を行う
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet != pEnemyShotSet) { // バリア自身のセットは除外
                sEnemyShot* pEnemyBullet = pSet->pEnemyShotHead->next;
                while (pEnemyBullet != pSet->pEnemyShotHead) {

                    // ボスが発射した弾の色フラグとバリアの色が一致するかチェック
                    if (pEnemyBullet->param_i[0] == barrierColor) {
                        double dx = pEnemyBullet->x - pShot->x;
                        double dy = pEnemyBullet->y - pShot->y;

                        // バリアに触れた場合、弾を消滅させる
                        if (dx * dx + dy * dy < 15.0 * 15.0) {
                            sEnemyShot* delShot = pEnemyBullet;
                            pEnemyBullet = pEnemyBullet->next; // 削除前に次へ進める

                            // リストから外してメモリプールに返却
                            delShot->prev->next = delShot->next;
                            delShot->next->prev = delShot->prev;
                            delete delShot;
                            continue;
                        }
                    }
                    pEnemyBullet = pEnemyBullet->next;
                }
            }
            pSet = pSet->next;
        }
        pShot = pShot->next;
    }
}

// ----------------------------------------------------
// 敵本体のパターン関数
// ----------------------------------------------------
void EnemyPat_Ikaruga_Gemini()
{
    if (count == 1) {
        // ボス1(赤)とボス2(青)の初期位置
        enemy.x = 120.0;
        enemy.y = 80.0;
        enemy.x2 = 360.0;
        enemy.y2 = 80.0;
        enemy.maxHp = enemy.hp = 200;

        // --- 1. バリア制御セットの登録 ---
        sEnemyShotSet* pBarrierSet = new sEnemyShotSet;
        pBarrierSet->count = 0;
        pBarrierSet->patternFunc = ShotBarrier;
        pBarrierSet->pEnemyShotHead = new sEnemyShot;
        pBarrierSet->pEnemyShotHead->prev = pBarrierSet->pEnemyShotHead;
        pBarrierSet->pEnemyShotHead->next = pBarrierSet->pEnemyShotHead;

        pBarrierSet->prev = enemyShotSetHead.prev;
        pBarrierSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBarrierSet;
        enemyShotSetHead.prev = pBarrierSet;

        // --- 2. 赤弾幕(ボス1)セットの登録 ---
        sEnemyShotSet* pRedSet = new sEnemyShotSet;
        pRedSet->count = 0;
        pRedSet->patternFunc = ShotRed;
        pRedSet->pEnemyShotHead = new sEnemyShot;
        pRedSet->pEnemyShotHead->prev = pRedSet->pEnemyShotHead;
        pRedSet->pEnemyShotHead->next = pRedSet->pEnemyShotHead;

        pRedSet->prev = enemyShotSetHead.prev;
        pRedSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pRedSet;
        enemyShotSetHead.prev = pRedSet;

        // --- 3. 青弾幕(ボス2)セットの登録 ---
        sEnemyShotSet* pBlueSet = new sEnemyShotSet;
        pBlueSet->count = 0;
        pBlueSet->patternFunc = ShotBlue;
        pBlueSet->pEnemyShotHead = new sEnemyShot;
        pBlueSet->pEnemyShotHead->prev = pBlueSet->pEnemyShotHead;
        pBlueSet->pEnemyShotHead->next = pBlueSet->pEnemyShotHead;

        pBlueSet->prev = enemyShotSetHead.prev;
        pBlueSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBlueSet;
        enemyShotSetHead.prev = pBlueSet;
    }

    // 2体のボスを緩やかに左右に揺らす
    enemy.x += 0.5 * sin(count * 0.02);
    enemy.x2 -= 0.5 * sin(count * 0.02);
}