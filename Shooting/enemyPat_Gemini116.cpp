// enemyPat_CloudChamber.cpp

// ======================================================================
// 弾幕：環境演出（過飽和蒸気）
// ======================================================================
static void ShotEnv(sEnemyShotSet* pEnemyShotSet)
{
    // 4フレームに1回、画面上部から微小弾（シアンの小玉）を降らせる
    if (pEnemyShotSet->count % 6 == 0) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = GetRand(480); // 画面全幅
        pEnemyShot->y = -10.0;
        // 真下(PI/2)から少しだけ揺らぎを持たせる
        pEnemyShot->muki = DX_PI / 2.0 + (GetRand(20) - 10) / 100.0;
        pEnemyShot->speed = 0.5 + GetRand(50) / 100.0;
        pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン小玉

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ======================================================================
// 弾幕：α線（太く直線的な重軌跡）
// ======================================================================
static void ShotAlpha(sEnemyShotSet* pEnemyShotSet)
{
    // 0フレーム目でチャージ（予告）音
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 40フレーム目で発射
    if (pEnemyShotSet->count == 40) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 自機方向を中心に少しバラけさせる
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 10.0 + GetRand(40) / 10.0;
            pEnemyShot->kind = img_enemyShotBullet[0]; // 赤銃弾（親弾）
            pEnemyShot->param_i[0] = 0; // フラグ 0:親弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動と軌跡生成処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) { // 親弾の処理
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 毎フレーム軌跡（白の中玉）を生成してその場に残す
            sEnemyShot* pTrace = new sEnemyShot;
            pTrace->x = pShot->x;
            pTrace->y = pShot->y;
            // 散る時のベースとなる向きを親の進行方向に対して垂直にする
            pTrace->muki = pShot->muki + DX_PI / 2.0;
            pTrace->speed = 0.0; // その場に留まる
            pTrace->kind = img_enemyShotMediumBall[6]; // 白中玉（軌跡弾）
            pTrace->param_i[0] = 1; // フラグ 1:軌跡弾

            pTrace->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pTrace->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pTrace;
            pEnemyShotSet->pEnemyShotHead->prev = pTrace;

        }
        else if (pShot->param_i[0] == 1) { // 軌跡弾の処理
            // 生成されてから一定時間(120フレーム)経過したら散らす
            if (pShot->count == 120) {
                pShot->speed = 0.5 + GetRand(150) / 100.0;
                // 進行方向の左右に分かれて霧散させる
                if (GetRand(1) == 0) pShot->muki += DX_PI;
                pShot->muki += (GetRand(30) - 15) / 180.0 * DX_PI;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ======================================================================
// 弾幕：β線（ジグザグに屈曲する細い軌跡）
// ======================================================================
static void ShotBeta(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count == 40) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 4; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 自機方向から少し広めに発射
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(120) - 60) / 180.0 * DX_PI;
            pEnemyShot->speed = 12.0 + GetRand(20) / 10.0;
            pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青小玉（親弾）
            pEnemyShot->param_i[0] = 0; // フラグ 0:親弾
            pEnemyShot->param_i[1] = 0; // 屈曲した回数
            pEnemyShot->param_i[2] = 8 + GetRand(12); // 次の屈曲タイミング（フレーム数）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動と軌跡生成処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) { // 親弾の処理
            // 屈曲タイミング到達 ＆ 屈曲回数が2回未満なら曲がる
            if (pShot->count == pShot->param_i[2] && pShot->param_i[1] < 2) {
                int sign = (GetRand(1) == 0) ? 1 : -1;
                // 20度〜40度の急角度で屈曲
                double angle = (20.0 + GetRand(20)) / 180.0 * DX_PI;
                pShot->muki += sign * angle;

                pShot->param_i[1]++; // 屈曲回数をインクリメント
                pShot->param_i[2] = pShot->count + 8 + GetRand(12); // 次のタイミングを再設定
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 毎フレーム軌跡（白の小玉）を生成してその場に残す
            sEnemyShot* pTrace = new sEnemyShot;
            pTrace->x = pShot->x;
            pTrace->y = pShot->y;
            pTrace->muki = pShot->muki + DX_PI / 2.0;
            pTrace->speed = 0.0;
            pTrace->kind = img_enemyShotSmallBall[6]; // 白小玉（軌跡弾）
            pTrace->param_i[0] = 1; // フラグ 1:軌跡弾

            pTrace->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pTrace->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pTrace;
            pEnemyShotSet->pEnemyShotHead->prev = pTrace;

        }
        else if (pShot->param_i[0] == 1) { // 軌跡弾の処理
            // α線より少し早いタイミング(90フレーム)で散らす
            if (pShot->count == 90) {
                pShot->speed = 0.8 + GetRand(80) / 100.0;
                if (GetRand(1) == 0) pShot->muki += DX_PI;
                pShot->muki += (GetRand(40) - 20) / 180.0 * DX_PI;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ======================================================================
// 敵本体のパターン関数（メイン呼び出し用）
// ======================================================================
void EnemyPat_CloudChamber_Gemini()
{
    static int muki;

    if (count == 1) {
        // 敵の初期化
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // 【環境演出セット】戦闘開始時に1つだけ常駐させる
        sEnemyShotSet* pEnvSet = new sEnemyShotSet;
        pEnvSet->count = 0;
        pEnvSet->patternFunc = ShotEnv;
        pEnvSet->x = enemy.x;
        pEnvSet->y = enemy.y;
        pEnvSet->muki = 0.0;

        pEnvSet->pEnemyShotHead = new sEnemyShot;
        pEnvSet->pEnemyShotHead->prev = pEnvSet->pEnemyShotHead;
        pEnvSet->pEnemyShotHead->next = pEnvSet->pEnemyShotHead;

        pEnvSet->prev = enemyShotSetHead.prev;
        pEnvSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnvSet;
        enemyShotSetHead.prev = pEnvSet;
    }
    else {
        // 敵の移動処理（ゆっくり左右に揺れる）
        enemy.x += 0.5 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // 240フレーム（約4秒）周期で、α線とβ線を交互に発射
	int cycle = count % 360;

	if (cycle == 0) {
		// α線のセットを登録
		sEnemyShotSet* pSet = new sEnemyShotSet;
		pSet->count = 0;
		pSet->patternFunc = ShotAlpha;
		pSet->x = enemy.x;
		pSet->y = enemy.y;
		pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

		pSet->pEnemyShotHead = new sEnemyShot;
		pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
		pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

		pSet->prev = enemyShotSetHead.prev;
		pSet->next = &enemyShotSetHead;
		enemyShotSetHead.prev->next = pSet;
		enemyShotSetHead.prev = pSet;
	}
	else if (cycle == 180) {
		// β線のセットを登録
		sEnemyShotSet* pSet = new sEnemyShotSet;
		pSet->count = 0;
		pSet->patternFunc = ShotBeta;
		pSet->x = enemy.x;
		pSet->y = enemy.y;
		pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

		pSet->pEnemyShotHead = new sEnemyShot;
		pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
		pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

		pSet->prev = enemyShotSetHead.prev;
		pSet->next = &enemyShotSetHead;
		enemyShotSetHead.prev->next = pSet;
		enemyShotSetHead.prev = pSet;
	}
}