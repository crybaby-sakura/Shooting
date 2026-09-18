// EnemyPat_CloudChamber_Qwen.cpp
// ウィルソンの霧箱をモチーフにした弾幕パターン

// ============================================================
// 弾幕パターン関数：霧（背景演出）
// ============================================================
static void ShotFog(sEnemyShotSet* pEnemyShotSet)
{
    // 8フレームごとに霧の粒子を生成
    if (pEnemyShotSet->count % 8 == 0) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = (double)GetRand(480);
        pShot->y = -10.0;
        pShot->muki = DX_PI / 2.0 + ((double)GetRand(40) - 20.0) / 180.0 * DX_PI;
        pShot->speed = 0.3 + (double)GetRand(10) / 30.0;

        // 色6(白)の小玉を使用。霧のように見せる
        pShot->kind = img_enemyShotSmallBall[6];
        pShot->param_i[0] = 2; // ID: 霧

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;

        if (pShot->param_i[0] == 2) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            // 霧らしく、正弦波でわずかに揺らぐ挙動を加える
            pShot->x += sin(pEnemyShotSet->count / 40.0 + pShot->y / 60.0) * 0.15;
        }

        pShot = next;
    }
    // 霧セットは敵が生きている間ずっと稼働するため、ここでは削除しない
}

// ============================================================
// 弾幕パターン関数：霧箱の飛跡（メイン）
// ============================================================
static void ShotCloudChamber(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 発射予告音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // プレイヤー方向を基準に、わずかにランダムな角度を設定
        double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->param_d[0] = base_muki + ((double)GetRand(60) - 30.0) / 180.0 * DX_PI;
        pEnemyShotSet->param_d[1] = 15.0; // 粒子の高速移動スピード

        // 「荷電粒子」本体の生成（高速で直進する白い銃弾型）
        sEnemyShot* pParticle = new sEnemyShot;
        pParticle->x = pEnemyShotSet->x;
        pParticle->y = pEnemyShotSet->y;
        pParticle->muki = pEnemyShotSet->param_d[0];
        pParticle->speed = pEnemyShotSet->param_d[1];
        pParticle->kind = img_enemyShotBullet[6]; // 色6(白)の銃弾(5.0x2.0)。高速だと筋に見える
        pParticle->param_i[0] = 0; // ID: 粒子本体

        pParticle->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pParticle->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pParticle;
        pEnemyShotSet->pEnemyShotHead->prev = pParticle;
    }
    else {
        // 粒子の軌跡上に「飛跡」を点描のように生成（2フレームごと、40フレーム間）
        if (pEnemyShotSet->count % 2 == 0 && pEnemyShotSet->count <= 40) {
            double tx = pEnemyShotSet->x + cos(pEnemyShotSet->param_d[0]) * pEnemyShotSet->param_d[1] * pEnemyShotSet->count;
            double ty = pEnemyShotSet->y + sin(pEnemyShotSet->param_d[0]) * pEnemyShotSet->param_d[1] * pEnemyShotSet->count;

            sEnemyShot* pTrace = new sEnemyShot;
            pTrace->x = tx;
            pTrace->y = ty;
            pTrace->muki = atan2(player.y - ty, player.x - tx); // 初期状態はプレイヤー方向を向く
            pTrace->speed = 0.0; // 生成直後は静止（凝縮中）
            pTrace->kind = img_enemyShotSmallBall[6]; // 色6(白)の小玉(2.5x2.5)

            pTrace->param_i[0] = 1; // ID: 飛跡
            pTrace->param_i[1] = 0; // 状態: 0=凝縮中, 1=活性化
            pTrace->param_i[2] = 0; // 経過タイマー
            // 種類: 0=ベータ線風(曲がる), 1=アルファ線風(炸裂)。約30%の確率でアルファ線風
            pTrace->param_i[3] = (GetRand(10) < 3) ? 1 : 0;
            // 磁場の強さ（横方向への加速度）。-0.25 ～ 0.25 のランダムな値
            pTrace->param_d[0] = ((double)GetRand(10) - 5.0) / 20.0;

            pTrace->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pTrace->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pTrace;
            pEnemyShotSet->pEnemyShotHead->prev = pTrace;
        }
    }

    // このセットに属する全弾の更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;

        if (pShot->param_i[0] == 0) { // 粒子本体
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] == 1) { // 飛跡
            if (pShot->param_i[1] == 0) { // 凝縮中
                pShot->param_i[2]++;
                // 凝縮中のわずかな流れを表現
                pShot->x += cos(pShot->muki) * 0.3;
                pShot->y += sin(pShot->muki) * 0.3;

                int wait_time = (pShot->param_i[3] == 1) ? 30 : 40;
                if (pShot->param_i[2] > wait_time) {
                    pShot->param_i[1] = 1; // 活性化

                    if (pShot->param_i[3] == 0) {
                        // ベータ線風: 凝縮が完了し、中玉になって曲がり始める
                        pShot->kind = img_enemyShotMediumBall[6]; // 色6(白)の中玉(7.0x7.0)
                        pShot->speed = 2.5;
                    }
                    else {
                        // アルファ線風: 凝縮が完了し、炸裂準備
                        pShot->kind = img_enemyShotMediumOval[6]; // 色6(白)の中楕円(10.5x7.0)
                    }
                }
            }
            else if (pShot->param_i[1] == 1) { // 活性化後
                if (pShot->param_i[3] == 0) { // ベータ線風（磁場で曲がる）
                    // 進行方向(muki)に対して垂直なベクトルに加速度を加える（ローレンツ力の模倣）
                    double ax = -sin(pShot->muki) * pShot->param_d[0];
                    double ay = cos(pShot->muki) * pShot->param_d[0];

                    double vx = pShot->speed * cos(pShot->muki) + ax;
                    double vy = pShot->speed * sin(pShot->muki) + ay;

                    pShot->muki = atan2(vy, vx);
                    pShot->speed = sqrt(vx * vx + vy * vy);
                    if (pShot->speed > 4.0) pShot->speed = 4.0; // 速度上限

                    pShot->x += pShot->speed * cos(pShot->muki);
                    pShot->y += pShot->speed * sin(pShot->muki);
                }
                else { // アルファ線風（炸裂）
                    // 8方向に小玉を炸裂させる
                    for (int i = 0; i < 8; i++) {
                        sEnemyShot* pSub = new sEnemyShot;
                        pSub->x = pShot->x;
                        pSub->y = pShot->y;
                        pSub->muki = (DX_PI * 2.0 / 8.0) * (double)i;
                        pSub->speed = 2.0 + (double)GetRand(10) / 10.0;
                        pSub->kind = img_enemyShotSmallBall[6];
                        pSub->param_i[0] = 3; // ID: 炸裂小弾

                        pSub->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pSub->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pSub;
                        pEnemyShotSet->pEnemyShotHead->prev = pSub;
                    }
                    // 元の飛跡弾は削除マーク
                    pShot->param_i[0] = 99;
                }
            }            
        }
        else if (pShot->param_i[0] == 3) { // 炸裂小弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        // 削除マークが付いた弾の安全な削除とリストからの切り離し
        if (pShot->param_i[0] == 99) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }

        pShot = next;
    }

    // メモリリーク防止: 一定時間経過後にセット自体を破棄
    if (pEnemyShotSet->count > 300) {
        sEnemyShot* pShotDel = pEnemyShotSet->pEnemyShotHead->next;
        while (pShotDel != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* nextDel = pShotDel->next;
            pShotDel->prev->next = pShotDel->next;
            pShotDel->next->prev = pShotDel->prev;
            delete pShotDel;
            pShotDel = nextDel;
        }
        // セットをグローバルリストから切り離して削除
        pEnemyShotSet->prev->next = pEnemyShotSet->next;
        pEnemyShotSet->next->prev = pEnemyShotSet->prev;
        delete pEnemyShotSet;
    }
}

// ============================================================
// 敵本体のパターン制御
// ============================================================
void EnemyPat_CloudChamber_Qwen()
{
    static int shot_count;
    static bool fog_initialized = false;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
        fog_initialized = false;
    }
    else {
        // 敵の移動: 画面中央付近をゆっくりと揺れ動く
        enemy.x = 240.0 + sin(count / 60.0) * 100.0;
        enemy.y = 60.0 + cos(count / 90.0) * 20.0;
    }

    // 霧の生成セットはゲーム開始直後に1度だけ登録する
    if (!fog_initialized && count > 10) {
        sEnemyShotSet* pFogSet = new sEnemyShotSet;
        pFogSet->count = 0;
        pFogSet->patternFunc = ShotFog;
        pFogSet->x = 0.0;
        pFogSet->y = 0.0;
        pFogSet->muki = 0.0;
        pFogSet->kind = 0;

        pFogSet->pEnemyShotHead = new sEnemyShot;
        pFogSet->pEnemyShotHead->prev = pFogSet->pEnemyShotHead;
        pFogSet->pEnemyShotHead->next = pFogSet->pEnemyShotHead;

        pFogSet->prev = enemyShotSetHead.prev;
        pFogSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pFogSet;
        enemyShotSetHead.prev = pFogSet;

        fog_initialized = true;
    }

    // 120フレーム(約2秒)ごとに「霧箱の飛跡」イベントを発火
    if (count % 120 == 1) {
        sEnemyShotSet* pCCSet = new sEnemyShotSet;
        pCCSet->count = 0;
        pCCSet->patternFunc = ShotCloudChamber;
        pCCSet->x = enemy.x;
        pCCSet->y = enemy.y + 10.0;
        pCCSet->muki = 0.0;
        pCCSet->kind = shot_count++;

        pCCSet->pEnemyShotHead = new sEnemyShot;
        pCCSet->pEnemyShotHead->prev = pCCSet->pEnemyShotHead;
        pCCSet->pEnemyShotHead->next = pCCSet->pEnemyShotHead;

        pCCSet->prev = enemyShotSetHead.prev;
        pCCSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pCCSet;
        enemyShotSetHead.prev = pCCSet;
    }
}