// EnemyPat_Inbachi_Grok.cpp
// 陰蜂風弾幕：陰の結晶輪・絶望交差
// 使える素材を抜粋して使用

// ------------------------------------------------------------
// 結晶輪＋青針弾パターン
// ------------------------------------------------------------
static void ShotCrystalRing(sEnemyShotSet* pEnemyShotSet)
{
    // count==0 で結晶8つを生成（永続）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 8; i++) {
            sEnemyShot* pCrystal = new sEnemyShot;
            pCrystal->x = enemy.x;
            pCrystal->y = enemy.y;
            pCrystal->muki = 0.0;
            pCrystal->speed = 0.0;
            // 黒の大玉を結晶に見立てる
            pCrystal->kind = img_enemyShotLargeBall[7];
            // param_d[0]: 現在角度
            // param_d[1]: 半径
            // param_d[2]: 角速度
            // param_i[0]: 回転方向 (+1/-1)
            // param_i[1]: 1=結晶フラグ
            pCrystal->param_d[0] = (double)i * (DX_PI * 2.0 / 8.0) + (GetRand(60) - 30) / 180.0 * DX_PI;
            pCrystal->param_d[1] = (i % 2 == 0) ? 55.0 + GetRand(20) : 95.0 + GetRand(25); // 内輪・外輪
            pCrystal->param_d[2] = 0.025 + (GetRand(20) / 1000.0); // 角速度
            pCrystal->param_i[0] = (GetRand(1) == 0) ? 1 : -1;
            pCrystal->param_i[1] = 1; // 結晶
            pCrystal->margin = 240;

            pCrystal->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pCrystal->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pCrystal;
            pEnemyShotSet->pEnemyShotHead->prev = pCrystal;
        }
    }

    // 毎フレーム：結晶位置更新＋青針弾生成、通常弾移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 追加中に安全に進める

        if (pShot->param_i[1] == 1) {
            // --- 結晶の軌道更新 ---
            pShot->param_d[0] += pShot->param_d[2] * (double)pShot->param_i[0];
            pShot->x = enemy.x + pShot->param_d[1] * cos(pShot->param_d[0]);
            pShot->y = enemy.y + pShot->param_d[1] * sin(pShot->param_d[0]);

            // 一定間隔で青針弾を発射（高密度）
            // count は自動インクリメントされるのでそれを利用
            if (pShot->count > 0 && (pShot->count % 3) == 0) {
                // 1〜2発
                int num = 1 + GetRand(1);
                for (int n = 0; n < num; n++) {
                    sEnemyShot* pNeedle = new sEnemyShot;
                    pNeedle->x = pShot->x;
                    pNeedle->y = pShot->y;
                    // おおむね外向き＋揺らぎで交差を生む
                    double base = pShot->param_d[0];
                    pNeedle->muki = base + (GetRand(90) - 45) / 180.0 * DX_PI;
                    pNeedle->speed = 5.5 + GetRand(150) / 100.0; // 超高速
                    // 青の銃弾（針弾っぽい）
                    pNeedle->kind = img_enemyShotBullet[4];
                    pNeedle->param_i[1] = 0; // 通常弾

                    pNeedle->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNeedle->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNeedle;
                    pEnemyShotSet->pEnemyShotHead->prev = pNeedle;
                }
            }
        }
        else {
            // --- 通常青針弾の移動 ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// ------------------------------------------------------------
// 赤弾＋ふぐ刺しパターン
// ------------------------------------------------------------
static void ShotRedFugu(sEnemyShotSet* pEnemyShotSet)
{
    // 約4秒おき（240フレーム想定）に32-way赤弾を生成
    if (pEnemyShotSet->count > 0 && (pEnemyShotSet->count % 240) == 1) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double baseMuki = atan2(player.y - enemy.y, player.x - enemy.x);
        for (int i = 0; i < 32; i++) {
            sEnemyShot* pRed = new sEnemyShot;
            pRed->x = enemy.x;
            pRed->y = enemy.y;
            pRed->muki = baseMuki + (double)i * (DX_PI * 2.0 / 32.0);
            pRed->speed = 1.8;
            // 赤の中玉
            pRed->kind = img_enemyShotMediumBall[0];
            // param_i[0]: フェーズ 0=移動中 1=放出中
            // param_i[1]: 放出回数カウンタ
            // param_i[2]: 未使用
            pRed->param_i[0] = 0;
            pRed->param_i[1] = 0;
            pRed->param_i[1] = 0; // 放出済回数
            pRed->margin = 240;

            pRed->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRed->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRed;
            pEnemyShotSet->pEnemyShotHead->prev = pRed;
        }
    }

    // 毎フレーム処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
            // 赤弾の移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 18フレーム後に放出開始
            if (pShot->count == 18) {
                pShot->param_i[0] = 1;
            }

            // 放出フェーズ：4フレームごとにふぐ刺しを最大15回
            if (pShot->param_i[0] == 1 && (pShot->count % 4) == 0 && pShot->param_i[1] < 15) {
                // 回転しながら1発（密度を上げたい場合はループ数を増やす）
                sEnemyShot* pFugu = new sEnemyShot;
                pFugu->x = pShot->x;
                pFugu->y = pShot->y;
                // 時間で回転する向き
                double rot = (double)pShot->count * 0.15 + (double)pShot->param_i[1] * 0.4;
                pFugu->muki = pShot->muki + rot;
                pFugu->speed = 2.8 + GetRand(80) / 100.0;
                // 鱗弾 or 菱形をふぐ刺しに見立てる（赤寄り）
                pFugu->kind = img_enemyShotScale[0];
                pFugu->param_i[0] = 2; // 通常ふぐ弾フラグ
                pFugu->margin = 240;

                pFugu->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pFugu->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pFugu;
                pEnemyShotSet->pEnemyShotHead->prev = pFugu;

                pShot->param_i[1]++;
            }
        }
        else {
            // ふぐ刺し弾の移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// ------------------------------------------------------------
// 敵本体パターン（必須名：EnemyPat_Inbachi_Grok）
// ------------------------------------------------------------
void EnemyPat_Inbachi_Grok()
{
    static int moveDir = 1;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;

        // 結晶輪セットを1つだけ生成（永続動作）
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotCrystalRing;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // 赤弾＋ふぐ刺しセットを1つ生成（永続で定期発射）
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotRedFugu;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        moveDir = 1;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x += 0.7 * (double)moveDir;
        if (enemy.x < 120.0) {
            enemy.x = 120.0;
            moveDir = 1;
        }
        else if (enemy.x > 360.0) {
            enemy.x = 360.0;
            moveDir = -1;
        }
        // 念のため上下もわずかに
        enemy.y = 80.0 + 8.0 * sin((double)count * 0.02);
    }
}