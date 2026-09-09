// enemyPat_watermelon.cpp
// スイカ割りをモチーフにした弾幕パターン

#include "gv.h" // 構造体や定数の定義が含まれていると仮定

// ============================================================
// 弾幕パターン関数群
// ============================================================

// Phase 1: スイカの皮（緑色の螺旋弾幕）
static void ShotWatermelonPhase1(sEnemyShotSet* pEnemyShotSet)
{
    // 10フレームごとに螺旋状に発射
    if (pEnemyShotSet->count % 14 == 0) {
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        for (int i = 0; i < 8; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 緑(2)の中玉でスイカの皮の模様を表現
            pEnemyShot->kind = img_enemyShotMediumBall[2];

            // 螺旋状になるように角度をずらす
            pEnemyShot->muki = pEnemyShotSet->param_d[0] + i * (DX_PI * 2.0 / 8.0) + pEnemyShotSet->count / 100.0;
            pEnemyShot->speed = 2.5;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
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

// Phase 2 遷移時: ひび割れ演出（白の鱗弾が放射状に広がる）
static void ShotWatermelonCrack(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 6方向にひび割れのように発射
        for (int i = 0; i < 6; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 白(6)の鱗弾でひびを表現
            pEnemyShot->kind = img_enemyShotScale[6];
            pEnemyShot->muki = i * (DX_PI * 2.0 / 6.0);
            pEnemyShot->speed = 3.5;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// Phase 2: ひび割れ中の拡散弾（シアン/白の菱形弾）
static void ShotWatermelonPhase2(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % 18 == 0) {
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // プレイヤー方向をベースに3方向へ拡散
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // シアン(3)の菱形弾
            pEnemyShot->kind = img_enemyShotDiamond[3];
            pEnemyShot->muki = pEnemyShotSet->muki + (i - 1) * 0.3 + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// Phase 3 遷移時: 大破裂演出（赤の全方位弾 + 黒の高速種）
static void ShotWatermelonExplosion(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 赤(0)の大玉で衝撃波を表現 (36方向)
        for (int i = 0; i < 36; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->kind = img_enemyShotLargeBall[0];
            pEnemyShot->muki = i * (DX_PI * 2.0 / 36.0);
            pEnemyShot->speed = 2.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 黒(7)の小玉で飛び散る種を表現 (20発ランダム)
        for (int i = 0; i < 20; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->kind = img_enemyShotSmallBall[7];
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
            pEnemyShot->speed = 4.0 + GetRand(30) / 10.0; // 4.0 〜 7.0 の高速

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// Phase 3: 破裂中の継続弾（赤と黒の混合）
static void ShotWatermelonPhase3(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % 20 == 0) {
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        for (int i = 0; i < 5; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 赤の大玉と黒の小玉をランダムに混ぜる
            if (GetRand(1) == 0) {
                pEnemyShot->kind = img_enemyShotLargeBall[0];
                pEnemyShot->speed = 2.5;
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[7];
                pEnemyShot->speed = 5.0;
            }

            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// Phase 4: 種の雨（画面上部から落ちてきてバウンドする黒い小弾）
static void ShotWatermelonPhase4(sEnemyShotSet* pEnemyShotSet)
{
    // 5フレームごとに新しい種を降らせる
    if (pEnemyShotSet->count % 10 == 0) {
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        sEnemyShot* pEnemyShot = new sEnemyShot;
        // 画面上部のランダムな位置から発生
        pEnemyShot->x = (double)GetRand(480);
        pEnemyShot->y = -20.0;
        pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒(7)の小玉

        // param_d[0] にY速度、param_d[1] にX速度を格納して重力・バウンド処理に使用
        pEnemyShot->param_d[0] = 2.0 + GetRand(30) / 10.0;       // 下向き初速 2.0 〜 5.0
        pEnemyShot->param_d[1] = (GetRand(20) - 10) / 10.0;      // 横方向のブレ -1.0 〜 1.0

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 重力加速度を加算
        pShot->param_d[0] += 0.15 / 5;

        // 位置更新
        pShot->x += pShot->param_d[1];
        pShot->y += pShot->param_d[0];

        // 画面下端(480)付近でバウンド処理
        if (pShot->y > 470.0 && pShot->param_d[0] > 0) {
            pShot->param_d[0] *= -0.5; // Y速度を反転・減衰
            pShot->param_d[1] *= 0.8;  // X速度も摩擦で減衰

            // 速度が小さくなったら停止させる
            if (pShot->param_d[0] > -0.5) {
                pShot->param_d[0] = 0;
                pShot->param_d[1] = 0;
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン制御
// ============================================================
void EnemyPat_Suikawari_Qwen()
{
    static int phase = 1; // 1:皮, 2:ひび割れ, 3:大破裂, 4:種の雨

    if (count == 1) {
        // ゲーム画面は 480x480。スイカは画面中央やや上に配置
        enemy.x = 240.0;
        enemy.y = 200.0;
        enemy.maxHp = enemy.hp = 200; // HP200で固定
        phase = 1;

        {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotWatermelonPhase1;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShotSet->kind = 0;
            // 螺旋の位相をセットごとにずらす
            pEnemyShotSet->param_d[0] = fmod(count * 0.1, DX_PI * 2.0);

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    else {
        // スイカがゆっくりと揺れる動き
        enemy.x = 240.0 + sin(count * 0.02) * 40.0;
        enemy.y = 200.0 + cos(count * 0.03) * 20.0;

        // HPに応じたフェーズ遷移と演出発動
        if (enemy.hp <= 50 && phase < 4) {
            phase = 4;

            {
                sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                pEnemyShotSet->count = 0;
                pEnemyShotSet->patternFunc = ShotWatermelonPhase4;
                // 種の雨は敵の位置ではなく、画面上部全体から降らせる
                pEnemyShotSet->x = 0.0;
                pEnemyShotSet->y = 0.0;
                pEnemyShotSet->muki = 0;
                pEnemyShotSet->kind = 0;

                pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

                pEnemyShotSet->prev = enemyShotSetHead.prev;
                pEnemyShotSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pEnemyShotSet;
                enemyShotSetHead.prev = pEnemyShotSet;
            }
        }
        else if (enemy.hp <= 100 && phase < 3) {
            phase = 3;
            // 大破裂演出のShotSetを生成
            sEnemyShotSet* pExplosionSet = new sEnemyShotSet;
            pExplosionSet->count = 0;
            pExplosionSet->patternFunc = ShotWatermelonExplosion;
            pExplosionSet->x = enemy.x;
            pExplosionSet->y = enemy.y;
            pExplosionSet->muki = 0;
            pExplosionSet->kind = 0;

            pExplosionSet->pEnemyShotHead = new sEnemyShot;
            pExplosionSet->pEnemyShotHead->prev = pExplosionSet->pEnemyShotHead;
            pExplosionSet->pEnemyShotHead->next = pExplosionSet->pEnemyShotHead;

            pExplosionSet->prev = enemyShotSetHead.prev;
            pExplosionSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pExplosionSet;
            enemyShotSetHead.prev = pExplosionSet;

            {
                sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                pEnemyShotSet->count = 0;
                pEnemyShotSet->patternFunc = ShotWatermelonPhase3;
                pEnemyShotSet->x = enemy.x;
                pEnemyShotSet->y = enemy.y;
                pEnemyShotSet->muki = 0;
                pEnemyShotSet->kind = 0;

                pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

                pEnemyShotSet->prev = enemyShotSetHead.prev;
                pEnemyShotSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pEnemyShotSet;
                enemyShotSetHead.prev = pEnemyShotSet;
            }
        }
        else if (enemy.hp <= 150 && phase < 2) {
            phase = 2;
            // ひび割れ演出のShotSetを生成
            sEnemyShotSet* pCrackSet = new sEnemyShotSet;
            pCrackSet->count = 0;
            pCrackSet->patternFunc = ShotWatermelonCrack;
            pCrackSet->x = enemy.x;
            pCrackSet->y = enemy.y;
            pCrackSet->muki = 0;
            pCrackSet->kind = 0;

            pCrackSet->pEnemyShotHead = new sEnemyShot;
            pCrackSet->pEnemyShotHead->prev = pCrackSet->pEnemyShotHead;
            pCrackSet->pEnemyShotHead->next = pCrackSet->pEnemyShotHead;

            pCrackSet->prev = enemyShotSetHead.prev;
            pCrackSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pCrackSet;
            enemyShotSetHead.prev = pCrackSet;

            {
                sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                pEnemyShotSet->count = 0;
                pEnemyShotSet->patternFunc = ShotWatermelonPhase2;
                pEnemyShotSet->x = enemy.x;
                pEnemyShotSet->y = enemy.y;
                pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
                pEnemyShotSet->kind = 0;

                pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

                pEnemyShotSet->prev = enemyShotSetHead.prev;
                pEnemyShotSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pEnemyShotSet;
                enemyShotSetHead.prev = pEnemyShotSet;
            }
        }

        //// 各フェーズに応じた定期弾幕生成
        //if (phase == 1 && count % 15 == 1) {
        //    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        //    pEnemyShotSet->count = 0;
        //    pEnemyShotSet->patternFunc = ShotWatermelonPhase1;
        //    pEnemyShotSet->x = enemy.x;
        //    pEnemyShotSet->y = enemy.y;
        //    pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        //    pEnemyShotSet->kind = 0;
        //    // 螺旋の位相をセットごとにずらす
        //    pEnemyShotSet->param_d[0] = fmod(count * 0.1, DX_PI * 2.0);

        //    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        //    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        //    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        //    pEnemyShotSet->prev = enemyShotSetHead.prev;
        //    pEnemyShotSet->next = &enemyShotSetHead;
        //    enemyShotSetHead.prev->next = pEnemyShotSet;
        //    enemyShotSetHead.prev = pEnemyShotSet;
        //}
        //else if (phase == 2 && count % 20 == 1) {
        //    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        //    pEnemyShotSet->count = 0;
        //    pEnemyShotSet->patternFunc = ShotWatermelonPhase2;
        //    pEnemyShotSet->x = enemy.x;
        //    pEnemyShotSet->y = enemy.y;
        //    pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        //    pEnemyShotSet->kind = 0;

        //    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        //    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        //    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        //    pEnemyShotSet->prev = enemyShotSetHead.prev;
        //    pEnemyShotSet->next = &enemyShotSetHead;
        //    enemyShotSetHead.prev->next = pEnemyShotSet;
        //    enemyShotSetHead.prev = pEnemyShotSet;
        //}
        //else if (phase == 3 && count % 10 == 1) {
        //    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        //    pEnemyShotSet->count = 0;
        //    pEnemyShotSet->patternFunc = ShotWatermelonPhase3;
        //    pEnemyShotSet->x = enemy.x;
        //    pEnemyShotSet->y = enemy.y;
        //    pEnemyShotSet->muki = 0;
        //    pEnemyShotSet->kind = 0;

        //    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        //    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        //    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        //    pEnemyShotSet->prev = enemyShotSetHead.prev;
        //    pEnemyShotSet->next = &enemyShotSetHead;
        //    enemyShotSetHead.prev->next = pEnemyShotSet;
        //    enemyShotSetHead.prev = pEnemyShotSet;
        //}
        //else if (phase == 4 && count % 5 == 1) {
        //    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        //    pEnemyShotSet->count = 0;
        //    pEnemyShotSet->patternFunc = ShotWatermelonPhase4;
        //    // 種の雨は敵の位置ではなく、画面上部全体から降らせる
        //    pEnemyShotSet->x = 0.0;
        //    pEnemyShotSet->y = 0.0;
        //    pEnemyShotSet->muki = 0;
        //    pEnemyShotSet->kind = 0;

        //    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        //    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        //    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        //    pEnemyShotSet->prev = enemyShotSetHead.prev;
        //    pEnemyShotSet->next = &enemyShotSetHead;
        //    enemyShotSetHead.prev->next = pEnemyShotSet;
        //    enemyShotSetHead.prev = pEnemyShotSet;
        //}
    }
}