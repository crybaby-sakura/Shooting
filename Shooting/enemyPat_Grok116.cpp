// enemyPat_Tmp.cpp
// 弾幕：ウィルソン霧箱の飛跡（Cloud Chamber Tracks）
// 既存弾のみで「粒子本体 + 白い飛跡」を表現

// 使用素材:
//   粒子本体 : img_enemyShotMediumBall[6] （白の中玉）
//   飛跡     : img_enemyShotSmallBall[6] （白の小玉）
//   効果音   : sound_enemyShot_medium

static void ShotCloudChamber(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --- 初期化時および一定間隔で粒子を放出 ---
    // count==0 で最初の粒子群、その後も定期的に追加放出
    if (pEnemyShotSet->count == 0) {

        // 効果音（重複再生防止）
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 1回の放出で 3〜5 本の粒子を飛ばす
        int num = 3 + GetRand(2);  // 3,4,5
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;

            // 放出位置はセット位置を中心に少しばらつかせる
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(60) - 30);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(20) - 10);

            // 主に下方向へ。少し左右に開き、緩やかなカーブも付与
            double baseMuki = DX_PI * 0.5;  // 下向き
            double spread = (GetRand(100) - 50) / 180.0 * DX_PI;  // ±約50度
            pEnemyShot->muki = baseMuki + spread;

            // 速度はそこそこ速め（飛跡がきれいに残る速度）
            pEnemyShot->speed = 3.2 + GetRand(120) / 100.0;  // 3.2 〜 4.4

            // 白の中玉を粒子本体に使用
            pEnemyShot->kind = img_enemyShotMediumBall[6];

            // パラメータ: param_i[0] = 1 なら「粒子本体」
            pEnemyShot->param_i[0] = 1;
            // 飛跡生成用カウンタ
            pEnemyShot->param_i[1] = 0;

            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 全弾の更新 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 位置更新
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 粒子本体の場合：飛跡を残す + ごく弱い揺らぎ
        if (pShot->param_i[0] == 1) {
            // 弱いランダム揺らぎ（ベータ線っぽい微細な曲がりを表現）
            if (GetRand(7) == 0) {
                pShot->muki += (GetRand(20) - 10) / 1800.0 * DX_PI;
            }

            // 2フレームに1回、現在位置に飛跡弾を生成
            pShot->param_i[1]++;
            if (pShot->param_i[1] % 2 == 0) {
                sEnemyShot* pTrail = new sEnemyShot;
                pTrail->x = pShot->x;
                pTrail->y = pShot->y;
                pTrail->muki = pShot->muki;          // 一応進行方向を持たせる
                pTrail->speed = 0.0;                 // 最初は静止（飛跡として残る）
                pTrail->kind = img_enemyShotSmallBall[6];  // 白の小玉
                pTrail->param_i[0] = 0;              // 飛跡フラグ
                pTrail->param_i[1] = 0;              // 寿命用

                pTrail->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pTrail->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pTrail;
                pEnemyShotSet->pEnemyShotHead->prev = pTrail;
            }
        }
        // 飛跡弾の場合：一定時間後にゆっくり拡散させて画面外へ逃がす
        else {
            // 寿命カウンタ
            pShot->param_i[1]++;

            // 約1秒（60フレーム）経過後、ゆっくり外へ動かし始める
            if (pShot->param_i[1] > 55 && pShot->speed < 0.01) {
                // ランダムな方向へごく遅い速度を与える
                pShot->muki = GetRand(360) / 180.0 * DX_PI;
                pShot->speed = 0.15 + GetRand(20) / 100.0;  // 0.15〜0.35
            }
            // さらに時間が経ったら少し加速して確実に消えるようにする
            if (pShot->param_i[1] > 120) {
                pShot->speed += 0.04;
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_CloudChamber_Grok()
{
    static int muki;
    static int phase;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 80.0)  muki = 1;
        if (enemy.x > 400.0) muki = -1;

        // 少し上下にも揺れる
        enemy.y = 50.0 + 8.0 * sin(count * 0.04);
    }

    // 攻撃開始タイミング（出現後少し待ってから）
    if (count >= 40 && (count - 40) % 300 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCloudChamber;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = 0.0;          // このパターンではセットの向きはほぼ使わない
        pEnemyShotSet->kind = phase++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}